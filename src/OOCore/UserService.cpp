#include "./UserService.h"

const Omega::guid_t Session::IID_InputCDR = { 0xe39658, 0x1774, 0x4f02, { 0x86, 0xe5, 0xfc, 0xe8, 0xbe, 0x3c, 0xe4, 0xa5 } };
OMEGA_DECLARE_IID_TRAITS(Session,InputCDR)

const Omega::guid_t Session::IID_OutputCDR = { 0x21118e84, 0x2ef8, 0x4f53, { 0xb4, 0xfd, 0xdb, 0xd4, 0xee, 0xc3, 0xaf, 0xc3 } };
OMEGA_DECLARE_IID_TRAITS(Session,OutputCDR)

using namespace Omega;
using namespace OTL;

void UserService::process_message(void* param)
{
	// Ensure pMsg gets deleted
	ACE_Auto_Ptr<msg_param> pMsg(static_cast<msg_param*>(param));
	
	// Call OnReceive
	pMsg->ptrSink->OnReceiveMessage(pMsg->ptrInput,pMsg->cookie);
}

int UserService::handle_input(ACE_HANDLE /*fd*/)
{
	ACE_Message_Block* mb;
	if (recv(mb) != 0)
		return -1;
	
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Append the new data
	if (m_curr_block)
	{
		// Append the message block
		m_curr_block->cont(mb);	
	}
	else
		m_curr_block = mb;

	// Create an input stream
	ACE_InputCDR input(m_curr_block);
	
	m_curr_block->release();
	m_curr_block = 0;

    // Loop copying bytes from the new_block
	while (input.length()>0)
	{
		// Stash current read point
		char* rd_ptr = input.start()->rd_ptr();

		// Read the header
		size_t msg_size = 0;
		int ret = read_header(input,msg_size);
		if (ret == -1)
			return -1;

		if (ret==1) // More data required
		{
			// Update m_curr_block to input's message block
			m_curr_block = input.start()->duplicate();

			// Reset the amount just read, we read again in a moment
			m_curr_block->rd_ptr(rd_ptr - m_curr_block->rd_ptr());

			return 0;
		}

		if (msg_size>0)
		{
			// Try to post it
			msg_param* p;
			ACE_NEW_RETURN(p,msg_param,-1);
						
			try
			{
				p->cookie = m_cookie;
				p->ptrSink = m_ptrSink;

				// Wrap the input
				p->ptrInput.Attach(ObjectImpl<Session::InputCDR>::CreateObject());
				p->ptrInput->Init(input);
			
				// Move this stuff to StdObjectManager
				void* MOVE_ME;

				ObjectPtr<Activation::IApartment> ptrApartment;
				ptrApartment.Attach(Activation::IApartment::GetCurrentApartment());
				ptrApartment->PostRequest(process_message,p);
			}
			catch (...)
			{
				delete p;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Exception thrown by process message!\n")),-1);
			}

			// Skip the bytes of the message
			if (!input.skip_bytes(msg_size))
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to skip header bytes\n")),-1);
		}
	}

	return 0;
}

int UserService::handle_output(ACE_HANDLE /*fd*/)
{
	return send_i();
}

int UserService::handle_close(ACE_HANDLE /*fd*/, ACE_Reactor_Mask /*mask*/)
{
	try
	{
		// Notify Sink we have gone!
		if (m_ptrSink)
		{
			ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

			// Swap out m_ptrSink
			ObjectPtr<Remoting::IChannelSink> ptrSink;
			ptrSink.Attach(m_ptrSink.Detach());

			ptrSink->OnDisconnect(m_cookie);
		}
	}
	catch (IException* pE)
	{
		pE->Release();
	}		
	
	// Do not call svc_class::handle_close() it calls delete this!
	return 0;
}

void UserService::send(ACE_Message_Block* mb, ACE_Time_Value* wait)
{
	if (this->putq(mb,wait) == -1)
		OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
	
	if (send_i() < 0)
		OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
}

int UserService::recv(ACE_Message_Block*& mb, ACE_Time_Value* /*wait*/)
{
    ACE_NEW_RETURN(mb,ACE_Message_Block(1024),-1);

	// Recv some data
	ssize_t recv_cnt = this->peer().recv(mb->wr_ptr(),1024);
	
	// See if we got anything
	if (recv_cnt > 0)
	{
		// Set the wr_ptr to the end
		mb->wr_ptr(recv_cnt);
		return 0;
	}

	if (recv_cnt==0 || ACE_OS::last_error() != EWOULDBLOCK)
	{
		// Connection closed
		mb->release();
		return -1;
	}

	return 0;
}

int UserService::read_header(ACE_InputCDR& input, size_t& msg_size)
{
	ACE_CDR::UShort header_size;

	// Read the header size
	if (input.length() < sizeof(header_size))
		return 1;
	if (!input.read_ushort(header_size))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to reader header size\n")),-1);

	// Check the header size
	if (header_size != sizeof(header_size) + sizeof(ACE_CDR::ULong))
	{
		errno = EFAULT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Bad header\n")),-1);
	}
	
	// Check the header length
	if (input.length() < header_size - sizeof(header_size))
		return 1;

	// Read the message length
	ACE_CDR::ULong msg_size_wire;
	if (!input.read_ulong(msg_size_wire))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read message length\n")),-1);
	
	// Read the msg data
	if (input.length() < msg_size_wire)
		return 1;

	// Return the msg size
	msg_size = static_cast<size_t>(msg_size_wire);
	
	return 0;
}

int UserService::send_i()
{
	ACE_Message_Block *mb;
	ACE_Time_Value nowait(ACE_OS::gettimeofday());
	while (-1 != this->getq(mb, &nowait))
	{
		// Send the data
		ssize_t send_cnt = peer().send(mb->rd_ptr(),mb->length());
		if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Error sending data: %m\n")),-1);
		
		if (send_cnt == -1)
			send_cnt = 0;
		
		if (mb->total_length() > static_cast<size_t>(send_cnt))
		{
			this->ungetq(mb);
			break;
		}
		
		mb->release();
	}

	if (this->msg_queue()->is_empty())
		this->reactor()->cancel_wakeup(this,ACE_Event_Handler::WRITE_MASK);
	else
		this->reactor()->schedule_wakeup(this,ACE_Event_Handler::WRITE_MASK);

	return 0;
}

Serialize::IFormattedStream* UserService::CreateStream(IObject* pOuter)
{
	if (pOuter)
	{
		ObjectPtr<AggregatedObjectImpl<Session::OutputCDR> > ptrStream(AggregatedObjectImpl<Session::OutputCDR>::CreateObjectPtr(pOuter));
		return reinterpret_cast<Serialize::IFormattedStream*>(ptrStream->QueryInterface(Serialize::IID_IFormattedStream));
	}
	else
	{
		ObjectPtr<ObjectImpl<Session::OutputCDR> > ptrStream(ObjectImpl<Session::OutputCDR>::CreateObjectPtr());
		return ptrStream.AddRefReturn();
	}
}

void UserService::Attach(Remoting::IChannel* pOverlay, Remoting::IChannelSink* pSink, Omega::uint32_t cookie)
{
	// We don't support overlays here
	if (pOverlay)
		OMEGA_THROW(ACE_OS::strerror(EINVAL));
		
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (m_ptrSink)
		OMEGA_THROW(ACE_OS::strerror(EISCONN));
		
	m_ptrSink = pSink;
	m_cookie = cookie;
}

void UserService::Detach()
{
	if (this->idle() != 0)
		OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
}

void UserService::SendMessage(Serialize::IFormattedStream* pStream)
{
	if (!pStream)
		OMEGA_THROW(ACE_OS::strerror(EINVAL));

	// QI for OutputCDR - this will throw
	ObjectPtr<Session::OutputCDR> ptrStream(pStream);

	// Align the output buffer end position, because it may get concatenated with another
	ptrStream->align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Get the length as a uint32_t
	size_t l = ptrStream->total_length();
	if (l >= 0xffffffff - (sizeof(Omega::uint16_t) + sizeof(Omega::uint32_t)))
		OMEGA_THROW(ACE_OS::strerror(E2BIG));
	
	// Write a header
	ObjectPtr<ObjectImpl<Session::OutputCDR> > ptrHeader(ObjectImpl<Session::OutputCDR>::CreateObjectPtr());
	ptrHeader->WriteUInt16(sizeof(Omega::uint16_t) + sizeof(Omega::uint32_t));
	ptrHeader->WriteUInt32(static_cast<Omega::uint32_t>(l));
		
	// Append the stream
	ptrHeader->write_octet_array_mb(ptrStream->begin());

	// Make a copy and send
	ACE_Message_Block* mb = ptrHeader->begin()->duplicate();
	try
	{
		send(mb);
	}
	catch (...)
	{
		mb->release();
		throw;
	}
}
