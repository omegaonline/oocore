#include "./Transport_Impl.h"

//#include "./OOCore_Impl.h"
#include "./Object.h"
#include "./Engine.h"
#include "./OutputStream_CDR.h"

OOCore::Transport_Impl::Transport_Impl(void) :
	m_curr_block(0)
{
}

OOCore::Transport_Impl::~Transport_Impl(void)
{
}

int 
OOCore::Transport_Impl::open_transport()
{
	if (m_ptrOM)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Transport already open!\n")),-1);

	ACE_NEW_RETURN(m_ptrOM,OOCore::ObjectManager,-1);

	return m_ptrOM->Open(this,false);
}

int 
OOCore::Transport_Impl::close_transport()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;
	m_ptrOM = 0;

	guard.release();

	if (ptrOM)
		ptrOM->Close();

	return 0;
}

int 
OOCore::Transport_Impl::handle_recv()
{
	ACE_Message_Block* mb;
	if (recv(mb) != 0)
		return -1;

	return process_block(mb);
}

int
OOCore::Transport_Impl::process_block(ACE_Message_Block* mb)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

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
		ACE_CDR::ULong msg_size;
		int ret = read_header(input,msg_size);
		if (ret == -1)
			return -1;

		if (ret==1) // More data required
		{
			//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) transport requires more data\n")));

			// Update m_curr_block to input's message block
			m_curr_block = input.start()->duplicate();

			// Reset the amount just read, we read again in a moment
			m_curr_block->rd_ptr(rd_ptr - m_curr_block->rd_ptr());

			return 0;
		}

		if (msg_size>0)
		{
			// Clone and post the input
			Object_Ptr<Impl::InputStream_CDR> i;
			ACE_NEW_RETURN(i,Impl::InputStream_CDR(ACE_InputCDR(input,msg_size)),-1);

			msg_param* p;
			ACE_NEW_RETURN(p,msg_param(m_ptrOM,i),-1);
			if (ENGINE::instance()->post_request(p) != 0)
			{
				delete p;
				return -1;
			}

			// Skip the bytes of the message
			if (!input.skip_bytes(msg_size))
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to skip header bytes\n")),-1);
		}
	}

	return 0;
}

int
OOCore::Transport_Impl::read_header(ACE_InputCDR& input, size_t& msg_size)
{
	ACE_CDR::UShort header_size;

	// Read the header size
	if (input.length() < sizeof(header_size))
		return 1;
	if (!input.read_ushort(header_size))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to reader header size\n")),-1);

	// Check the header size
	if (header_size != sizeof(header_size) + sizeof(ACE_CDR::ULong))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Bad header\n")),-1);
	
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

OOObject::int32_t 
OOCore::Transport_Impl::CreateObject(const OOObject::char_t* class_name, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;

	guard.release();

	if (!ptrOM)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No server\n")),-1);

	return ptrOM->CreateObject(class_name,iid,ppVal);
}

int 
OOCore::Transport_Impl::CreateOutputStream(OutputStream** ppStream)
{
	if (!ppStream)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

	Impl::OutputStream_CDR* pStream;
	ACE_NEW_RETURN(pStream,Impl::OutputStream_CDR(reinterpret_cast<unsigned long>(this)),-1);

	*ppStream = pStream;
	(*ppStream)->AddRef();

	return 0;
}

int 
OOCore::Transport_Impl::Send(OutputStream* output)
{
	Impl::OutputStream_CDR* pStream = reinterpret_cast<Impl::OutputStream_CDR*>(output);

	if (pStream->get_magic() != reinterpret_cast<unsigned long>(this))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream passed in to transport\n")),-1);

	ACE_Message_Block* mb = pStream->begin()->duplicate();
	if (send(mb) != 0)
	{
		mb->release();
		return -1;
	}

	return 0;
}
