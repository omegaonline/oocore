#include "./Transport_Impl.h"
#include "./Object.h"
#include "./Engine.h"
#include "./OutputStream_CDR.h"

OOCore::Transport_Impl::Transport_Impl(void) :
	m_curr_block(0)
{
}

OOCore::Transport_Impl::~Transport_Impl(void)
{
	while (!m_init_queue.empty())
	{
		m_init_queue.front()->release();
		m_init_queue.pop();
	}
}

int 
OOCore::Transport_Impl::open_transport(const bool bAcceptor)
{
	if (m_ptrOM)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Transport already open!\n")),-1);
	}

	ACE_NEW_RETURN(m_ptrOM,OOCore::ObjectManager,-1);
	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;

	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	while (!m_init_queue.empty())
	{
		if (process_block(m_init_queue.front()) != 0)
		{
			m_ptrOM = 0;
			return -1;
		}
		m_init_queue.pop();
	}

	guard.release();

	return ptrOM->Open(this,bAcceptor);
}

int 
OOCore::Transport_Impl::close_transport(bool transport_alive)
{
	// Get the object manager
	Object_Ptr<ObjectManager> ptrOM = m_ptrOM.clear();
	
	if (ptrOM)
	{
		// Call close
		ptrOM->Close(transport_alive);
	}

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

	if (!m_ptrOM)
	{
		m_init_queue.push(mb);
		return 0;
	}

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
			// Clone the input
			Object_Ptr<Impl::InputStream_CDR> i;
			ACE_NEW_RETURN(i,Impl::InputStream_CDR(ACE_InputCDR(input.start())),-1);

			// Try to post it
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

OOObject::int32_t 
OOCore::Transport_Impl::CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;
	if (!ptrOM)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No object manager\n")),-1);
	}

	return ptrOM->RequestRemoteObject(remote_url,clsid,iid,ppVal);
}

OOObject::int32_t 
OOCore::Transport_Impl::AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, ObjectFactory* pFactory)
{
	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;
	if (!ptrOM)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No object manager\n")),-1);
	}

	return ptrOM->AddObjectFactory(flags,clsid,pFactory);
}

OOObject::int32_t 
OOCore::Transport_Impl::RemoveObjectFactory(const OOObject::guid_t& clsid)
{
	Object_Ptr<ObjectManager> ptrOM = m_ptrOM;
	if (!ptrOM)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No object manager\n")),-1);
	}

	return ptrOM->RemoveObjectFactory(clsid);
}

int 
OOCore::Transport_Impl::CreateOutputStream(OutputStream** ppStream)
{
	if (!ppStream)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	Impl::OutputStream_CDR* pStream;
	ACE_NEW_RETURN(pStream,Impl::OutputStream_CDR(),-1);

	*ppStream = pStream;
	(*ppStream)->AddRef();

	return 0;
}

int 
OOCore::Transport_Impl::Send(OutputStream* output)
{
	// See if output is a Impl::OutputStream_CDR
	Object_Ptr<Impl::OutputStream_CDR> pStream;
	if (output->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&pStream)) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream passed in to transport\n")),-1);
	
	// Align the output buffer end position, because it may get concatenated with another
	pStream->align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write a header
    ACE_OutputCDR header;
	if (!header.write_ushort(sizeof(ACE_CDR::UShort) + sizeof(ACE_CDR::ULong)) ||
		!header.write_ulong(pStream->begin()->total_length()))
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to compose header\n")),-1);
	}

	// Append the data
	ACE_Message_Block* mb = header.begin()->duplicate();
	mb->cont(pStream->begin()->duplicate());
		
	// Send the data
	if (send(mb) != 0)
	{
		mb->release();
		return -1;
	}

	return 0;
}

OOObject::int32_t 
OOCore::Transport_Impl::CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	return this->CreateRemoteObject(0,clsid,iid,ppVal);
}
