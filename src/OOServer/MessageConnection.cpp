///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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

ACE_WString Root::MessagePipe::unique_name(const ACE_WString& strPrefix)
{
	ACE_Time_Value t = ACE_OS::gettimeofday();

	wchar_t szBuf[32];
	ACE_OS::sprintf(szBuf,L"%lx%lx",t.sec(),t.usec());

	return strPrefix + szBuf;
}

Root::MessageConnection::~MessageConnection()
{
	m_pipe.close();
}

ACE_CDR::UShort Root::MessageConnection::open(MessagePipe& pipe)
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (m_reader.open(*this,pipe.get_read_handle()) != 0 && GetLastError() != ERROR_MORE_DATA)
#else
	if (m_reader.open(*this,pipe.get_read_handle()) != 0)
#endif
	{
	    ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"reader.open() failed"),0);
	}

	ACE_CDR::UShort uId = m_pHandler->register_channel(pipe);
	if (uId == 0)
		return 0;

	if (!read())
		return 0;

	m_pipe = pipe;

	return uId;
}

bool Root::MessageConnection::read()
{
	ACE_Message_Block* mb = 0;
	ACE_NEW_RETURN(mb,ACE_Message_Block(2048),false);

	// Align the message block for CDR
	ACE_CDR::mb_align(mb);

	// We read the header first
	m_read_len = 0;

	// Start an async read
	if (m_reader.read(*mb,s_initial_read) != 0)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"read() failed"));
		mb->release();
		return false;
	}

	return true;
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
void Root::MessageConnection::handle_read_file(const ACE_Asynch_Read_File::Result& result)
#else
void Root::MessageConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
#endif
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = false;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (result.success() || result.error() == ERROR_MORE_DATA)
#else
	if (result.success())
#endif
	{
		if (m_read_len == 0)
		{
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

							if (m_reader.read(mb,m_read_len) == 0)
								return;
							else
								ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"reader.read() failed"));
						}
					}
					else
						ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));
				}
				else
					ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));
			}
		}
		else
		{
			if (result.bytes_transferred() < m_read_len)
			{
				m_read_len -= result.bytes_transferred();
				if (m_reader.read(mb,m_read_len) == 0)
					return;
			}
			else if (result.bytes_transferred() == m_read_len)
			{
				// Create a new input CDR wrapping mb
				ACE_InputCDR* input = 0;
				ACE_NEW_NORETURN(input,ACE_InputCDR(&mb,m_byte_order));
				if (input)
				{
					// Read in the message info
					MessageHandler::Message* msg = 0;
					ACE_NEW_NORETURN(msg,MessageHandler::Message);
					if (msg)
					{
						msg->m_pipe = m_pipe;

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
						else
							ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));

						delete msg;
					}
					delete input;
				}
			}
			else
				ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Over-read?\n"));
		}
	}

	mb.release();

	if (!bSuccess)
	{
		int err = ACE_OS::last_error();
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		if (err != 0 && err != ERROR_BROKEN_PIPE)
#else
		if (err != 0 && err != ENOTSOCK)
#endif
		{
			ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"handle_read_*() failed"));
		}

		m_pHandler->pipe_closed(m_pipe);
		delete this;
	}
}

Root::MessageHandler::MessageHandler() :
	m_uNextChannelId(0)
{
}

Root::MessageConnection* Root::MessageHandler::make_handler()
{
	MessageConnection* handler = 0;
	ACE_NEW_RETURN(handler,MessageConnection(this),0);
	return handler;
}

int Root::MessageHandler::on_accept(MessagePipe& pipe, int key)
{
	if (key != 0)
		return -1;

	Root::MessageConnection* pMC = 0;
	ACE_NEW_RETURN(pMC,Root::MessageConnection(this),-1);

	if (pMC->open(pipe) == 0)
	{
		delete pMC;
		return -1;
	}

	return 0;
}

ACE_CDR::UShort Root::MessageHandler::register_channel(MessagePipe& pipe)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	ACE_CDR::UShort uChannelId = 0;
	try
	{
		ACE_Thread_Mutex* pLock = 0;
		ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
		ChannelInfo channel(pipe,0,pLock);

		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}
		m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(uChannelId,channel));

		std::map<ACE_CDR::UShort,ACE_CDR::UShort> reverse_map;
		reverse_map.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(0,uChannelId));

		m_mapReverseChannelIds.insert(std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(pipe,reverse_map));
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
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
		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel);
		if (i==m_mapChannelIds.end())
			return 0;

		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(i->second.pipe);
		if (j==m_mapReverseChannelIds.end())
			return 0;

		ACE_Thread_Mutex* pLock = 0;
		ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
		ChannelInfo channel(i->second.pipe, dest_route, pLock);

		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}

		std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(dest_channel,uChannelId));
		if (!p.second)
			uChannelId = p.first->second;
		else
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(uChannelId,channel));
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
	}

	return uChannelId;
}

ACE_CDR::UShort Root::MessageHandler::get_pipe_channel(const MessagePipe& pipe, ACE_CDR::UShort channel)
{
	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	try
	{
		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::const_iterator j=m_mapReverseChannelIds.find(pipe);
		if (j!=m_mapReverseChannelIds.end())
		{
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::const_iterator i=j->second.find(channel);
			if (i!=j->second.end())
				return i->second;
		}
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
	}

	return 0;
}

Root::MessagePipe Root::MessageHandler::get_channel_pipe(ACE_CDR::UShort channel)
{
	static Root::MessagePipe ret;

	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,ret);

	try
	{
		std::map<ACE_CDR::UShort,ChannelInfo>::const_iterator i=m_mapChannelIds.find(channel);
		if (i != m_mapChannelIds.end())
			return i->second.pipe;
	}
	catch (...)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"));
	}

	return ret;
}

void Root::MessageHandler::pipe_closed(const MessagePipe& pipe)
{
	try
	{
		std::list<ACE_CDR::UShort> listClosed;
		{
			ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(pipe);
			if (j != m_mapReverseChannelIds.end())
			{
				for (std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k=j->second.begin();k!=j->second.end();++k)
				{
					listClosed.push_back(k->second);

					m_mapChannelIds.erase(k->second);
				}	
				m_mapReverseChannelIds.erase(j);
			}
		}

		for (std::list<ACE_CDR::UShort>::iterator i=listClosed.begin();i!=listClosed.end();++i)
		{
			channel_closed(*i);
		}
	}
	catch (...)
	{}
}

void Root::MessageHandler::channel_closed(ACE_CDR::UShort channel)
{
	void* TODO; // Propogate the channel close
}

bool Root::MessageHandler::parse_message(Message* msg)
{
	ChannelInfo dest_channel;

	// Update the source, so we can send it back the right way...
	try
	{
		bool bFound = false;
		ACE_CDR::UShort reply_channel_id = 0;
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			if (msg->m_dest_channel_id != 0)
			{
				std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(msg->m_dest_channel_id);
				if (i == m_mapChannelIds.end())
					return false;

				dest_channel = i->second;
			}

			// Find the local channel id that matches src_channel_id
			std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_pipe);
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

			std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_pipe);
			if (j==m_mapReverseChannelIds.end())
				return false;

			ACE_Thread_Mutex* pLock = 0;
			ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
			ChannelInfo channel(msg->m_pipe, msg->m_src_channel_id, pLock);

			reply_channel_id = ++m_uNextChannelId;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = ++m_uNextChannelId;
			}

			std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,reply_channel_id));
			if (!p.second)
				reply_channel_id = p.first->second;
			else
				m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(reply_channel_id,channel));
		}

		msg->m_src_channel_id = reply_channel_id;
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),false);
	}

	if (msg->m_dest_channel_id == 0)
	{
		if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
			if (i == m_mapThreadContexts.end())
				return false;

			return (i->second->m_msg_queue->enqueue_tail(msg,&msg->m_deadline) != -1);
		}
		else
		{
			void* TODO; // Spawn off more threads if we go over a limit??
			return (m_default_msg_queue.enqueue_tail(msg,&msg->m_deadline) != -1);
		}
	}
	else
	{
		// Forward it...
		msg->m_dest_channel_id = dest_channel.channel_id;

		ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
		if (build_header(header,*msg,msg->m_pPayload->start()))
		{
            // Send to the handle
			ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

			ACE_Time_Value wait = msg->m_deadline - ACE_OS::gettimeofday();
			dest_channel.pipe.send(header.begin(),&wait);
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
	m_pHandler(0)
{
	m_context.m_deadline = ACE_Time_Value::max_time;
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

int Root::MessageHandler::start(const ACE_WString& strName)
{
	return m_connector.start(this,0,strName);
}

void Root::MessageHandler::stop_accepting()
{
	m_connector.stop();
}

void Root::MessageHandler::stop()
{
	ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	for (std::map<ACE_CDR::UShort,const ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
	{
		i->second->m_msg_queue->close();
	}

	m_default_msg_queue.close();
}

const Root::MessageHandler::CallContext& Root::MessageHandler::get_call_context()
{
	return ThreadContext::instance(this)->m_context;
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
			// Update context
			CallContext old_context = pContext->m_context;
			pContext->m_context.m_src_channel = msg->m_src_channel_id;
			pContext->m_context.m_deadline = (msg->m_deadline < pContext->m_context.m_deadline ? msg->m_deadline : pContext->m_context.m_deadline);
			pContext->m_context.m_attribs = msg->m_attribs;

			// Set per channel thread id
			ACE_CDR::UShort old_thread_id = 0;
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
			if (i != pContext->m_mapChannelThreads.end())
				i = pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,0)).first;

			old_thread_id = i->second;
			i->second = msg->m_src_thread_id;

			// Process the message...
			process_request(msg->m_pipe,*msg->m_pPayload,msg->m_src_thread_id,pContext->m_context);

			// Restore old per channel thread id
			i->second = old_thread_id;

			// Restore old context
			pContext->m_context = old_context;
		}
		else
		{
			response = msg->m_pPayload;
			msg->m_pPayload = 0;
			delete msg;
			return true;
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

void Root::MessageHandler::pump_requests(const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance(this);
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = m_default_msg_queue.dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			break;

		if (msg->m_bIsRequest)
		{
			// Set context
			pContext->m_context.m_src_channel = msg->m_src_channel_id;
			pContext->m_context.m_attribs = msg->m_attribs;
			pContext->m_context.m_deadline = msg->m_deadline;
			if (deadline && *deadline < pContext->m_context.m_deadline)
				pContext->m_context.m_deadline = *deadline;

			// Set per channel thread id
			pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
			
			// Process the message...
			process_request(msg->m_pipe,*msg->m_pPayload,msg->m_src_thread_id,pContext->m_context);

			// Clear the channel/threads map
			pContext->m_mapChannelThreads.clear();
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

bool Root::MessageHandler::send_request(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_channel_id = dest_channel_id;
	msg.m_dest_thread_id = 0;
	std::map<ACE_CDR::UShort,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(pContext->m_thread_id);
	if (i != pContext->m_mapChannelThreads.end())
		msg.m_dest_thread_id = i->second;

	msg.m_src_channel_id = 0;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_bIsRequest = true;
	msg.m_deadline = pContext->m_context.m_deadline;
	ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
		{
			ACE_OS::last_error(ENOENT);
			return false;
		}

		dest_channel = i->second;
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),false);
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
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

		size_t sent = 0;
		ACE_Time_Value wait = msg.m_deadline - now;
		if (dest_channel.pipe.send(header.begin(),&wait,&sent) == -1)
			return false;

		if (sent != header.total_length())
			return false;
	}

	if (attribs & 1)
		return true;
	else
		// Wait for response...
		return wait_for_response(response,&msg.m_deadline);
}

void Root::MessageHandler::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs)
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
	msg.m_deadline = pContext->m_context.m_deadline;
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return;

		dest_channel = i->second;
	}
	catch (...)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"));
		return;
	}

	// Remap the destination channel...
	msg.m_dest_channel_id = dest_channel.channel_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,msg,mb))
		return;

	// Check the timeout
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (msg.m_deadline <= now)
		return;

	ACE_GUARD(ACE_Thread_Mutex,guard,*dest_channel.lock);

	ACE_Time_Value wait = msg.m_deadline - now;
	dest_channel.pipe.send(header.begin(),&wait);
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
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] Message too big!\n"),false);

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

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
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR replace() failed"),false);

	return true;
}
