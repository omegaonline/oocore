/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./MessageConnection.h"

Root::MessageConnection::MessageConnection(Root::MessageHandler* pHandler) :
	m_pHandler(pHandler)
{
}

Root::MessageConnection::~MessageConnection()
{
}

ACE_CDR::UShort Root::MessageConnection::attach(ACE_HANDLE new_handle)
{
	// Open the reader
	if (m_reader.open(*this,new_handle) != 0)
	    ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::MessageConnection::connect")),0);

	ACE_CDR::UShort uId = m_pHandler->register_channel(new_handle);
	if (uId == 0)
		return 0;

	if (!read())
		return 0;

	return uId;
}

void Root::MessageConnection::open(ACE_HANDLE new_handle, ACE_Message_Block& /*mb*/)
{
	// Stash the handle
	this->handle(new_handle);

	// Open the reader
	if (m_reader.open(*this,new_handle) != 0)
	{
	    ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("Root::MessageConnection::open")));
		delete this;
		return;
	}

	ACE_CDR::UShort uId = m_pHandler->register_channel(new_handle);
	if (uId == 0)
	{
	    delete this;
		return;
	}

	if (!read())
		delete this;
}

bool Root::MessageConnection::read()
{
	// We read the header first
	m_read_len = 0;
	ACE_Message_Block* mb;
	ACE_NEW_RETURN(mb,ACE_Message_Block(2048),false);

	// Align the message block for CDR
	ACE_CDR::mb_align(mb);

	// Start an async read
	if (m_reader.read(*mb,s_initial_read) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::MessageConnection::read")));
		mb->release();
		return false;
	}

	return true;
}

void Root::MessageConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = false;
	if (result.success())
	{
		if (m_read_len==0)
		{
			// Read the header length
			if (result.bytes_transferred() == s_initial_read)
			{
				// Create a temp input CDR
				ACE_InputCDR input(mb.data_block(),ACE_Message_Block::DONT_DELETE,static_cast<size_t>(mb.rd_ptr() - mb.base()),static_cast<size_t>(mb.wr_ptr() - mb.base()));
				input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

				// Read and set the byte order
				if (input.read_octet(m_byte_order) && input.read_octet(m_version))
				{
					input.reset_byte_order(m_byte_order);

					// Read the length
					input >> m_read_len;
					if (input.good_bit())
					{
						// Resize the message block
						if (mb.size(m_read_len) == 0)
						{
							// Subtract what we have already read
							m_read_len -= static_cast<ACE_CDR::ULong>(mb.length());

							mb.rd_ptr(s_initial_read);

							// Issue another read for the rest of the data
							bSuccess = (m_reader.read(mb,m_read_len) == 0);
							if (bSuccess)
								return;
						}
					}
				}
			}
		}
		else
		{
			// Check the header length
			if (result.bytes_transferred() == m_read_len)
			{
				// Create a new input CDR wrapping mb
				ACE_InputCDR* input = 0;
				size_t rd_ptr = static_cast<size_t>(mb.rd_ptr() - mb.base());
				size_t wr_ptr = static_cast<size_t>(mb.wr_ptr() - mb.base());
				ACE_NEW_NORETURN(input,ACE_InputCDR(mb.data_block()->duplicate(),0,rd_ptr,wr_ptr,static_cast<int>(m_byte_order)));
				if (input)
				{
					// Read in the message info
					MessageHandler::Message* msg = 0;
					ACE_NEW_NORETURN(msg,MessageHandler::Message);
					if (msg)
					{
						msg->m_handle = result.handle();

						// Read the message
						(*input) >> msg->m_dest_channel_id;
						(*input) >> msg->m_dest_thread_id;
						(*input) >> msg->m_src_channel_id;
						(*input) >> msg->m_src_thread_id;

						ACE_CDR::ULong req_dline_secs;
						ACE_CDR::ULong req_dline_usecs;
						(*input) >> req_dline_secs;
						(*input) >> req_dline_usecs;
						msg->m_deadline = ACE_Time_Value(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
						(*input) >> msg->m_attribs;
						input->read_boolean(msg->m_bIsRequest);
						msg->m_pPayload = input;

						#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
							input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
						#endif	

						if (input->good_bit())
						{
							// Push into the HandlerBase queue...
							if (m_pHandler->parse_message(msg))
							{
								// We want to keep msg and input alive
								msg = 0;
								input = 0;
							}
														
							// Start a new read
							bSuccess = read();
						}
						delete msg;
					}
					delete input;
				}
			}
		}
	}

	mb.release();

	if (!bSuccess)
	{
#if defined(ACE_WIN32)
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_IO_PENDING && dwErr != ERROR_SUCCESS && dwErr != WSAENOTSOCK)
#endif
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::MessageConnection::handle_read_stream")));
		
		m_pHandler->handle_closed(result.handle());
		delete this;
	}
}

Root::MessageConnection* Root::MessageHandler::make_handler()
{
	MessageConnection* handler = 0;
	ACE_NEW_RETURN(handler,MessageConnection(this),0);
	return handler;
}

Root::MessageHandler::MessageHandler() :
	m_uNextChannelId(0)
{
}

ACE_CDR::UShort Root::MessageHandler::register_channel(ACE_HANDLE handle)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	ACE_CDR::UShort uChannelId = 0;
	try
	{
		ChannelPair channel = {handle, 0};
		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}
		m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(uChannelId,channel));

		std::map<ACE_CDR::UShort,ACE_CDR::UShort> reverse_map;
		reverse_map.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(0,uChannelId));

		m_mapReverseChannelIds.insert(std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(handle,reverse_map));
	}
	catch (...)
	{
		return 0;
	}

	return uChannelId;
}

ACE_CDR::UShort Root::MessageHandler::add_routing(ACE_CDR::UShort dest_channel, ACE_CDR::UShort dest_route)
{
	// Add a new channel that routes dest_channel's dest_route correctly...

	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	ACE_CDR::UShort uChannelId = 0;
	try
	{
		// Find the handle for dest_channel
		std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(dest_channel);
		
		ChannelPair channel = {i->second.handle, dest_route};
		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}

		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(i->second.handle);
		if (j==m_mapReverseChannelIds.end())
			return 0;

		std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(dest_channel,uChannelId));
		if (!p.second)
			uChannelId = p.first->second;
		else
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(uChannelId,channel));
	}
	catch (...)
	{
		return 0;
	}

	return uChannelId;
}

void Root::MessageHandler::handle_closed(ACE_HANDLE handle)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(handle);
		if (j!=m_mapReverseChannelIds.end())
		{
			for (std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k=j->second.begin();k!=j->second.end();++k)
			{
				m_mapChannelIds.erase(k->second);
			}
			m_mapReverseChannelIds.erase(j);
		}
	}
	catch (...)
	{}
}

bool Root::MessageHandler::parse_message(Message* msg)
{
	ChannelPair dest_channel = {ACE_INVALID_HANDLE, 0};

	// Update the source, so we can send it back the right way...
	try
	{
		bool bFound = false;
		ACE_CDR::UShort reply_channel_id = 0;
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			if (msg->m_dest_channel_id != 0)
			{
				std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(msg->m_dest_channel_id);
				if (i == m_mapChannelIds.end())
					return false;
				
				dest_channel = i->second;
			}

			// Find the local channel id that matches src_channel_id
			std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_handle);
			if (j==m_mapReverseChannelIds.end())
				return false;

			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(msg->m_src_channel_id);
			if (k != j->second.end())
			{
				bFound = true;
				reply_channel_id = k->second;
			}
		}

		if (!bFound)
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			ChannelPair channel = {msg->m_handle, msg->m_src_channel_id};
			reply_channel_id = ++m_uNextChannelId;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = ++m_uNextChannelId;
			}

			std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_handle);
			if (j==m_mapReverseChannelIds.end())
				return false;

			std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,reply_channel_id));
			if (!p.second)
				reply_channel_id = p.first->second;
			else
				m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(reply_channel_id,channel));
		}

		msg->m_src_channel_id = reply_channel_id;
	}
	catch (...)
	{
		return false;
	}

	if (msg->m_dest_channel_id == 0)
	{
		// Find the right queue to send it to...
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.end();
		if (msg->m_dest_thread_id != 0)
		{
			i=m_mapThreadContexts.find(msg->m_dest_thread_id);
		}
		else
		{
			for (int k=0;k<2;++k)
			{
				size_t max = (size_t)-1;
				for (std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator j=m_mapThreadContexts.begin();j!=m_mapThreadContexts.end();++j)
				{
					size_t c = j->second->m_msg_queue->message_count();
					if (c < max)
					{
						if (k==1 || j->second->m_bWaitingOnZero)
						{
							i = j;
							max = c;
						}
					}
				}

				if (i!=m_mapThreadContexts.end())
					break;
			}
		}

		if (i == m_mapThreadContexts.end())
			return false;
		
		return (i->second->m_msg_queue->enqueue_tail(msg,&msg->m_deadline) != -1);
	}
	else
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("Root context: Forwarding request from %u(%u) to %u(%u)"),reply_channel_id,src_channel_id,dest_channel_id,dest_channel.channel));

		// Forward it...
		msg->m_dest_channel_id = dest_channel.channel_id;

		ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
		if (build_header(header,*msg,msg->m_pPayload->start()))
		{
			// Send to the handle
			ACE_Time_Value wait = msg->m_deadline - ACE_OS::gettimeofday();
			size_t sent = 0;
			ACE::send_n(dest_channel.handle,header.begin(),&wait,&sent);
		}

		// We are done with the message...
		return false;
	}
}

Root::MessageHandler::ThreadContext* Root::MessageHandler::ThreadContext::instance(Root::MessageHandler* pHandler)
{
	ThreadContext* pThis = ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>::instance();
	if (pThis->m_thread_id == 0)
	{
		ACE_NEW_NORETURN(pThis->m_msg_queue,(ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>));
		pThis->m_thread_id = pHandler->insert_thread_context(pThis);
		pThis->m_pHandler = pHandler;
	}

	if (pThis->m_thread_id == 0)
		return 0;
	else
		return pThis;
}

Root::MessageHandler::ThreadContext::ThreadContext() :
	m_thread_id(0), 
	m_msg_queue(0),
	m_bWaitingOnZero(false),
	m_deadline(ACE_Time_Value::max_time),
	m_pHandler(0)
{
}

Root::MessageHandler::ThreadContext::~ThreadContext()
{
	if (m_pHandler)
		m_pHandler->remove_thread_context(this);
	delete m_msg_queue;
}

// Accessors for ThreadContext
ACE_CDR::UShort Root::MessageHandler::insert_thread_context(const Root::MessageHandler::ThreadContext* pContext)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	for (ACE_CDR::UShort i=1;;++i)
	{
		if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
		{
			m_mapThreadContexts.insert(std::map<ACE_CDR::UShort,const ThreadContext*>::value_type(i,pContext));
			return i;
		}
	}
}

void Root::MessageHandler::remove_thread_context(const Root::MessageHandler::ThreadContext* pContext)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

bool Root::MessageHandler::wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance(this);
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = pContext->m_msg_queue->dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return false;

		if (msg->m_bIsRequest)
		{
			// Update deadline
			ACE_Time_Value old_deadline = pContext->m_deadline;
			pContext->m_deadline = (msg->m_deadline < pContext->m_deadline ? msg->m_deadline : pContext->m_deadline);

			// Process the message...
			process_request(msg->m_handle,*msg->m_pPayload,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);

			// Restore deadline
			pContext->m_deadline = old_deadline;
		}
		else
		{
			response = msg->m_pPayload;
			delete msg;
			return true;
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

bool Root::MessageHandler::pump_requests(const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance(this);
	bool bWas = pContext->m_bWaitingOnZero;
	pContext->m_bWaitingOnZero = true;

	ACE_InputCDR* response = 0;
	bool bRet = wait_for_response(response,deadline);
	if (response)
		delete response;

	pContext->m_bWaitingOnZero = bWas;

	return bRet;
}

bool Root::MessageHandler::send_request(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_channel_id = dest_channel_id;
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = 0;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_bIsRequest = true;
	msg.m_deadline = pContext->m_deadline;
	ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;	

	ChannelPair dest_channel = {ACE_INVALID_HANDLE, 0};
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return false;
		
		dest_channel = i->second;
	}
	catch (...)
	{
		return false;
	}

	// Remap the destination channel...
	msg.m_dest_channel_id = dest_channel.channel_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,msg,mb))
		return false;
	
	// Check the timeout
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (msg.m_deadline <= now)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	ACE_Time_Value wait = msg.m_deadline - now;
	bool bRet = false;
	size_t sent = 0;
	ssize_t res = ACE::send_n(dest_channel.handle,header.begin(),&wait,&sent);
	if (res != -1 && sent == header.total_length())
	{
		if (attribs & 1)
			bRet = true;
		else
			// Wait for response...
			bRet = wait_for_response(response,&msg.m_deadline);
	}
		
	return bRet;
}

bool Root::MessageHandler::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_channel_id = dest_channel_id;
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = 0;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_bIsRequest = false;
	msg.m_deadline = pContext->m_deadline;
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;	

	ChannelPair dest_channel = {ACE_INVALID_HANDLE, 0};
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return false;
		
		dest_channel = i->second;
	}
	catch (...)
	{
		return false;
	}

	// Remap the destination channel...
	msg.m_dest_channel_id = dest_channel.channel_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,msg,mb))
		return false;
	
	// Check the timeout
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (msg.m_deadline <= now)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	ACE_Time_Value wait = msg.m_deadline - now;
	size_t sent = 0;
	int res = ACE::send_n(dest_channel.handle,header.begin(),&wait,&sent);
	if (res == -1 || sent != header.total_length())
		return false;
		
	return true;
}

static bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
{
#if ACE_MAJOR_VERSION <= 5 && ACE_MINOR_VERSION <= 5 && ACE_BETA_VERSION == 0

    ACE_CDR::Long len = static_cast<ACE_CDR::Long>(stream.total_length());

#if !defined (ACE_ENABLE_SWAP_ON_WRITE)
    *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
#else
    if (!stream.do_byte_swap())
        *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
    else
        ACE_CDR::swap_4(reinterpret_cast<const char*>(len),msg_len_point);
#endif

    return true;
#else
    return stream.replace(static_cast<ACE_CDR::Long>(stream.total_length()),msg_len_point);
#endif
}

bool Root::MessageHandler::build_header(ACE_OutputCDR& header, const Message& msg, const ACE_Message_Block* mb)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
		return false;
	
	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return false;

	// We have room for 2 bytes here!

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write the message
	header.write_ushort(msg.m_dest_channel_id);
	header.write_ushort(msg.m_dest_thread_id);
	header.write_ushort(msg.m_src_channel_id);
	header.write_ushort(msg.m_src_thread_id);
		
	header.write_ulong(static_cast<const timeval*>(msg.m_deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(msg.m_deadline)->tv_usec);

	header.write_ushort(msg.m_attribs);

	header.write_boolean(msg.m_bIsRequest);
	
	if (!header.good_bit())
		return false;

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	return true;
}
