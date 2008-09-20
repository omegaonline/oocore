///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

ACE_CString Root::MessagePipe::unique_name(const ACE_CString& strPrefix)
{
	ACE_Time_Value t = ACE_OS::gettimeofday();

	char szBuf[64];
	ACE_OS::snprintf(szBuf,63,"%lx%llx",ACE_OS::getpid(),t.usec());

#if defined(ACE_WIN32)
    // ACE Adds the "\\.\"...
	return strPrefix + szBuf;
#else

    if (ACE_OS::mkdir("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return "";
	}

    // Try to make it public
	chmod("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO);

    return "/tmp/omegaonline/" + strPrefix + szBuf;
#endif
}

Root::MessageConnection::MessageConnection(MessageHandler* pHandler)  :
	m_pHandler(pHandler),
	m_chunk_size(256),
	m_channel_id(0)
{
}

Root::MessageConnection::~MessageConnection()
{
	m_pipe->close();
}

ACE_CDR::ULong Root::MessageConnection::open(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, ACE_CDR::ULong channel_id, bool bStart)
{
	if (m_channel_id)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Already open!")),0);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (m_reader.open(*this,pipe->get_read_handle()) != 0 && GetLastError() != ERROR_MORE_DATA)
#else
	if (m_reader.open(*this,pipe->get_read_handle()) != 0)
#endif
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("reader.open() failed")),0);
	}

	m_channel_id = m_pHandler->register_channel(pipe,channel_id);
	if (m_channel_id == 0)
		return 0;

	if (bStart && !read())
		return 0;

	m_pipe = pipe;

	return m_channel_id;
}

bool Root::MessageConnection::read(ACE_Message_Block* mb)
{
	bool bCreated = false;
	if (!mb)
	{
		ACE_NEW_RETURN(mb,ACE_Message_Block(m_chunk_size),false);
		ACE_CDR::mb_align(mb);
		bCreated = true;
	}

	// Start an async read
	if (m_reader.read(*mb,mb->space()) != 0)
	{
		int err = ACE_OS::last_error();
#if defined(ACE_WIN32)
		if (err == ERROR_BROKEN_PIPE)
#else
		if (err == ENOTSOCK)
#endif
			err = 0;

		if (err)
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: read failed, code: %#x - %m\n"),err));

		if (bCreated)
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
	ACE_Message_Block* mb = &result.message_block();

	bool bSuccess = false;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (result.success() || result.error() == ERROR_MORE_DATA)
#else
	if (result.success())
#endif
	{
		// Loop while we can still parse
		size_t read_more = 0;
		for (;;)
		{
			// Try to work out if we need to read more...
			if (mb->length() < ACE_CDR::LONG_SIZE * 2)
			{
				// We need to read more...
				bSuccess = true;
				read_more = m_chunk_size - (ACE_CDR::LONG_SIZE * 2);
				break;
			}

			ACE_InputCDR input(mb);

			// Read the payload specific data
			ACE_CDR::Octet byte_order;
			input.read_octet(byte_order);
			ACE_CDR::Octet version;
			input.read_octet(version);

			// Set the read for the right endianess
			input.reset_byte_order(byte_order);

			// Read the length
			ACE_CDR::ULong read_len = 0;
			input >> read_len;
			if (!input.good_bit())
				ACE_ERROR_BREAK((LM_ERROR,ACE_TEXT("%N:%l: Corrupt header\n")));

			// Update average chunk size
			m_chunk_size = (m_chunk_size * 3 + read_len) / 4;

			// Check whether we need more
			if (mb->length() < read_len)
			{
				// We need to read more...
				bSuccess = true;
				read_more = ((size_t)read_len - mb->length());
				break;
			}

			// Create a shallow copy of the block
			ACE_Message_Block* mb_short = mb->duplicate();
			mb_short->wr_ptr(mb_short->rd_ptr() + read_len);

			// Try to parse
			if (!m_pHandler->parse_message(input,mb_short))
			{
				mb_short->release();
				break;
			}
			mb_short->release();

			// Update the pointers
			mb->rd_ptr(read_len);
			if (mb->length() == 0)
			{
				bSuccess = true;
				read_more = m_chunk_size;
				ACE_CDR::mb_align(mb);
				break;
			}

			// Create a new Message Block
			size_t sz = mb->length();
			if (sz < m_chunk_size)
				sz = m_chunk_size;

			ACE_Message_Block* mb_new = 0;
			ACE_NEW_NORETURN(mb_new,ACE_Message_Block(sz + ACE_CDR::MAX_ALIGNMENT));
			if (!mb_new)
				break;

			// Align and copy to rest of the message - this is the same as a clone, but maintains alignment
			ACE_CDR::mb_align(mb_new);
			mb_new->copy(mb->rd_ptr(),mb->length());
			mb->release();
			mb = mb_new;
		}

		if (bSuccess)
		{
			if (mb->space() < read_more)
				mb->size(mb->size() + read_more);

			bSuccess = read(mb);
		}
	}

	if (!bSuccess)
	{
		int err = result.error();
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		if (err != 0 && err != ERROR_BROKEN_PIPE)
#else
		if (err != 0 && err != ENOTSOCK)
#endif
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: handle_read_*() failed %C"),ACE_OS::strerror(err)));
		}

		m_pHandler->channel_closed(m_channel_id,0);
		m_pipe->close();
		delete this;

		mb->release();
	}
}

Root::MessageHandler::MessageHandler() :
	m_req_thrd_grp_id(-1),
	m_pro_thrd_grp_id(-1),
	m_uChannelId(0),
	m_uChannelMask(0),
	m_uChildMask(0),
	m_uUpstreamChannel(0),
	m_uNextChannelId(0),
	m_uNextChannelMask(0),
	m_uNextChannelShift(0)
{
}

int Root::MessageHandler::start()
{
	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 1)
		threads = 1;

	// Spawn off the request threads
	m_req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,request_worker_fn,this);
	if (m_req_thrd_grp_id == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("spawn() failed")),-1);
	else
	{
		// Spawn off the proactor threads
		m_pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,proactor_worker_fn,this);
		if (m_pro_thrd_grp_id == -1)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("spawn() failed")));

			// Stop the MessageHandler
			stop();

			return -1;
		}
		else
		{
			m_thread_queue.high_water_mark(sizeof(WorkerInfo) * (threads+1));
			m_thread_queue.low_water_mark(m_thread_queue.high_water_mark());
		}
	}

	return 0;
}

ACE_THR_FUNC_RETURN Root::MessageHandler::proactor_worker_fn(void* pParam)
{
    ThreadContext::instance(static_cast<MessageHandler*>(pParam))->m_bPrivate = true;

	ACE_Proactor::instance()->proactor_run_event_loop();
	return 0;
}

ACE_THR_FUNC_RETURN Root::MessageHandler::request_worker_fn(void* pParam)
{
    static_cast<MessageHandler*>(pParam)->pump_requests();
	return 0;
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

ACE_CDR::ULong Root::MessageHandler::register_channel(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, ACE_CDR::ULong channel_id)
{
	try
	{
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

			if (channel_id != 0)
			{
				if (m_mapChannelIds.find(channel_id)!=m_mapChannelIds.end())
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Duplicate fixed channel registered\n")),0);
			}
			else if (m_mapChannelIds.size() >= m_uNextChannelMask - 0xF)
			{
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Out of free channels\n")),0);
			}
			else
			{
				do
				{
					channel_id = m_uChannelId | ((++m_uNextChannelId & m_uNextChannelMask) << m_uNextChannelShift);
				} while (channel_id==m_uChannelId || m_mapChannelIds.find(channel_id)!=m_mapChannelIds.end());
			}

			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::value_type(channel_id,pipe));
		}

		if (!on_channel_open(channel_id))
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

			m_mapChannelIds.erase(channel_id);
			return 0;
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),0);
	}

	return channel_id;
}

bool Root::MessageHandler::can_route(ACE_CDR::ULong, ACE_CDR::ULong)
{
	// Do nothing, used in derived classes
	return true;
}

bool Root::MessageHandler::on_channel_open(ACE_CDR::ULong)
{
	// Do nothing, used in derived classes
	return true;
}

void Root::MessageHandler::do_route_off(void* pParam, ACE_InputCDR& input)
{
	MessageHandler* pThis = static_cast<MessageHandler*>(pParam);

	// Read the payload specific data
	ACE_CDR::Octet byte_order;
	input.read_octet(byte_order);
	ACE_CDR::Octet version;
	input.read_octet(version);

	// Set the read for the right endianess
	input.reset_byte_order(byte_order);

	// Read the length
	ACE_CDR::ULong read_len = 0;
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

	ACE_CDR::ULong attribs = 0;
	input >> attribs;

	ACE_CDR::UShort dest_thread_id;
	input >> dest_thread_id;

	ACE_CDR::UShort src_thread_id;
	input >> src_thread_id;

	ACE_CDR::ULong seq_no;
	input >> seq_no;

	ACE_CDR::UShort flags;
	input >> flags;

	if (input.good_bit())
	{
		if (input.length() > 0)
			input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

		pThis->route_off(input,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
	}
}

bool Root::MessageHandler::route_off(ACE_InputCDR&, ACE_CDR::ULong, ACE_CDR::ULong, const ACE_Time_Value&, ACE_CDR::ULong, ACE_CDR::UShort, ACE_CDR::UShort, ACE_CDR::UShort, ACE_CDR::ULong)
{
	// We have nowhere to route!
	ACE_OS::last_error(ENOENT);
	return false;
}

void Root::MessageHandler::channel_closed(ACE_CDR::ULong channel_id, ACE_CDR::ULong src_channel_id)
{
	// Inform derived classes that the pipe has gone...
	on_channel_closed(channel_id);

	// Propogate the message to all user processes...
	try
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
		if (guard.locked())
		{
			for (std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
			{
				// Always route upstream, and/or follow routing rules
				if (i->first != channel_id && i->first != src_channel_id &&
					(i->first == m_uUpstreamChannel || can_route(channel_id & (m_uChannelMask | m_uChildMask),i->first & (m_uChannelMask | m_uChildMask))))
				{
					send_channel_close(i->first,channel_id);
				}
			}
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
	}

	try
	{
		// Remove the pipe
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		m_mapChannelIds.erase(channel_id);
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
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

bool Root::MessageHandler::route_message(MessageHandler::Message* msg)
{
	if (msg->m_dest_thread_id != 0)
	{
		// Find the right queue to send it to...
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
		if (guard.locked () == 0)
		{
			delete msg;
			return false;
		}

		try
		{
			std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
			if (i == m_mapThreadContexts.end())
			{
				// Bad destination thread, or thread has died...
				delete msg;
			}
			else if (i->second->m_msg_queue->enqueue_tail(msg,msg->m_deadline==ACE_Time_Value::max_time ? 0 : &msg->m_deadline) == -1)
			{
				delete msg;
			}
		}
		catch (std::exception& e)
		{
			delete msg;
			ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),false);
		}
	}
	else
	{
		// Sort out the timeout
		ACE_Time_Value deadline = msg->m_deadline;
		ACE_Time_Value now = ACE_OS::gettimeofday();

		if (deadline <= now)
		{
			// Timeout
			delete msg;
			return true;
		}

		if (deadline - now > ACE_Time_Value(1))
			deadline = now + ACE_Time_Value(1);

		// Get the next thread from the queue
		WorkerInfo* pInfo = 0;

		for (;;)
		{
			if (m_thread_queue.dequeue_head(pInfo,&deadline) == -1)
			{
				if (ACE_OS::last_error() == EWOULDBLOCK)
				{
					// Spawn off an extra thread
					if (ACE_Thread_Manager::instance()->spawn(request_worker_fn,this,THR_NEW_LWP | THR_JOINABLE,0,0,0,m_req_thrd_grp_id) == -1)
					{
						delete msg;
						ACE_ERROR_RETURN((LM_ERROR,"%N:%l: Failed to spawn extra worker thread\n"),false);
					}

					// Sleep to let the new thread have a chance to start..
					ACE_OS::sleep(ACE_Time_Value(0,100000));
					continue;
				}

				delete msg;
			}
			break;
		}

		// Set the message and signal
		pInfo->m_msg = msg;
		if (pInfo->m_Event.signal() != 0)
			delete msg;
	}

	return true;
}

bool Root::MessageHandler::parse_message(ACE_InputCDR& input, const ACE_Message_Block* mb)
{
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
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: Corrupt input\n"),false);

	// Check the destination
	bool bRoute = true;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		if (!(dest_channel_id & m_uChildMask))
		{
			bRoute = false;
		}
		else
		{
			// See if we are routing from a local source
			if ((src_channel_id & m_uChannelMask) == m_uChannelId)
			{
				// Check we can route from src to dest
				if (!can_route(src_channel_id & (m_uChannelMask | m_uChildMask),dest_channel_id & (m_uChannelMask | m_uChildMask)))
					ACE_ERROR_RETURN((LM_ERROR,"%N:%l: Attempting to route via illegal path\n"),false);
			}

			// Clear off sub channel bits
			dest_channel_id &= (m_uChannelMask | m_uChildMask);
		}
	}
	else if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		return call_async_function_i(&do_route_off,this,mb);
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
		input >> msg->m_attribs;
		input >> msg->m_dest_thread_id;
		input >> msg->m_src_thread_id;

		// Did everything make sense?
		if (!input.good_bit())
		{
			delete msg;
			ACE_ERROR_RETURN((LM_ERROR,"%N:%l: Corrupt input\n"),false);
		}

		ACE_InputCDR* pI = 0;
		ACE_NEW_NORETURN(pI,ACE_InputCDR(input));
		if (!pI)
		{
			// We are running out of memory!
			delete msg;
			return false;
		}
		msg->m_ptrPayload.reset(pI);

		// Route it correctly...
		return route_message(msg);
	}

	// Find the correct channel
	ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> dest_pipe;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return true;

		dest_pipe = i->second;
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),0);
	}

	if (deadline != ACE_Time_Value::max_time && deadline <= ACE_OS::gettimeofday())
		return true;

	size_t sent = 0;
	if (dest_pipe->send(mb,&sent) == -1)
		return false;

	return (sent == mb->total_length());
}

Root::MessageHandler::ThreadContext* Root::MessageHandler::ThreadContext::instance(Root::MessageHandler* pHandler)
{
	ThreadContext* pThis = ACE_TSS_Singleton<ThreadContext,ACE_Recursive_Thread_Mutex>::instance();
	if (pThis->m_thread_id == 0)
	{
		ACE_NEW_RETURN(pThis->m_msg_queue,(ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>),0);
		pThis->m_thread_id = pHandler->insert_thread_context(pThis);
		pThis->m_pHandler = pHandler;
	}

	if (pThis->m_thread_id == 0)
		return 0;
	else
		return pThis;
}

Root::MessageHandler::ThreadContext::ThreadContext() :
	m_bPrivate(false),
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

	m_msg_queue->close();
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
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),0);
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
			for (std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
			{
				i->second->close();
			}
		}
		catch (std::exception& e)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
		}
	}

	// Now spin, waiting for all the channels to close...
	ACE_Time_Value wait(0,100);
	for (;;)
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		if (m_mapChannelIds.empty())
			break;

		guard.release();

		ACE_OS::sleep(wait);
	}

	// Stop the proactor
	ACE_Proactor::instance()->proactor_end_event_loop();

	// Wait for all the proactor threads to finish
	ACE_Thread_Manager::instance()->wait_grp(m_pro_thrd_grp_id);
}

void Root::MessageHandler::stop()
{
	try
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::UShort,const ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		{
			if (i->second->m_usage_count)
				i->second->m_msg_queue->pulse();
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
	}

	// Close all waiting threads
	while (!m_thread_queue.is_empty())
	{
		// Dequeue each waiter
		WorkerInfo* pInfo;
		if (m_thread_queue.dequeue_head(pInfo,0) == -1)
			break;

		// Set the message and signal
		pInfo->m_msg = 0;
		pInfo->m_Event.signal();
	}
	m_thread_queue.close();

	// Wait for all the request threads to finish
	ACE_Thread_Manager::instance()->wait_grp(m_req_thrd_grp_id);
}

bool Root::MessageHandler::wait_for_response(ACE_InputCDR*& response, ACE_CDR::ULong seq_no, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id)
{
	bool bRet = false;

	ThreadContext* pContext = 0;
	pContext = ThreadContext::instance(this);
	if (!pContext)
        return false;

	// Up the usage count on the context
	++pContext->m_usage_count;

	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		{
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
				break;

			std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::iterator i=m_mapChannelIds.find(from_channel_id);
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
			ACE_CDR::ULong recv_seq_no = 0;
			(*msg->m_ptrPayload) >> recv_seq_no;

			ACE_CDR::UShort type = 0;
			(*msg->m_ptrPayload) >> type;

			if (msg->m_ptrPayload->length() > 0)
			{
				// 6 Bytes of padding here
				msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
			}

			if (msg->m_ptrPayload->good_bit())
			{
				if (type == Message_t::Request)
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
						ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
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
				else if (type == Message_t::Response && recv_seq_no == seq_no)
				{
					ACE_NEW_NORETURN(response,ACE_InputCDR(*msg->m_ptrPayload));
					if (!response)
						ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %m\n")));
					else
						bRet = true;

					delete msg;
					break;
				}
				else
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Duff message\n")));
				}
			}

			delete msg;
		}
	}

	// Dec the usage count on the context
	--pContext->m_usage_count;

	return bRet;
}

int Root::MessageHandler::pump_requests(const ACE_Time_Value* deadline, bool bOnce)
{
	ThreadContext* pContext = 0;
	pContext = ThreadContext::instance(this);
	if (!pContext)
        return -1;

	WorkerInfo info;
	info.m_msg = 0;

	do
	{
		ACE_Time_Value deadline2 = ACE_Time_Value(15) + ACE_OS::gettimeofday();
		if (deadline)
			deadline2 = *deadline;

		// Enqueue ourselves to the worker pool
		if (m_thread_queue.enqueue_tail(&info,&deadline2) == -1)
		{
			if (ACE_OS::last_error() == EWOULDBLOCK)
				return 1;
			else
				return -1;
		}

		// Wait for our signal
		if (deadline)
		{
			if (info.m_Event.wait(deadline) != 0)
			{
				if (ACE_OS::last_error() == ETIME)
					return 1;
				else
					return -1;
			}
		}
		else if (info.m_Event.wait() != 0)
			return -1;

		// Check for close
		if (!info.m_msg)
			return -1;

		// Get the next message
		Message* msg = 0;
		msg = info.m_msg;

		ACE_CDR::ULong seq_no = 0;
		(*msg->m_ptrPayload) >> seq_no;

		ACE_CDR::UShort type = 0;
		(*msg->m_ptrPayload) >> type;

		if (msg->m_ptrPayload->length() > 0)
		{
			// 6 Bytes of padding here
			msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
		}

		if (!msg->m_ptrPayload->good_bit())
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Corrupt message\n")));
		else if (type == Message_t::Request)
		{
			// Set deadline
			ACE_Time_Value old_deadline = pContext->m_deadline;
			pContext->m_deadline = msg->m_deadline;

			if (msg->m_attribs & Message_t::system_message)
			{
				if ((msg->m_attribs & Message_t::system_message) == Message_t::async_function)
					process_async_function(msg);
				else if ((msg->m_attribs & Message_t::system_message) == Message_t::channel_close)
					process_channel_close(msg);
				else if ((msg->m_attribs & Message_t::system_message) == Message_t::channel_reflect)
				{
					// Send back the src_channel_id
					ACE_OutputCDR response;
					response.write_ulong(msg->m_src_channel_id);

					send_response(seq_no,msg->m_src_channel_id,msg->m_src_thread_id,response.current(),pContext->m_deadline,Message_t::synchronous | Message_t::channel_reflect);
				}
				else
					ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Unrecognised system message\n")));
			}
			else
			{
				try
				{
					// Set per channel thread id
					pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
				}
				catch (std::exception& e)
				{
					// This shouldn't ever occur, but that means it will ;)
					ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
					pContext->m_deadline = old_deadline;
					delete msg;
					continue;
				}

				// Process the message...
				process_request(*msg->m_ptrPayload,seq_no,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);

				// Clear the channel/threads map
				pContext->m_mapChannelThreads.clear();
			}

			// Reset deadline
			pContext->m_deadline = old_deadline;
		}
		else
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Discarding response in message pump\n")));

		delete msg;

	} while (!bOnce);

	return 0;
}

void Root::MessageHandler::process_channel_close(Message* msg)
{
	ACE_CDR::ULong closed_channel_id;
	(*msg->m_ptrPayload) >> closed_channel_id;

	if (!msg->m_ptrPayload->good_bit())
		return;

	// Close the pipe
	channel_closed(closed_channel_id,msg->m_src_channel_id);

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
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
	}
}

void Root::MessageHandler::process_async_function(Message* msg)
{
	void (*pfnCall)(void*,ACE_InputCDR&);
	void* pParam = 0;

#ifdef OMEGA_64
	ACE_CDR::ULongLong v;
	msg->m_ptrPayload->read_ulonglong(v);
	pfnCall = (void (*)(void*,ACE_InputCDR&))v;
	msg->m_ptrPayload->read_ulonglong(v);
	pParam = (void*)v;
#else
	ACE_CDR::ULong v;
	msg->m_ptrPayload->read_ulong(v);
	pfnCall = (void (*)(void*,ACE_InputCDR&))(size_t)v;
	msg->m_ptrPayload->read_ulong(v);
	pParam = (void*)(size_t)v;
#endif

	if (msg->m_ptrPayload->length() > 0)
		msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

	if (msg->m_ptrPayload->good_bit())
	{
		try
		{
			// Make the call...
			(*pfnCall)(pParam,*msg->m_ptrPayload);
		}
		catch (std::exception& e)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Unhandled std::exception: %C\n"),e.what()));
		}
		catch (...)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Unhandled exception!\n")));
		}
	}
}

bool Root::MessageHandler::send_channel_close(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong closed_channel_id)
{
	ACE_OutputCDR msg;
	msg << closed_channel_id;
	if (!msg.good_bit())
		return false;

	ACE_InputCDR* null;
	return send_request(dest_channel_id,msg.begin(),null,0,Message_t::asynchronous | Message_t::channel_close);
}

bool Root::MessageHandler::call_async_function_i(void (*pfnCall)(void*,ACE_InputCDR&), void* pParam, const ACE_Message_Block* mb)
{
	if (!pfnCall)
		return false;

	ACE_OutputCDR output;

	ACE_CDR::ULong seq_no = 0;
	output << seq_no;

	ACE_CDR::UShort type = Message_t::Request;
	output << type;

	// 6 Bytes of padding here
	output.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

#ifdef OMEGA_64
	output << (ACE_CDR::ULongLong)(size_t)pfnCall;
	output << (ACE_CDR::ULongLong)(size_t)pParam;
#else
	output << (ACE_CDR::ULong)(size_t)pfnCall;
	output << (ACE_CDR::ULong)(size_t)pParam;
#endif

	if (mb)
	{
		output.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
		output.write_octet_array_mb(mb);
	}

	// Create a new message
	MessageHandler::Message* msg = 0;
	ACE_NEW_RETURN(msg,MessageHandler::Message,false);

	msg->m_deadline = ACE_Time_Value::max_time;
	msg->m_src_channel_id = m_uChannelId;
	msg->m_dest_thread_id = 0;
	msg->m_src_thread_id = 0;
	msg->m_attribs = Message_t::asynchronous | Message_t::async_function;

	ACE_InputCDR* pI = 0;
	ACE_NEW_NORETURN(pI,ACE_InputCDR(output));
	if (!pI)
	{
		// We are running out of memory!
		delete msg;
		return false;
	}
	msg->m_ptrPayload.reset(pI);

	return route_message(msg);
}

bool Root::MessageHandler::send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_Time_Value* deadline, ACE_CDR::ULong attribs)
{
	// Build a header
	Message msg;
	msg.m_dest_thread_id = 0;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = 0;
	msg.m_deadline = deadline ? *deadline : ACE_Time_Value::max_time;
	msg.m_attribs = attribs;

	ACE_CDR::ULong seq_no = 0;

	// Only use thread context if we are a synchronous call
	if (!(attribs & Message_t::asynchronous))
	{
		ThreadContext* pContext = ThreadContext::instance(this);
		if (pContext->m_bPrivate && m_uUpstreamChannel)
		{
		    ACE_OS::last_error(EACCES);
			ACE_ERROR_RETURN((LM_ERROR,"Attempting Omega method call in private thread\n"),false);
		}

		try
		{
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(dest_channel_id);
			if (i != pContext->m_mapChannelThreads.end())
				msg.m_dest_thread_id = i->second;
		}
		catch (std::exception& e)
		{
			ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),false);
		}

		msg.m_src_thread_id = pContext->m_thread_id;
		msg.m_deadline = pContext->m_deadline;

		while (!seq_no)
		{
			seq_no = ++pContext->m_seq_no;
		}
	}

	// Find the destination channel
	ACE_CDR::ULong actual_dest_channel_id = m_uUpstreamChannel;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}

	if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		ACE_InputCDR input(mb);
		if (!route_off(input,msg.m_src_channel_id,dest_channel_id,msg.m_deadline,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,Message_t::Request,seq_no))
			return false;
	}
	else
	{
		// Send upstream
		if (!send_message(Message_t::Request,seq_no,actual_dest_channel_id,dest_channel_id,msg,mb))
			return false;
	}

	if (attribs & Message_t::asynchronous)
		return true;

	// Wait for response...
	return wait_for_response(response,seq_no,msg.m_deadline != ACE_Time_Value::max_time ? &msg.m_deadline : 0,actual_dest_channel_id);
}

bool Root::MessageHandler::send_response(ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
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
	ACE_CDR::ULong actual_dest_channel_id = m_uUpstreamChannel;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}

	if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		ACE_InputCDR input(mb);
		return route_off(input,msg.m_src_channel_id,dest_channel_id,msg.m_deadline,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,Message_t::Response,seq_no);
	}
	else
	{
		// Send upstream
		return send_message(Message_t::Response,seq_no,actual_dest_channel_id,dest_channel_id,msg,mb);
	}
}

static bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
{
#if ACE_MAJOR_VERSION < 5 || (ACE_MAJOR_VERSION == 5 && (ACE_MINOR_VERSION < 5 || (ACE_MINOR_VERSION == 5 && ACE_BETA_VERSION == 0)))

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
	if (mb && mb->total_length() > ACE_INT32_MAX - 128)
	{
		ACE_OS::last_error(E2BIG);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Message too big!\n")),false);
	}

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	 // version

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	header << dest_channel_id;
	header << msg.m_src_channel_id;

	header.write_ulonglong(msg.m_deadline.sec());
	header.write_long(msg.m_deadline.usec());

	header << msg.m_attribs;
	header << msg.m_dest_thread_id;
	header << msg.m_src_thread_id;
	header << seq_no;
	header << flags;

	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("CDR write() failed")),false);

	if (mb)
	{
		header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

		// Write the request stream
		header.write_octet_array_mb(mb);
		if (!header.good_bit())
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("CDR write() failed")),false);
	}

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("CDR replace() failed")),false);

	return true;
}

bool Root::MessageHandler::forward_message(ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, const ACE_Message_Block* mb)
{
	// Check the destination
	bool bRoute = true;
	ACE_CDR::ULong actual_dest_channel_id = m_uUpstreamChannel;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		if (!(dest_channel_id & m_uChildMask))
		{
			bRoute = false;
		}
		else
		{
			// See if we are routing from a local source
			if ((src_channel_id & m_uChannelMask) == m_uChannelId)
			{
				// Check we can route from src to dest
				if (!can_route(src_channel_id & (m_uChannelMask | m_uChildMask),dest_channel_id & (m_uChannelMask | m_uChildMask)))
					ACE_ERROR_RETURN((LM_ERROR,"%N:%l: Attempting to route via illegal path\n"),false);
			}

			// Clear off sub channel bits
			actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
		}
	}
	else if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		ACE_InputCDR input(mb);
		return route_off(input,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
	}

	if (!bRoute)
	{
		// If its our message, route it

		// Build a message
		ACE_OutputCDR output;
		output << seq_no;
		output << flags;

		// 6 Bytes of padding here
		output.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
		output.write_octet_array_mb(mb);
		if (!output.good_bit())
			return false;

		MessageHandler::Message* msg = 0;
		ACE_NEW_RETURN(msg,MessageHandler::Message,false);

		msg->m_dest_thread_id = dest_thread_id;
		msg->m_src_channel_id = src_channel_id;
		msg->m_src_thread_id = src_thread_id;
		msg->m_attribs = attribs;
		msg->m_deadline = deadline;

		ACE_InputCDR* pI = 0;
		ACE_NEW_NORETURN(pI,ACE_InputCDR(output));
		if (!pI)
		{
			// We are running out of memory!
			delete msg;
			return false;
		}
		msg->m_ptrPayload.reset(pI);

		// Route it correctly...
		return route_message(msg);
	}

	// Build a header
	Message msg;
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = src_channel_id;
	msg.m_src_thread_id = src_thread_id;
	msg.m_attribs = attribs;
	msg.m_deadline = deadline;

	return send_message(flags,seq_no,actual_dest_channel_id,dest_channel_id,msg,mb);
}

bool Root::MessageHandler::send_message(ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, ACE_CDR::ULong actual_dest_channel_id, ACE_CDR::ULong dest_channel_id, const Message& msg, const ACE_Message_Block* mb)
{
	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,flags,seq_no,dest_channel_id,msg,mb))
		return false;

	ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> dest_pipe;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> >::iterator i=m_mapChannelIds.find(actual_dest_channel_id);
		if (i == m_mapChannelIds.end())
		{
			ACE_OS::last_error(ENOENT);
			return false;
		}

		dest_pipe = i->second;
	}
	catch (std::exception& e)
	{
		ACE_OS::last_error(EINVAL);
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),false);
	}

	// Check the timeout
	if (msg.m_deadline != ACE_Time_Value::max_time && msg.m_deadline <= ACE_OS::gettimeofday())
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	size_t sent = 0;
	if (dest_pipe->send(header.begin(),&sent) == -1)
		return false;

	return (sent == header.total_length());
}
