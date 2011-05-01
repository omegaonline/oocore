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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

//////////////////////////////////////////////

#include <OOBase/Memory.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/CDRStream.h>
#include <OOBase/BoundedQueue.h>
#include <OOBase/HandleTable.h>
#include <OOBase/HashTable.h>
#include <OOBase/Stack.h>
#include <OOBase/Thread.h>

#include <OOSvrBase/Proactor.h>
#include <OOSvrBase/Logger.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#include "../../include/Omega/internal/base_types.h"

//////////////////////////////////////////////

#include "MessageConnection.h"

OOServer::MessageConnection::MessageConnection(MessageHandler* pHandler, OOSvrBase::AsyncLocalSocketPtr ptrSocket) :
		m_pHandler(pHandler),
		m_ptrSocket(ptrSocket),
		m_channel_id(0),
		m_async_count(0)
{
	assert(m_ptrSocket);

	m_ptrSocket->bind_handler(this);
}

OOServer::MessageConnection::~MessageConnection()
{
	m_ptrSocket->shutdown(true,true);

	while (m_async_count > 0)
		OOBase::Thread::yield();
}

void OOServer::MessageConnection::set_channel_id(Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_channel_id = channel_id;
}

void OOServer::MessageConnection::close()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_ptrSocket->shutdown(true,false);

	Omega::uint32_t channel_id = m_channel_id;
	m_channel_id = 0;

	guard.release();

	if (channel_id)
		m_pHandler->channel_closed(channel_id,0);
}

bool OOServer::MessageConnection::read()
{
	// This buffer is reused...
	OOBase::Buffer* pBuffer = OOBase::Buffer::create(m_default_buffer_size,OOBase::CDRStream::MaxAlignment);
	if (!pBuffer)
		LOG_ERROR_RETURN(("Out of memory"),false);

	++m_async_count;

	int err = m_ptrSocket->async_recv(pBuffer);
	pBuffer->release();

	if (err != 0)
	{
		--m_async_count;
		close();
		LOG_ERROR_RETURN(("AsyncSocket read failed: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void OOServer::MessageConnection::on_recv(OOBase::Buffer* buffer, int err)
{
	if (err != 0)
	{
		LOG_ERROR(("AsyncSocket read failed: %s",OOBase::system_error_text(err)));
		--m_async_count;
		close();
		return;
	}

	const size_t header_len = 2 * sizeof(Omega::uint32_t);
	size_t read_more = 0;
	bool bSuccess = false;
	bool bRelease = false;

	// Loop reading
	for (;;)
	{
		// See if we have enough to work out the header...
		if (buffer->length() < header_len)
		{
			read_more = buffer->space();
			bSuccess = true;
			break;
		}

		// Mark the read point
		size_t mark_rd = buffer->mark_rd_ptr();

		OOBase::CDRStream header(buffer);

		// Read the payload specific data
		bool big_endian;
		header.read(big_endian);

		// Set the read for the right endianess
		header.big_endian(big_endian);

		// Read the version byte
		Omega::byte_t version;
		header.read(version);
		if (version != 1)
		{
			LOG_ERROR(("Invalid protocol version"));
			break;
		}

		// Room for 2 bytes here!

		// Read the length
		Omega::uint32_t read_len = 0;
		header.read(read_len);

		// If we add anything extra here to the header,
		// it must be padded to 8 bytes.
		// And update header_len above!

		err = header.last_error();
		if (err != 0)
		{
			LOG_ERROR(("Corrupt header: %s",OOBase::system_error_text(err)));
			break;
		}

		// Try to work out if we need to read more...
		if (buffer->length() < (read_len - header_len))
		{
			read_more = (read_len - header_len) - buffer->length();

			// Reset buffer to the start
			buffer->mark_rd_ptr(mark_rd);
			bSuccess = true;
			break;
		}

		// We now have at least 1 complete message

		// Create new stream and copy the contents
		OOBase::CDRStream input(read_len);

		// Skip back to start
		buffer->mark_rd_ptr(mark_rd);

		memcpy(input.buffer()->wr_ptr(),buffer->rd_ptr(),read_len);
		input.buffer()->wr_ptr(read_len);
		input.buffer()->rd_ptr(header_len);
		input.big_endian(header.big_endian());

		// Give the handler a chance to process the message
		if (!m_pHandler->parse_message(input))
			break;

		// Move the current rd_ptr
		buffer->rd_ptr(read_len);

		// Shuffle the rest of the buffer up to the top
		buffer->compact(OOBase::CDRStream::MaxAlignment);
		if (buffer->length() == 0)
		{
			// Don't keep oversize buffers alive
			if (buffer->space() > m_default_buffer_size * 4)
			{
				OOBase::Buffer* new_buffer = OOBase::Buffer::create(m_default_buffer_size,OOBase::CDRStream::MaxAlignment);
				if (new_buffer)
				{
					if (bRelease)
						buffer->release();

					// Just replace the buffer...
					buffer = new_buffer;
					bRelease = true;
				}
			}
		}
	}

	if (bSuccess)
	{
		err = buffer->space(read_more);
		if (err != 0)
		{
			bSuccess = false;
			LOG_ERROR(("Out of buffer space: %s",OOBase::system_error_text(err)));
		}
		else
		{
			++m_async_count;

			err = m_ptrSocket->async_recv(buffer);
			if (err != 0)
			{
				--m_async_count;

				bSuccess = false;
				LOG_ERROR(("AsyncSocket read failed: %s",OOBase::system_error_text(err)));
			}
		}
	}

	if (bRelease)
		buffer->release();

	--m_async_count;

	if (!bSuccess)
		close();
}

bool OOServer::MessageConnection::send(OOBase::Buffer* pBuffer)
{
	++m_async_count;

	int err = m_ptrSocket->async_send(pBuffer);
	if (err != 0)
	{
		--m_async_count;
		close();
		LOG_ERROR_RETURN(("AsyncSocket write failed: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void OOServer::MessageConnection::on_sent(OOBase::Buffer* /*buffer*/, int err)
{
	--m_async_count;

	if (err != 0)
	{
		LOG_ERROR(("AsyncSocket write failed: %s",OOBase::system_error_text(err)));
		close();
	}
}

void OOServer::MessageConnection::on_closed()
{
	if (m_async_count-- == 0)
		++m_async_count;

	close();
}

OOServer::MessageHandler::MessageHandler() :
		m_uUpstreamChannel(0),
		m_uChannelId(0),
		m_uChannelMask(0),
		m_uChildMask(0),
		m_uNextChannelId(0),
		m_uNextChannelMask(0),
		m_uNextChannelShift(0),
		m_mapChannelIds(m_hash),
		m_waiting_threads(0),
		m_mapThreadContexts(1)
{
	m_hash.m_p = this;
}

OOServer::MessageHandler::~MessageHandler()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Tell every thread context that we have gone...
	for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
		(*m_mapThreadContexts.at(i))->m_pHandler = NULL;
}

bool OOServer::MessageHandler::start_request_threads()
{
	// Create 2 request threads
	if (!start_thread())
		return false;
	else if (!start_thread())
	{
		stop_request_threads();
		return false;
	}

	return true;
}

bool OOServer::MessageHandler::start_thread()
{
	OOBase::Thread* pThread = new (std::nothrow) OOBase::Thread(true);
	if (!pThread)
		LOG_ERROR_RETURN(("Out of memory"),false);

	pThread->run(request_worker_fn,this);
	return true;
}

int OOServer::MessageHandler::request_worker_fn(void* pParam)
{
	return static_cast<MessageHandler*>(pParam)->pump_requests();
}

bool OOServer::MessageHandler::parse_message(OOBase::CDRStream& input)
{
	// Read the destination
	Omega::uint32_t dest_channel_id = 0;
	input.read(dest_channel_id);
	Omega::uint32_t orig_dest_channel_id = dest_channel_id;

	// Read the source
	Omega::uint32_t src_channel_id = 0;
	input.read(src_channel_id);

	// Read the deadline
	Omega::int64_t req_dline_secs;
	input.read(req_dline_secs);
	Omega::int32_t req_dline_usecs;
	input.read(req_dline_usecs);
	OOBase::timeval_t deadline = OOBase::timeval_t(req_dline_secs,req_dline_usecs);

	// Did everything make sense?
	int err = input.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Corrupt input: %s",OOBase::system_error_text(err)),false);

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
				if (!can_route(src_channel_id,dest_channel_id))
					LOG_ERROR_RETURN(("Attempting to route via illegal path"),false);
			}

			// Clear off sub channel bits
			dest_channel_id &= (m_uChannelMask | m_uChildMask);
		}
	}
	else if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Reset the rd_ptr to the start...
		input.buffer()->mark_rd_ptr(0);

		// Ignore the return
		call_async_function_i("do_route_off",&do_route_off,this,&input);

		return true;
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
		OOBase::SmartPtr<MessageHandler::Message> msg = new (std::nothrow) MessageHandler::Message();
		if (!msg)
			LOG_ERROR_RETURN(("Out of memory"),false);

		msg->m_deadline = deadline;
		msg->m_src_channel_id = src_channel_id;

		// Read the rest of the message
		input.read(msg->m_attribs);
		input.read(msg->m_dest_thread_id);
		input.read(msg->m_src_thread_id);

		// Did everything make sense?
		err = input.last_error();
		if (err != 0)
			LOG_ERROR_RETURN(("Corrupt input: %s",OOBase::system_error_text(err)),false);

		// Attach input
		msg->m_payload = input;

		// Route it correctly...
		io_result::type res = queue_message(msg);
		return (res == io_result::success || res == io_result::timedout);
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Find the correct channel
		OOBase::SmartPtr<MessageConnection> ptrMC;
		if (!m_mapChannelIds.find(dest_channel_id,ptrMC))
		{
			// Send closed message back to sender
			send_channel_close(src_channel_id,orig_dest_channel_id);
			return true;
		}

		if (deadline <= OOBase::timeval_t::gettimeofday())
			return true;

		// Reset the buffer all the way to the start
		input.buffer()->mark_rd_ptr(0);

		return ptrMC->send(input.buffer());
	}
}

int OOServer::MessageHandler::pump_requests(const OOBase::timeval_t* wait, bool bOnce)
{
	ThreadContext* pContext = ThreadContext::instance(this);

	do
	{
		// Wait up to 15 secs
		OOBase::timeval_t wait2 = OOBase::timeval_t(15);
		if (wait)
			wait2 = *wait;

		// Inc usage count
		++m_waiting_threads;

		// Get the next message
		OOBase::SmartPtr<Message> msg;
		OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = m_default_msg_queue.pop(msg,&wait2);
		
		// Dec usage count
		size_t waiters = --m_waiting_threads;

		if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(m_default_msg_queue.last_error())),0);
		else if (res != OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success)
		{
			// If we have too many threads running, or we were waiting or closed, exit this thread
			if (wait || res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::closed || (waiters > 2 && !bOnce))
				return 0;

			// Wait again...
			continue;
		}

		// Read remaining message members
		Omega::uint32_t seq_no = 0;
		msg->m_payload.read(seq_no);

		Omega::uint16_t type = 0;
		msg->m_payload.read(type);

		// Align to the next boundary
		if (msg->m_payload.buffer()->length() > 0)
		{
			// 6 Bytes padding here!
			msg->m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);
		}

		// Did everything make sense?
		int err = msg->m_payload.last_error();
		if (err != 0)
			LOG_ERROR(("Corrupt message: %s",OOBase::system_error_text(err)));
		else if (type == Message_t::Request)
		{
			if (msg->m_attribs & Message_t::system_message)
			{
				// Set deadline
				OOBase::timeval_t old_deadline = pContext->m_deadline;
				pContext->m_deadline = msg->m_deadline;

				if ((msg->m_attribs & Message_t::system_message) == Message_t::async_function)
					process_async_function(msg);
				else if ((msg->m_attribs & Message_t::system_message) == Message_t::channel_close)
					process_channel_close(msg);
				else if ((msg->m_attribs & Message_t::system_message) == Message_t::channel_reflect)
				{
					// Send back the src_channel_id
					OOBase::CDRStream response;
					response.write(msg->m_src_channel_id);

					send_response(seq_no,msg->m_src_channel_id,msg->m_src_thread_id,response,pContext->m_deadline,Message_t::synchronous | Message_t::channel_reflect);
				}
				else if ((msg->m_attribs & Message_t::system_message) == Message_t::channel_ping)
				{
					// Send back 1 byte
					OOBase::CDRStream response;
					response.write(Omega::byte_t(1));

					send_response(seq_no,msg->m_src_channel_id,msg->m_src_thread_id,response,pContext->m_deadline,Message_t::synchronous | Message_t::channel_ping);
				}
				else
					LOG_ERROR(("Unrecognised system message"));

				// Reset deadline
				pContext->m_deadline = old_deadline;
			}
			else
			{
				// Process the message...
				process_request_context(pContext,msg,seq_no);
			}
		}
		else
			LOG_ERROR(("Discarding response in message pump"));

	}
	while (!bOnce);

	return 1;
}

void OOServer::MessageHandler::set_channel(Omega::uint32_t channel_id, Omega::uint32_t mask, Omega::uint32_t child_mask, Omega::uint32_t upstream_id)
{
	assert(!(channel_id & ~mask));
	assert(!(channel_id & child_mask));

	m_uChannelId = channel_id;
	m_uChannelMask = mask;
	m_uChildMask = child_mask;
	m_uUpstreamChannel = upstream_id;

	// Turn child mask into a shift...
	m_uNextChannelMask = m_uChildMask;
	for (m_uNextChannelShift = 0; !(m_uNextChannelMask & 1); ++m_uNextChannelShift)
	{
		m_uNextChannelMask >>= 1;
	}
}

Omega::uint32_t OOServer::MessageHandler::register_channel(OOBase::SmartPtr<MessageConnection>& ptrMC, Omega::uint32_t channel_id)
{
	// Scope the lock
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		if (channel_id != 0)
		{
			if (m_mapChannelIds.exists(channel_id))
				LOG_ERROR_RETURN(("Duplicate fixed channel registered"),0);
		}
		else if (m_mapChannelIds.size() >= m_uNextChannelMask - 0xF)
		{
			LOG_ERROR_RETURN(("Out of free channels"),0);
		}
		else
		{
			do
			{
				channel_id = m_uChannelId | ((++m_uNextChannelId & m_uNextChannelMask) << m_uNextChannelShift);
			}
			while (channel_id==m_uChannelId || m_mapChannelIds.exists(channel_id));
		}

		int err = m_mapChannelIds.insert(channel_id,ptrMC);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to allocate table space: %s",OOBase::system_error_text(err)),0);
	}
	
	ptrMC->set_channel_id(channel_id);
	return channel_id;
}

bool OOServer::MessageHandler::can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel)
{
	// Don't route to null channels
	return (src_channel != 0 && dest_channel != 0 && src_channel != dest_channel);
}

void OOServer::MessageHandler::do_route_off(void* pParam, OOBase::CDRStream& input)
{
	MessageHandler* pThis = static_cast<MessageHandler*>(pParam);

	// Read the payload specific data
	bool big_endian;
	input.read(big_endian);

	// Set the read for the right endianess
	input.big_endian(big_endian);

	// Read the version byte
	Omega::byte_t version;
	input.read(version);

	// Read the length
	Omega::uint32_t read_len = 0;
	input.read(read_len);

	// Read the destination
	Omega::uint32_t dest_channel_id = 0;
	input.read(dest_channel_id);

	// Read the source
	Omega::uint32_t src_channel_id = 0;
	input.read(src_channel_id);

	// Read the deadline
	Omega::int64_t req_dline_secs;
	input.read(req_dline_secs);
	Omega::int32_t req_dline_usecs;
	input.read(req_dline_usecs);
	OOBase::timeval_t deadline = OOBase::timeval_t(req_dline_secs,req_dline_usecs);

	Omega::uint32_t attribs = 0;
	input.read(attribs);

	Omega::uint16_t dest_thread_id;
	input.read(dest_thread_id);

	Omega::uint16_t src_thread_id;
	input.read(src_thread_id);

	Omega::uint32_t seq_no;
	input.read(seq_no);

	Omega::uint16_t flags;
	input.read(flags);

	// Did everything make sense?
	int err = input.last_error();
	if (err != 0)
		LOG_ERROR(("Corrupt input: %s",OOBase::system_error_text(err)));
	else
	{
		if (input.buffer()->length() > 0)
			input.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

		pThis->route_off(input,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::route_off(const OOBase::CDRStream&, Omega::uint32_t, Omega::uint32_t, const OOBase::timeval_t&, Omega::uint32_t, Omega::uint16_t, Omega::uint16_t, Omega::uint16_t, Omega::uint32_t)
{
	// We have nowhere to route!
	return io_result::channel_closed;
}

void OOServer::MessageHandler::channel_closed(Omega::uint32_t channel_id, Omega::uint32_t src_channel_id)
{
	// Remove the channel
	bool bPulse = false;
	bool bReport = true;
	
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		bPulse = m_mapChannelIds.erase(channel_id);

		// If src_channel_id == 0 then the underlying connection has closed, check whether we need to report it...
		if (!bPulse && src_channel_id == 0)
			bReport = false;
	}
	
	if (bReport)
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Propogate the message to all user processes...
		OOBase::Stack<Omega::uint32_t,OOBase::LocalAllocator> send_to;
		for (size_t i=m_mapChannelIds.begin(); i!=m_mapChannelIds.npos; i=m_mapChannelIds.next(i))
		{
			// Always route upstream, and/or follow routing rules
			Omega::uint32_t k = *m_mapChannelIds.key_at(i);
			if (k != src_channel_id && (k == m_uUpstreamChannel || can_route(channel_id,k)))
				send_to.push(k);
		}

		guard.release();

		Omega::uint32_t i = 0;
		while (send_to.pop(&i))
			send_channel_close(i,channel_id);
				
		// Inform derived classes that the channel has gone...
		on_channel_closed(channel_id);
	}

	if (bPulse)
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Unblock all waiting threads
		for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
			(*m_mapThreadContexts.at(i))->m_msg_queue.pulse();
	}
}

Omega::uint16_t OOServer::MessageHandler::classify_channel(Omega::uint32_t channel_id)
{
	// The response codes must match Remoting::MarshalFlags values
	if ((channel_id & m_uChannelMask) == m_uChannelId)
	{
		if (!(channel_id & m_uChildMask))
		{
			Omega::uint16_t compartment_id = static_cast<Omega::uint16_t>(channel_id & ~(m_uChannelMask | m_uChildMask));
			if (compartment_id == 0)
				return 0; // same
			else
				return 1; // compartment
		}
		else
			return 2; // inter_process;
	}
	else if (channel_id & 0x80000000)
		return 3; // inter_user;
	else
		return 4; // another_machine
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::queue_message(OOBase::SmartPtr<Message>& msg)
{
	OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::closed;

	if (msg->m_dest_thread_id != 0)
	{
		// Find the right queue to send it to...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		ThreadContext* pContext = NULL;
		if (m_mapThreadContexts.find(msg->m_dest_thread_id,pContext))
			res = pContext->m_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);

		if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(pContext->m_msg_queue.last_error())),io_result::failed);
	}
	else
	{
		size_t waiting = m_waiting_threads;

		res = m_default_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);

		if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success && waiting <= 1)
			start_thread();
		else if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(m_default_msg_queue.last_error())),io_result::failed);
	}

	if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::timedout)
		return io_result::timedout;
	else if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::closed)
		return io_result::channel_closed;
	else
		return io_result::success;
}

OOServer::MessageHandler::ThreadContext* OOServer::MessageHandler::ThreadContext::instance(OOServer::MessageHandler* pHandler)
{
	ThreadContext* pThis = OOBase::TLSSingleton<ThreadContext,MessageHandler>::instance();
	if (pThis->m_thread_id == 0)
	{
		pThis->m_thread_id = pHandler->insert_thread_context(pThis);
		pThis->m_pHandler = pHandler;
	}

	if (pThis->m_thread_id == 0)
		return 0;
	else
		return pThis;
}

OOServer::MessageHandler::ThreadContext::ThreadContext() :
		m_thread_id(0),
		m_pHandler(0),
		m_deadline(OOBase::timeval_t::MaxTime),
		m_seq_no(0)
{
}

OOServer::MessageHandler::ThreadContext::~ThreadContext()
{
	if (m_pHandler)
		m_pHandler->remove_thread_context(this);
}

// Accessors for ThreadContext
Omega::uint16_t OOServer::MessageHandler::insert_thread_context(OOServer::MessageHandler::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Omega::uint16_t id = 0;
	int err = m_mapThreadContexts.insert(pContext,id,1,0xFFF);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to add thread context: %s",OOBase::system_error_text(err)),0);

	return id;
}

void OOServer::MessageHandler::remove_thread_context(OOServer::MessageHandler::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

void OOServer::MessageHandler::close_channels()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<MessageConnection> ptrConn;
	while (m_mapChannelIds.pop(NULL,&ptrConn))
	{
		guard.release();

		ptrConn->close();
		
		guard.acquire();
	}
}

void OOServer::MessageHandler::stop_request_threads()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
		(*m_mapThreadContexts.at(i))->m_msg_queue.close();
		
	guard.release();
	
	m_default_msg_queue.close();

	// Wait for all the request threads to finish
	OOBase::timeval_t wait(30);
	OOBase::Countdown countdown(&wait);
	while (wait != OOBase::timeval_t::Zero && m_waiting_threads != 0)
	{
		OOBase::Thread::yield();

		countdown.update();
	}

	// Take a moment to let the threads actually complete...
	OOBase::Thread::sleep(OOBase::timeval_t(0,100000));
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::wait_for_response(OOBase::SmartPtr<OOBase::CDRStream>& response, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline, Omega::uint32_t from_channel_id)
{
	ThreadContext* pContext = ThreadContext::instance(this);

	io_result::type ret = io_result::failed;
	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		if (!m_mapChannelIds.exists(from_channel_id))
		{
			// Channel has gone!
			ret = io_result::channel_closed;
			break;
		}
		
		// Get the next message
		OOBase::SmartPtr<Message> msg;
		OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = pContext->m_msg_queue.pop(msg,deadline);

		if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(pContext->m_msg_queue.last_error())),io_result::failed);
		else if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::pulsed)
			continue;
		else if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::timedout)
		{
			ret = io_result::timedout;
			break;
		}
		else if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::closed)
		{
			ret = io_result::channel_closed;
			break;
		}

		Omega::uint32_t recv_seq_no = 0;
		msg->m_payload.read(recv_seq_no);

		Omega::uint16_t type = 0;
		msg->m_payload.read(type);

		if (msg->m_payload.buffer()->length() > 0)
		{
			// 6 Bytes of padding here
			msg->m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);
		}

		int err = msg->m_payload.last_error();
		if (err != 0)
			LOG_ERROR(("Message reading failed: %s",OOBase::system_error_text(err)));
		else
		{
			if (type == Message_t::Request)
			{
				process_request_context(pContext,msg,recv_seq_no,deadline);
			}
			else if (type == Message_t::Response && recv_seq_no == seq_no)
			{
				response = new (std::nothrow) OOBase::CDRStream(msg->m_payload);
				if (!response)
					LOG_ERROR(("Out of memory"));
				else
					ret = io_result::success;

				break;
			}
			else
			{
				LOG_ERROR(("Invalid message type"));
			}
		}
	}

	return ret;
}

bool OOServer::MessageHandler::process_request_context(ThreadContext* pContext, OOBase::SmartPtr<Message> msg, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline)
{
	// Set per channel thread id
	Omega::uint16_t old_thread_id = 0;
	Omega::uint16_t* v = pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
	if (v)
	{
		old_thread_id = *v;
		*v = msg->m_src_thread_id;
	}
	else
	{
		int err = pContext->m_mapChannelThreads.insert(msg->m_src_channel_id,msg->m_src_thread_id);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to update thread channel information: %s",OOBase::system_error_text(err)),false);
	}

	// Update deadline
	OOBase::timeval_t old_deadline = pContext->m_deadline;
	pContext->m_deadline = msg->m_deadline;
	if (deadline && *deadline < pContext->m_deadline)
		pContext->m_deadline = *deadline;

	// Process the message...
	process_request(msg->m_payload,seq_no,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);
	
	pContext->m_deadline = old_deadline;

	// Restore old per channel thread id
	if (v)
	{
		v = pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
		if (v)
			*v = old_thread_id;
	}
	else
		pContext->m_mapChannelThreads.erase(msg->m_src_channel_id);
	
	return true;
}

void OOServer::MessageHandler::process_channel_close(OOBase::SmartPtr<Message>& msg)
{
	Omega::uint32_t closed_channel_id;
	msg->m_payload.read(closed_channel_id);

	int err = msg->m_payload.last_error();
	if (err != 0)
	{
		LOG_ERROR(("Message reading failed: %s",OOBase::system_error_text(err)));
		return;
	}

	// Close the channel
	channel_closed(closed_channel_id,msg->m_src_channel_id);
}

void OOServer::MessageHandler::process_async_function(OOBase::SmartPtr<Message>& msg)
{
	OOBase::LocalString strFn;
	msg->m_payload.read(strFn);

	void (*pfnCall)(void*,OOBase::CDRStream&);
	msg->m_payload.read(pfnCall);

	void* pParam = 0;
	msg->m_payload.read(pParam);

	if (msg->m_payload.buffer()->length() > 0)
		msg->m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

	int err = msg->m_payload.last_error();
	if (err != 0)
	{
		LOG_ERROR(("Message reading failed: %s",OOBase::system_error_text(err)));
		return;
	}

	try
	{
		// Make the call...
		(*pfnCall)(pParam,msg->m_payload);
	}
	catch (...)
	{
		LOG_ERROR(("Unhandled exception in %s",strFn.c_str()));
	}
}

void OOServer::MessageHandler::send_channel_close(Omega::uint32_t dest_channel_id, Omega::uint32_t closed_channel_id)
{
	OOBase::CDRStream msg;
	if (msg.write(closed_channel_id))
	{
		OOBase::SmartPtr<OOBase::CDRStream> response;
		send_request(dest_channel_id,&msg,response,0,Message_t::asynchronous | Message_t::channel_close);
	}
}

bool OOServer::MessageHandler::call_async_function_i(const char* pszFn, void (*pfnCall)(void*,OOBase::CDRStream&), void* pParam, const OOBase::CDRStream* stream)
{
	assert(pfnCall);
	if (!pfnCall)
		return false;

	// Create a new message
	OOBase::SmartPtr<MessageHandler::Message> msg = new (std::nothrow) MessageHandler::Message(24 + (stream ? stream->buffer()->length() : 0));
	if (!msg)
		LOG_ERROR_RETURN(("Out of memory"),false);

	msg->m_deadline = OOBase::timeval_t::MaxTime;
	msg->m_src_channel_id = m_uChannelId;
	msg->m_dest_thread_id = 0;
	msg->m_src_thread_id = 0;
	msg->m_attribs = Message_t::asynchronous | Message_t::async_function;

	Omega::uint32_t seq_no = 0;
	msg->m_payload.write(seq_no);

	Omega::uint16_t type = Message_t::Request;
	msg->m_payload.write(type);

	// 2 Bytes of padding here
	msg->m_payload.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

	msg->m_payload.write(pszFn);
	msg->m_payload.write(pfnCall);
	msg->m_payload.write(pParam);

	if (stream)
	{
		msg->m_payload.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);
		msg->m_payload.write_buffer(stream->buffer());
	}

	int err = msg->m_payload.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Message writing failed: %s",OOBase::system_error_text(err)),false);

	return (queue_message(msg) == io_result::success);
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_request(Omega::uint32_t dest_channel_id, OOBase::CDRStream* request, OOBase::SmartPtr<OOBase::CDRStream>& response, const OOBase::timeval_t* deadline, Omega::uint32_t attribs)
{
	// Build a header
	Message msg;
	msg.m_dest_thread_id = 0;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = 0;
	msg.m_deadline = deadline ? *deadline : OOBase::timeval_t::MaxTime;
	msg.m_attribs = attribs;

	if (request)
		msg.m_payload = *request;

	Omega::uint32_t seq_no = 0;

	// Only use thread context if we are a synchronous call
	if (!(attribs & Message_t::asynchronous))
	{
		ThreadContext* pContext = ThreadContext::instance(this);

		pContext->m_mapChannelThreads.find(dest_channel_id,msg.m_dest_thread_id);
		
		msg.m_src_thread_id = pContext->m_thread_id;
		msg.m_deadline = pContext->m_deadline;

		while (!seq_no)
		{
			seq_no = ++pContext->m_seq_no;
		}
	}

	// Find the destination channel
	Omega::uint32_t actual_dest_channel_id = m_uUpstreamChannel;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}

	if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		io_result::type res = route_off(msg.m_payload,msg.m_src_channel_id,dest_channel_id,msg.m_deadline,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,Message_t::Request,seq_no);
		if (res != io_result::success)
			return res;
	}
	else
	{
		// Send upstream
		io_result::type res = send_message(Message_t::Request,seq_no,actual_dest_channel_id,dest_channel_id,msg);
		if (res != io_result::success)
			return res;
	}

	if (attribs & Message_t::asynchronous)
		return io_result::success;

	// Wait for response...
	return wait_for_response(response,seq_no,msg.m_deadline != OOBase::timeval_t::MaxTime ? &msg.m_deadline : 0,actual_dest_channel_id);
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_response(Omega::uint32_t seq_no, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, OOBase::CDRStream& response, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
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

	msg.m_payload = response;

	// Find the destination channel
	Omega::uint32_t actual_dest_channel_id = m_uUpstreamChannel;
	if ((dest_channel_id & m_uChannelMask) == m_uChannelId)
	{
		// Clear off sub channel bits
		actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
	}

	if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		return route_off(response,msg.m_src_channel_id,dest_channel_id,msg.m_deadline,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,Message_t::Response,seq_no);
	}
	else
	{
		// Send upstream
		return send_message(Message_t::Response,seq_no,actual_dest_channel_id,dest_channel_id,msg);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::forward_message(Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no, OOBase::CDRStream& message)
{
	// Check the destination
	bool bRoute = true;
	Omega::uint32_t actual_dest_channel_id = m_uUpstreamChannel;
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
				if (!can_route(src_channel_id,dest_channel_id))
					LOG_ERROR_RETURN(("Attempting to route via illegal path"),io_result::failed);
			}

			// Clear off sub channel bits
			actual_dest_channel_id = dest_channel_id & (m_uChannelMask | m_uChildMask);
		}
	}
	else if (m_uUpstreamChannel && !(dest_channel_id & m_uUpstreamChannel))
	{
		// Send off-machine
		return route_off(message,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
	}

	if (!bRoute)
	{
		// If its our message, route it

		OOBase::SmartPtr<MessageHandler::Message> msg = new (std::nothrow) MessageHandler::Message(8 + message.buffer()->length());
		if (!msg)
			LOG_ERROR_RETURN(("Out of memory"),io_result::failed);

		msg->m_dest_thread_id = dest_thread_id;
		msg->m_src_channel_id = src_channel_id;
		msg->m_src_thread_id = src_thread_id;
		msg->m_attribs = attribs;
		msg->m_deadline = deadline;

		// Build a message
		msg->m_payload.write(seq_no);
		msg->m_payload.write(flags);

		// 2 Bytes of padding here
		msg->m_payload.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);
		msg->m_payload.write_buffer(message.buffer());

		int err = msg->m_payload.last_error();
		if (err != 0)
			LOG_ERROR_RETURN(("Message writing failed: %s",OOBase::system_error_text(err)),io_result::failed);

		// Route it correctly...
		return queue_message(msg);
	}
	else
	{
		// Build a header
		Message msg;
		msg.m_dest_thread_id = dest_thread_id;
		msg.m_src_channel_id = src_channel_id;
		msg.m_src_thread_id = src_thread_id;
		msg.m_attribs = attribs;
		msg.m_deadline = deadline;
		msg.m_payload = message;

		return send_message(flags,seq_no,actual_dest_channel_id,dest_channel_id,msg);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_message(Omega::uint16_t flags, Omega::uint32_t seq_no, Omega::uint32_t actual_dest_channel_id, Omega::uint32_t dest_channel_id, const Message& msg)
{
	// Write the header info
	OOBase::CDRStream header;
	header.write(header.big_endian());
	header.write(Omega::byte_t(1));  // version

	// Write out the header length and remember where we wrote it
	size_t msg_len_mark = header.buffer()->mark_wr_ptr();
	header.write(Omega::uint32_t(0));

	header.write(dest_channel_id);
	header.write(msg.m_src_channel_id);

	header.write(static_cast<Omega::int64_t>(msg.m_deadline.tv_sec()));
	header.write(static_cast<Omega::int32_t>(msg.m_deadline.tv_usec()));

	header.write(msg.m_attribs);
	header.write(msg.m_dest_thread_id);
	header.write(msg.m_src_thread_id);
	header.write(seq_no);
	header.write(flags);

	if (msg.m_payload.buffer()->length() > 0)
	{
		header.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

		// Check the size
		if (msg.m_payload.buffer()->length() - header.buffer()->length() > 0xFFFFFFFF)
			LOG_ERROR_RETURN(("Message too big"),io_result::failed);

		// Write the payload stream
		header.write_buffer(msg.m_payload.buffer());
	}

	int err = header.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Message writing failed: %s",OOBase::system_error_text(err)),io_result::failed);

	// Update the total length
	header.replace(header.buffer()->length(),msg_len_mark);

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<MessageConnection> ptrMC;
	if (!m_mapChannelIds.find(actual_dest_channel_id,ptrMC))
		return io_result::channel_closed;

	// Check the timeout
	if (msg.m_deadline != OOBase::timeval_t::MaxTime && msg.m_deadline <= OOBase::timeval_t::gettimeofday())
		return io_result::timedout;

	return (ptrMC->send(header.buffer()) ? io_result::success : io_result::failed);
}
