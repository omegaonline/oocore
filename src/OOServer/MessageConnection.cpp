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

ACE_TString Root::MessagePipe::unique_name(const ACE_TString& strPrefix)
{
	ACE_Time_Value t = ACE_OS::gettimeofday();

	ACE_TCHAR szBuf[32];
	ACE_OS::snprintf(szBuf,32,ACE_TEXT("%lx%lx"),t.sec(),t.usec());

	return strPrefix + szBuf;
}

Root::MessageConnection::MessageConnection(MessageHandler* pHandler)  :
	m_pHandler(pHandler),
	m_channel_id(0)
{
}

Root::MessageConnection::~MessageConnection()
{
	m_pipe->close();
}

ACE_CDR::ULong Root::MessageConnection::open(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, ACE_CDR::ULong channel_id, bool bStart)
{
	if (m_channel_id)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"already open!"),0);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (m_reader.open(*this,pipe->get_read_handle()) != 0 && GetLastError() != ERROR_MORE_DATA)
#else
	if (m_reader.open(*this,pipe->get_read_handle()) != 0)
#endif
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"reader.open() failed"),0);
	}

	m_channel_id = m_pHandler->register_channel(pipe,channel_id);
	if (m_channel_id == 0)
		return 0;

	if (bStart && !read())
		return 0;

	m_pipe = pipe;

	return m_channel_id;
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
		ACE_ERROR((LM_ERROR,L"%N:%l: read failed, code: %#x\n",errno));
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
			if (result.bytes_transferred() < s_initial_read)
			{
				if (m_reader.read(mb,s_initial_read - result.bytes_transferred()) == 0)
					return;
				else
					ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"reader.read() failed"));
			}
			else if (result.bytes_transferred() == s_initial_read)
			{
				ACE_InputCDR input(&mb);
				
				// Read the length
				input >> m_read_len;
				if (input.good_bit())
				{
					// Resize the message block
					if (mb.size(m_read_len) == 0)
					{
						// Subtract what we have already read
						m_read_len -= static_cast<ACE_CDR::ULong>(mb.length());

						if (m_reader.read(mb,m_read_len) == 0)
							return;
						else
							ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"reader.read() failed"));
					}
					else
						ACE_ERROR((LM_ERROR,L"%N:%l: Packet too big\n"));
				}
				else
					ACE_ERROR((LM_ERROR,L"%N:%l: Corrupt header\n"));
			}
			else
				ACE_ERROR((LM_ERROR,L"%N:%l: Over-read?\n"));
		}
		else
		{
			if (result.bytes_transferred() < m_read_len)
			{
				m_read_len -= result.bytes_transferred();
				if (m_reader.read(mb,m_read_len) == 0)
					return;
				else
					ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"reader.read() failed"));
			}
			else if (result.bytes_transferred() == m_read_len)
			{
				if (m_pHandler->parse_message(&mb))
				{
					// Start a new read
					bSuccess = read();
				}
			}
			else
				ACE_ERROR((LM_ERROR,L"%N:%l: Over-read?\n"));
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
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"handle_read_*() failed"));
		}

		m_pHandler->pipe_closed(m_channel_id,0);
		m_pipe->close();
		delete this;
	}
}

Root::MessageHandler::MessageHandler() :
	m_uChannelId(0),
	m_uChannelMask(0),
	m_uChildMask(0),
	m_uUpstreamChannel(0),
	m_uNextChannelId(0),
	m_uNextChannelMask(0),
	m_uNextChannelShift(0)
{
}

void Root::MessageHandler::set_channel(ACE_CDR::ULong channel_id, ACE_CDR::ULong mask, ACE_CDR::ULong child_mask, ACE_CDR::ULong upstream_id)
{
	assert(!(channel_id & ~mask));
	assert(!(channel_id & child_mask));
	
	m_uChannelId = channel_id;
	m_uChannelMask = mask;
	m_uChildMask = child_mask;
	m_uUpstreamChannel = upstream_id;

	// Turn child mask into a shift...
	m_uNextChannelMask = m_uChildMask;
	for (m_uNextChannelShift = 0;!(m_uNextChannelMask & 1);++m_uNextChannelShift)
	{
		m_uNextChannelMask >>= 1;
	}
}

ACE_CDR::ULong Root::MessageHandler::register_channel(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, ACE_CDR::ULong channel_id)
{
	try
	{
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

			ACE_Thread_Mutex* pLock = 0;
			ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
			ChannelInfo channel(pipe,pLock);

			if (channel_id != 0)
			{
				if (m_mapChannelIds.find(channel_id)!=m_mapChannelIds.end())
					ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: Duplicate fixed channel registered\n"),0);
			}
			else if (m_mapChannelIds.size() >= m_uNextChannelMask - 0xF)
			{
				ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: Out of free channels\n"),0);
			}
			else
			{
				do
				{
					channel_id = m_uChannelId | ((++m_uNextChannelId & m_uNextChannelMask) << m_uNextChannelShift);
				} while (channel_id==m_uChannelId || m_mapChannelIds.find(channel_id)!=m_mapChannelIds.end());
			}

			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ChannelInfo>::value_type(channel_id,channel));
		}

		if (!channel_open(channel_id))
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

			m_mapChannelIds.erase(channel_id);
			return 0;
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),0);
	}
	
	return channel_id;
}

bool Root::MessageHandler::can_route(ACE_CDR::ULong, ACE_CDR::ULong)
{
	// Do nothing, used in derived classes
	return true;
}

bool Root::MessageHandler::channel_open(ACE_CDR::ULong)
{
	// Do nothing, used in derived classes
	return true;
}

void Root::MessageHandler::pipe_closed(ACE_CDR::ULong channel_id, ACE_CDR::ULong src_channel_id)
{
	// Inform derived classes that the pipe has gone...
	channel_closed(channel_id);	

	// Propogate the message to all user processes... 
	try
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
		if (guard.locked())
		{
			for (std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
			{
				// Always route upstream, and/or follow routing rules
				if (i->first != channel_id && i->first != src_channel_id &&
					(i->first == m_uUpstreamChannel || can_route(channel_id & (m_uChannelMask | m_uChildMask),i->first)))
				{
					send_channel_close(i->first,channel_id);
				}
			}
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
	}
	
	try
	{
		// Remove the pipe
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		m_mapChannelIds.erase(channel_id);
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
	}
}

ACE_CDR::UShort Root::MessageHandler::classify_channel(ACE_CDR::ULong channel_id)
{
	// The response codes must match Remoting::MarshalFlags values
	if ((channel_id & m_uChannelMask) == m_uChannelId)
	{
		if (!(channel_id & m_uChildMask))
		{
			ACE_CDR::UShort apartment_id = static_cast<ACE_CDR::UShort>(channel_id & ~(m_uChannelMask | m_uChildMask));
			if (apartment_id == 0)
				return 0; // same
			else
				return 1; // apartment
		}
		else
			return 2; // inter_process;
	}
	else if (channel_id & 0x80000000)
		return 3; // inter_user;
	else
		return 4; // another_machine
}

bool Root::MessageHandler::parse_message(const ACE_Message_Block* mb)
{
	ACE_InputCDR input(mb);

	// Skip the length
	ACE_CDR::ULong read_len;
	input >> read_len;

	// Read the destination
	ACE_CDR::ULong dest_channel_id = 0;
	input >> dest_channel_id;

	// Read the source
	ACE_CDR::ULong src_channel_id = 0;
	input >> src_channel_id;

	// Read the deadline
	ACE_CDR::ULongLong req_dline_secs;
	input >> req_dline_secs;
	ACE_CDR::Long req_dline_usecs;
	input >> req_dline_usecs;
	ACE_Time_Value deadline = ACE_Time_Value(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));

	// Did everything make sense?
	if (!input.good_bit())
		return false;

	// Check the destination
	bool bRoute = true;
	ACE_CDR::UShort dest_apartment_id = 0;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		if (!(dest_channel_id & m_uChildMask))
		{
			dest_apartment_id = static_cast<ACE_CDR::UShort>(dest_channel_id & ~(m_uChannelMask | m_uChildMask));
			bRoute = false;
		}
		else
		{
			// See if we are routing from a local source
			if ((src_channel_id & m_uChannelMask) == m_uChannelId)
			{
				// Check we can route from src to dest
				if (!can_route(src_channel_id & (m_uChannelMask | m_uChildMask),dest_channel_id & (m_uChannelMask | m_uChildMask)))
					return false;
			}

			// Clear off sub channel bits
			dest_channel_id &= (m_uChannelMask | m_uChildMask);
		}
	}
	else
	{
		// Send upstream
		dest_channel_id = m_uUpstreamChannel;
	}
	
	if (!bRoute)
	{
		// If its our message, process it
		
		// Read in the message info
		MessageHandler::Message* msg = 0;
		ACE_NEW_RETURN(msg,MessageHandler::Message,false);
		
		msg->m_deadline = deadline;
		msg->m_src_channel_id = src_channel_id;

		// Read the rest of the message
		input >> msg->m_dest_thread_id;
		input >> msg->m_src_thread_id;
		input >> msg->m_attribs;

		// Did everything make sense?
		if (!input.good_bit())
		{
			delete msg;
			return false;
		}

		// Now validate the apartment...
		if (dest_apartment_id)
		{
			if (msg->m_dest_thread_id != 0 && msg->m_dest_thread_id != dest_apartment_id)
			{
				// Destination thread is not the destination apartment!
				delete msg;
				return false;
			}

			// Route to the correct thread
			msg->m_dest_thread_id = dest_apartment_id;
		}
		
		input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
		
		ACE_NEW_NORETURN(msg->m_ptrPayload,ACE_InputCDR(input));
		if (msg->m_ptrPayload.null())
		{
			// We are running out of memory!
			delete msg;
			return false;
		}

		if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			try
			{
				std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
				if (i == m_mapThreadContexts.end())
				{
					// Bad destination thread, or thread has died...
					delete msg;
					return true;
				}
				else if (i->second->m_msg_queue->enqueue_tail(msg,msg->m_deadline==ACE_Time_Value::max_time ? 0 : &msg->m_deadline) == -1)
				{
					delete msg;
					return true;
				}
			}
			catch (std::exception& e)
			{
				delete msg;
				ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),false);
			}
		}
		else
		{
			void* TODO; // Check for timeout on high water mark and spawn more threads

			if (m_default_msg_queue.enqueue_tail(msg,msg->m_deadline==ACE_Time_Value::max_time ? 0 : &msg->m_deadline) == -1)
			{
				delete msg;
				return true;
			}
		}

		return true;
	}

	// Find the correct channel
	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return false;

		dest_channel = i->second;
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),0);
	}
		
	// Send on...
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

	ACE_Time_Value wait = deadline;
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
		{
			ACE_OS::last_error(ETIMEDOUT);
			return false;
		}

		wait = deadline - now;
	}

	return (dest_channel.pipe->send(mb,wait != ACE_Time_Value::max_time ? &wait : 0) == static_cast<ssize_t>(mb->total_length()));
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
	m_pHandler(0),
	m_usage_count(0),
	m_deadline(ACE_Time_Value::max_time),
	m_seq_no(0)
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

	try
	{
		for (ACE_CDR::UShort i=1;i<=0xFFF;++i)
		{
			if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
			{
				m_mapThreadContexts.insert(std::map<ACE_CDR::UShort,const ThreadContext*>::value_type(i,pContext));
				return i;
			}
		}

		return 0;
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),0);
	}
}

void Root::MessageHandler::remove_thread_context(const Root::MessageHandler::ThreadContext* pContext)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

void Root::MessageHandler::close()
{
	// Close all the pipes...
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
		
		try
		{
			for (std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
			{
				i->second.pipe->close();
			}
		}
		catch (std::exception& e)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
		}
	}

	// Now spin, waiting for all the channels to close...
	ACE_Time_Value wait(0,100);
	for (size_t i=0;i<300;++i)
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		if (m_mapChannelIds.empty())
			break;

		ACE_OS::sleep(wait);
	}
}

void Root::MessageHandler::stop()
{
	ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		for (std::map<ACE_CDR::UShort,const ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		{
			if (i->second->m_usage_count)
				i->second->m_msg_queue->pulse();
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
	}
	
	m_default_msg_queue.close();
}

bool Root::MessageHandler::wait_for_response(ACE_InputCDR*& response, ACE_CDR::ULong seq_no, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id)
{
	bool bRet = false;

	ThreadContext* pContext = ThreadContext::instance(this);

	// Up the usage count on the context
	++pContext->m_usage_count;

	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		{
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
				break;
			
			std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.find(from_channel_id);
			if (i == m_mapChannelIds.end())
			{
				// Channel has gone!
				ACE_OS::last_error(ECONNRESET);
				break;
			}
		}

		// Get the next message
		Message* msg = 0;
		if (pContext->m_msg_queue->dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline)) == -1)
		{
			if (pContext->m_msg_queue->state() != ACE_Message_Queue_Base::PULSED)
				break;
		}
		else
		{
			// Read the payload specific data
			ACE_CDR::Octet byte_order;
			msg->m_ptrPayload->read_octet(byte_order);
			ACE_CDR::Octet version = 0;
			msg->m_ptrPayload->read_octet(version);

			msg->m_ptrPayload->reset_byte_order(byte_order);
		
			ACE_CDR::UShort type = 0;
			(*msg->m_ptrPayload) >> type;

			ACE_CDR::ULong recv_seq_no = 0;
			(*msg->m_ptrPayload) >> recv_seq_no;

			msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
			
			if (msg->m_ptrPayload->good_bit() && version == 1)
			{
				if (type == Message::Request)
				{
					// Update deadline
					ACE_Time_Value old_deadline = pContext->m_deadline;
					pContext->m_deadline = msg->m_deadline;
					if (deadline && *deadline < pContext->m_deadline)
						pContext->m_deadline = *deadline;
					
					// Set per channel thread id
					std::map<ACE_CDR::ULong,ACE_CDR::UShort>::iterator i;
					try
					{
						i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
						if (i == pContext->m_mapChannelThreads.end())
							i = pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,0)).first;
					}
					catch (std::exception& e)
					{
						// This shouldn't ever occur, but that means it will ;)
						ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
						pContext->m_deadline = old_deadline;
						delete msg;
						continue;
					}
									
					ACE_CDR::UShort old_thread_id = i->second;
					i->second = msg->m_src_thread_id;

					// Process the message...
					process_request(*msg->m_ptrPayload,recv_seq_no,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);
					
					// Restore old per channel thread id
					i->second = old_thread_id;
									
					pContext->m_deadline = old_deadline;
				}
				else if (recv_seq_no == seq_no)
				{
					ACE_NEW_NORETURN(response,ACE_InputCDR(*msg->m_ptrPayload));
					if (!response)
						ACE_ERROR((LM_ERROR,L"%N:%l: Out of memory\n"));
					else
						bRet = true;

					delete msg;
					break;
				}
				else
				{
					ACE_ERROR((LM_ERROR,L"%N:%l: Duff message\n"));
				}
			}

			delete msg;
		}
	}

	// Dec the usage count on the context
	--pContext->m_usage_count;

	return bRet;
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

		// Read the payload specific data
		ACE_CDR::Octet byte_order;
		msg->m_ptrPayload->read_octet(byte_order);
		ACE_CDR::Octet version = 0;
		msg->m_ptrPayload->read_octet(version);

		msg->m_ptrPayload->reset_byte_order(byte_order);
	
		ACE_CDR::UShort type = 0;
		(*msg->m_ptrPayload) >> type;

		ACE_CDR::ULong seq_no = 0;
		(*msg->m_ptrPayload) >> seq_no;

		msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
		
		if (msg->m_ptrPayload->good_bit() && version == 1 && type == Message::Request)
		{
			if (msg->m_attribs & Message::system_message)
			{
				if (msg->m_attribs & Message::channel_close)
					process_channel_close(msg);
			}
			else
			{
				// Set deadline
				ACE_Time_Value old_deadline = pContext->m_deadline;
				pContext->m_deadline = msg->m_deadline;
				if (deadline && *deadline < pContext->m_deadline)
					pContext->m_deadline = *deadline;

				try
				{
					// Set per channel thread id
					pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
				}
				catch (std::exception& e)
				{
					// This shouldn't ever occur, but that means it will ;)
					ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
					pContext->m_deadline = old_deadline;
					delete msg;
					continue;
				}
												
				// Process the message...
				process_request(*msg->m_ptrPayload,seq_no,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);

				// Clear the channel/threads map
				pContext->m_mapChannelThreads.clear();

				// Reset deadline
				pContext->m_deadline = old_deadline;
			}
		}

		delete msg;
	}
}

void Root::MessageHandler::process_channel_close(Message* msg)
{
	ACE_CDR::ULong closed_channel_id;
	(*msg->m_ptrPayload) >> closed_channel_id;
	
	if (!msg->m_ptrPayload->good_bit())
		return;
	
	// Close the pipe
	pipe_closed(closed_channel_id,msg->m_src_channel_id);

	try
	{
		// Unblock all waiting threads
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::UShort,const ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		{
			if (i->second->m_usage_count)
				i->second->m_msg_queue->pulse();
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
	}
}

bool Root::MessageHandler::send_channel_close(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong closed_channel_id)
{
	ACE_OutputCDR msg;
	msg << closed_channel_id;
	if (!msg.good_bit())
		return false;

	ACE_InputCDR* null;
	return send_request(dest_channel_id,msg.begin(),null,0,Message::asynchronous | Message::system_message | Message::channel_close);
}

bool Root::MessageHandler::send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::ULong attribs)
{
	// Build a header
	Message msg;
	msg.m_dest_thread_id = 0;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = 0;
	msg.m_deadline = ACE_Time_Value::max_time;
	msg.m_attribs = attribs;

	ACE_CDR::ULong seq_no = 0;

	// Only use thread context if we are not a system message (they can come from odd threads)
	if (!(attribs & Message::system_message))
	{
		ThreadContext* pContext = ThreadContext::instance(this);

		try
		{
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(dest_channel_id);
			if (i != pContext->m_mapChannelThreads.end())
				msg.m_dest_thread_id = i->second;
		}
		catch (std::exception& e)
		{
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),false);
		}

		msg.m_src_thread_id = pContext->m_thread_id;
		msg.m_deadline = pContext->m_deadline;

		if (!(attribs & Message::asynchronous))
		{
			while (!seq_no)
			{
				seq_no = ++pContext->m_seq_no;
			}
		}
	}
		
	if (timeout > 0)
	{
		ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
		if (deadline < msg.m_deadline)
			msg.m_deadline = deadline;
	}

	// Find the destination channel
	ACE_CDR::ULong actual_dest_channel_id = dest_channel_id;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}
	else
	{
		// Send upstream
		actual_dest_channel_id = m_uUpstreamChannel;
	}

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.find(actual_dest_channel_id);
		if (i == m_mapChannelIds.end())
		{
			ACE_OS::last_error(ENOENT);
			return false;
		}

		dest_channel = i->second;
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()),false);
	}
	
	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,Message::Request,seq_no,dest_channel_id,msg,mb))
		return false;

	// Send to the handle
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

		// Check the timeout
		ACE_Time_Value wait = msg.m_deadline;
		if (msg.m_deadline != ACE_Time_Value::max_time)
		{
			ACE_Time_Value now = ACE_OS::gettimeofday();
			
			if (msg.m_deadline <= now)
			{
				ACE_OS::last_error(ETIMEDOUT);
				return false;
			}

			wait = msg.m_deadline - now;
		}

		size_t sent = 0;
		if (dest_channel.pipe->send(header.begin(),wait != ACE_Time_Value::max_time ? &wait : 0,&sent) == -1)
			return false;

		if (sent != header.total_length())
			return false;
	}

	if (attribs & Message::asynchronous)
		return true;
	else
		// Wait for response...
		return wait_for_response(response,seq_no,msg.m_deadline != ACE_Time_Value::max_time ? &msg.m_deadline : 0,actual_dest_channel_id);
}

void Root::MessageHandler::send_response(ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_deadline = pContext->m_deadline;
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;

	// Find the destination channel
	ACE_CDR::ULong actual_dest_channel_id = dest_channel_id;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}
	else
	{
		// Send upstream
		actual_dest_channel_id = m_uUpstreamChannel;
	}

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::ULong,ChannelInfo>::iterator i=m_mapChannelIds.find(actual_dest_channel_id);
		if (i == m_mapChannelIds.end())
			return;

		dest_channel = i->second;
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: std::exception thrown %C\n",e.what()));
		return;
	}
	
	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,Message::Response,seq_no,dest_channel_id,msg,mb))
		return;

	ACE_GUARD(ACE_Thread_Mutex,guard,*dest_channel.lock);

	// Check the timeout
	ACE_Time_Value wait = msg.m_deadline;
	if (msg.m_deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();

		if (msg.m_deadline <= now)
		{
			ACE_OS::last_error(ETIMEDOUT);
			return;
		}

		wait = msg.m_deadline - now;
	}

	dest_channel.pipe->send(header.begin(),wait != ACE_Time_Value::max_time ? &wait : 0);
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

bool Root::MessageHandler::build_header(ACE_OutputCDR& header, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, const Message& msg, const ACE_Message_Block* mb)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: Message too big!\n"),false);

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	header << dest_channel_id;
	header << msg.m_src_channel_id;

	header.write_ulonglong(msg.m_deadline.sec());
	header.write_long(msg.m_deadline.usec());

	header << msg.m_dest_thread_id;
	header << msg.m_src_thread_id;
	header << msg.m_attribs;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	 // version
	header << flags;
	header << seq_no;
	
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"CDR write() failed"),false);
	
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"CDR write() failed"),false);

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"CDR replace() failed"),false);

	return true;
}
