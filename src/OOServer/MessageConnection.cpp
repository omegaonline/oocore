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

#include <OOBase/GlobalNew.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/CDRStream.h>
#include <OOBase/BoundedQueue.h>
#include <OOBase/HandleTable.h>
#include <OOBase/HashTable.h>
#include <OOBase/Stack.h>
#include <OOBase/Logger.h>
#include <OOBase/Thread.h>

#include <OOSvrBase/Proactor.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#include "../../include/Omega/internal/base_types.h"

//////////////////////////////////////////////

#include "MessageConnection.h"

namespace
{
	static const size_t     s_default_buffer_size = 512;
	static const size_t     s_header_len = 2 * sizeof(Omega::uint32_t);
}

OOServer::MessageConnection::MessageConnection(MessageHandler* pHandler, OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket) :
		m_pHandler(pHandler),
		m_ptrSocket(ptrSocket),
		m_channel_id(0)
{
}

OOServer::MessageConnection::~MessageConnection()
{
}

void OOServer::MessageConnection::set_channel_id(Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_channel_id = channel_id;
}

void OOServer::MessageConnection::shutdown()
{
	LOG_DEBUG(("Shutting down connection %X",m_channel_id));

	// Send an empty message
	OOBase::CDRStream header;
	header.write_endianess();
	header.write(Omega::byte_t(1));  // version
	header.write(Omega::byte_t('S'));  // signature
	header.write(Omega::uint32_t(0));

	send(header.buffer(),NULL);
}

void OOServer::MessageConnection::on_closed()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Omega::uint32_t channel_id = m_channel_id;

	guard.release();

	if (channel_id)
		m_pHandler->channel_closed(channel_id,0);
}

int OOServer::MessageConnection::recv()
{
	// This buffer is reused...
	OOBase::RefPtr<OOBase::Buffer> pBuffer = new (std::nothrow) OOBase::Buffer(s_default_buffer_size,OOBase::CDRStream::MaxAlignment);
	if (!pBuffer)
		LOG_ERROR_RETURN(("%s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),ERROR_OUTOFMEMORY);

	addref();

	int err = m_ptrSocket->recv(this,&MessageConnection::on_recv1,pBuffer,s_header_len);
	if (err != 0)
	{
		release();
		LOG_ERROR(("AsyncSocket read failed: %s",OOBase::system_error_text(err)));
	}

	return err;
}

void OOServer::MessageConnection::on_recv1(void* param, OOBase::Buffer* buffer, int err)
{
	MessageConnection* pThis = static_cast<MessageConnection*>(param);
	if (!pThis->on_recv(buffer,err,1))
	{
		pThis->on_closed();
		pThis->release();
	}
}

void OOServer::MessageConnection::on_recv2(void* param, OOBase::Buffer* buffer, int err)
{
	MessageConnection* pThis = static_cast<MessageConnection*>(param);
	if (!pThis->on_recv(buffer,err,2))
		pThis->on_closed();

	pThis->release();
}

bool OOServer::MessageConnection::on_recv(OOBase::Buffer* buffer, int err, int part)
{
	if (err != 0)
		LOG_ERROR_RETURN(("AsyncSocket read failed: %s",OOBase::system_error_text(err)),false);

	if (buffer->length() == 0)
		return false;

	OOBase::CDRStream input(buffer);

	// Read the payload specific data
	input.read_endianess();

	// Read the version byte
	Omega::byte_t version;
	if (!input.read(version))
		LOG_ERROR_RETURN(("Failed to read version information: %s",OOBase::system_error_text(input.last_error())),false);
		
	if (version != 1)
		LOG_ERROR_RETURN(("Invalid protocol version"),false);
		
	// Room for 1 byte here! (We record an ASCII character currently)

	// Read the length
	Omega::uint32_t read_len = 0;
	input.read(read_len);

	err = input.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Corrupt header: %s",OOBase::system_error_text(err)),false);

	if (read_len == 0)
		return false;

	if (part == 1)
	{
		// Skip back to start
		buffer->mark_rd_ptr(0);

		// Now read read_len
		err = m_ptrSocket->recv(this,&MessageConnection::on_recv2,buffer,read_len);
		if (err != 0)
			LOG_ERROR_RETURN(("AsyncSocket read failed: %s",OOBase::system_error_text(err)),false);
		
		return true;
	}

	// Part 2 from here on...

	// We now have at least 1 complete message

	// Give the handler a chance to process the message
	if (!m_pHandler->parse_message(input))
		return false;

	// Start the next read
	return (recv() == 0);
}

int OOServer::MessageConnection::send(OOBase::Buffer* pBuffer1, OOBase::Buffer* pBuffer2)
{
	addref();

	int err = 0;
	if (pBuffer2)
	{
		OOBase::Buffer* bufs[2] = { pBuffer1, pBuffer2 };
		err = m_ptrSocket->send_v(this,&MessageConnection::on_sent,bufs,2);
	}
	else
		err = m_ptrSocket->send(this,&MessageConnection::on_sent,pBuffer1);

	if (err != 0)
	{
		release();
		LOG_ERROR(("AsyncSocket write failed: %s",OOBase::system_error_text(err)));
	}

	return err;
}

void OOServer::MessageConnection::on_sent(void* param, int err)
{
	MessageConnection* pThis = static_cast<MessageConnection*>(param);
	if (err != 0)
		pThis->on_closed();
		
	pThis->release();
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

bool OOServer::MessageHandler::start_request_threads(size_t threads)
{
	int err = m_threadpool.run(request_worker_fn,this,threads);
	if (err != 0)
		LOG_ERROR_RETURN(("ThreadPool::run failed: %s",OOBase::system_error_text(err)),false);

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

	// Read the timeout
	OOBase::Timeout timeout;
	input.read(timeout);
	
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
		// If it's our message, process it

		// Read in the message info
		MessageHandler::Message msg(input);
		msg.m_timeout = timeout;
		msg.m_src_channel_id = src_channel_id;

		// Read the rest of the message
		msg.m_payload.read(msg.m_attribs);
		msg.m_payload.read(msg.m_dest_thread_id);
		msg.m_payload.read(msg.m_src_thread_id);

		// Read remaining message members
		bool t;
		msg.m_payload.read(t);
		msg.m_type = (t ? Message_t::Request : Message_t::Response);

		// Align to the next boundary
		if (msg.m_payload.buffer()->length() > 0)
			msg.m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

		// Did everything make sense?
		err = msg.m_payload.last_error();
		if (err != 0)
			LOG_ERROR_RETURN(("Corrupt input: %s",OOBase::system_error_text(err)),false);

		// Route it correctly...
		io_result::type res = handle_message(msg);
		return (res == io_result::success || res == io_result::timedout);
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Find the correct channel
		OOBase::RefPtr<MessageConnection> ptrMC;
		if (!m_mapChannelIds.find(dest_channel_id,ptrMC))
		{
			// Send closed message back to sender
			send_channel_close(src_channel_id,orig_dest_channel_id);
			return true;
		}

		if (timeout.has_expired())
			return true;

		// Reset the buffer all the way to the start
		input.buffer()->mark_rd_ptr(0);

		return (ptrMC->send(input.buffer(),NULL) == 0);
	}
}

int OOServer::MessageHandler::pump_requests(const OOBase::Timeout& timeout, bool bOnce)
{
	ThreadContext* pContext = ThreadContext::instance(this);

	do
	{
		// Wait up to 15 secs
		OOBase::Timeout timeout2(15,0);
		if (!timeout.is_infinite())
			timeout2 = timeout;
		
		// Inc usage count
		++m_waiting_threads;

		// Get the next message
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = m_default_msg_queue.pop(msg,timeout2);
		
		// Dec usage count
		size_t waiters = --m_waiting_threads;

		if (res == OOBase::BoundedQueue<Message>::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(m_default_msg_queue.last_error())),0);
		else if (res != OOBase::BoundedQueue<Message>::success)
		{
			// If we have too many threads running, or we were waiting or closed, exit this thread
			if (!timeout.is_infinite() || res == OOBase::BoundedQueue<Message>::closed || (waiters > 2 && !bOnce))
				return 0;

			// Wait again...
			continue;
		}

		if (msg.m_type == Message_t::Request)
		{
			if (!process_request_context(pContext,msg,timeout))
				return 0;
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

Omega::uint32_t OOServer::MessageHandler::register_channel(OOBase::RefPtr<MessageConnection>& ptrMC, Omega::uint32_t channel_id)
{
	// Scope the lock
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		if (channel_id != 0)
		{
			if (m_mapChannelIds.exists(channel_id))
				LOG_ERROR_RETURN(("Duplicate fixed channel registered"),0);
		}
		else if (m_mapChannelIds.size() >= m_uNextChannelMask - 1)
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
	input.read_endianess();

	// Read the version byte
	Omega::byte_t version;
	input.read(version);

	// Room for 1 byte here!

	// Read the length
	Omega::uint32_t read_len = 0;
	input.read(read_len);

	// Read the destination
	Omega::uint32_t dest_channel_id = 0;
	input.read(dest_channel_id);

	// Read the source
	Omega::uint32_t src_channel_id = 0;
	input.read(src_channel_id);

	// Read the timeout
	OOBase::Timeout timeout;
	input.read(timeout);
	
	Omega::uint32_t attribs = 0;
	input.read(attribs);

	Omega::uint16_t dest_thread_id;
	input.read(dest_thread_id);

	Omega::uint16_t src_thread_id;
	input.read(src_thread_id);

	bool t;
	input.read(t);
	Message_t::Type type = (t ? Message_t::Request : Message_t::Response);

	// Did everything make sense?
	int err = input.last_error();
	if (err != 0)
		LOG_ERROR(("Corrupt input: %s",OOBase::system_error_text(err)));
	else
	{
		if (input.buffer()->length() > 0)
			input.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

		pThis->route_off(input,src_channel_id,dest_channel_id,timeout,attribs,dest_thread_id,src_thread_id,type);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::route_off(const OOBase::CDRStream&, Omega::uint32_t, Omega::uint32_t, const OOBase::Timeout&, Omega::uint32_t, Omega::uint16_t, Omega::uint16_t, Message_t::Type)
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

		OOBase::RefPtr<MessageConnection> ptrConn;
		bPulse = m_mapChannelIds.remove(channel_id,&ptrConn);

		guard.release();

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

		for (Omega::uint32_t i = 0;send_to.pop(&i);)
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

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::handle_message(Message& msg)
{
	if (msg.m_type == Message_t::Request && (msg.m_attribs & Message_t::system_message))
	{
		switch (msg.m_attribs & Message_t::system_message)
		{
		case Message_t::channel_close:
			return process_channel_close(msg);

		case Message_t::channel_reflect:
			{
				// Send back the src_channel_id
				OOBase::CDRStream response;
				if (!response.write(msg.m_src_channel_id))
					LOG_ERROR_RETURN(("Failed to write channel reflect response: %s",OOBase::system_error_text(response.last_error())),io_result::failed);

				return send_response(msg.m_src_channel_id,msg.m_src_thread_id,response,msg.m_timeout,Message_t::synchronous | Message_t::channel_reflect);
			}

		case Message_t::channel_ping:
			{
				// Send back 1 byte
				OOBase::CDRStream response;
				if (!response.write(Omega::byte_t(1)))
					LOG_ERROR_RETURN(("Failed to write ping response: %s",OOBase::system_error_text(response.last_error())),io_result::failed);

				return send_response(msg.m_src_channel_id,msg.m_src_thread_id,response,msg.m_timeout,Message_t::synchronous | Message_t::channel_ping);
			}

		case Message_t::async_function:
			// This message gets queued, let it pass...
			break;

		default:
			LOG_ERROR_RETURN(("Unrecognised system message: %X",msg.m_attribs),io_result::failed);
		}
	}

	return queue_message(msg);
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::queue_message(const Message& msg)
{
	OOBase::BoundedQueue<Message>::Result res = OOBase::BoundedQueue<Message>::closed;

	if (msg.m_dest_thread_id != 0)
	{
		// Find the right queue to send it to...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		ThreadContext* pContext = NULL;
		if (m_mapThreadContexts.find(msg.m_dest_thread_id,pContext))
			res = pContext->m_msg_queue.push(msg,msg.m_timeout);

		if (res == OOBase::BoundedQueue<Message>::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(pContext->m_msg_queue.last_error())),io_result::failed);
	}
	else
	{
		size_t waiting = m_waiting_threads;

		res = m_default_msg_queue.push(msg,msg.m_timeout);

		if (res == OOBase::BoundedQueue<Message>::success && waiting <= 1)
			start_request_threads(1);
		else if (res == OOBase::BoundedQueue<Message>::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(m_default_msg_queue.last_error())),io_result::failed);
	}

	if (res == OOBase::BoundedQueue<Message>::timedout)
		return io_result::timedout;
	else if (res == OOBase::BoundedQueue<Message>::closed)
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
		m_pHandler(0)
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
	int err = m_mapThreadContexts.insert(pContext,id,1,0xFFFF);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to add thread context: %s",OOBase::system_error_text(err)),0);

	return id;
}

void OOServer::MessageHandler::remove_thread_context(OOServer::MessageHandler::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.remove(pContext->m_thread_id);
}

void OOServer::MessageHandler::shutdown_channels()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (OOBase::RefPtr<MessageConnection> ptrConn;m_mapChannelIds.pop(NULL,&ptrConn);)
	{
		guard.release();

		ptrConn->shutdown();
		
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

	m_threadpool.join();
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::wait_for_response(OOBase::CDRStream& response, const OOBase::Timeout& timeout, Omega::uint32_t from_channel_id)
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

		guard.release();
		
		// Get the next message
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = pContext->m_msg_queue.pop(msg,timeout);

		if (res == OOBase::BoundedQueue<Message>::error)
			LOG_ERROR_RETURN(("Message pump failed: %s",OOBase::system_error_text(pContext->m_msg_queue.last_error())),io_result::failed);
		else if (res == OOBase::BoundedQueue<Message>::pulsed)
			continue;
		else if (res == OOBase::BoundedQueue<Message>::timedout)
		{
			ret = io_result::timedout;
			break;
		}
		else if (res == OOBase::BoundedQueue<Message>::closed)
		{
			ret = io_result::channel_closed;
			break;
		}

		if (msg.m_type == Message_t::Response)
		{
			response = msg.m_payload;
			ret = io_result::success;
			break;
		}
		else
			process_request_context(pContext,msg,timeout);
	}

	return ret;
}

bool OOServer::MessageHandler::process_request_context(ThreadContext* pContext, Message& msg, const OOBase::Timeout& timeout)
{
	// Check for async function messages
	if ((msg.m_attribs & Message_t::system_message) == Message_t::async_function)
		return process_async_function(msg);

	// Set per channel thread id
	Omega::uint16_t old_thread_id = 0;
	Omega::uint16_t* v = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
	if (v)
	{
		old_thread_id = *v;
		*v = msg.m_src_thread_id;
	}
	else
	{
		int err = pContext->m_mapChannelThreads.insert(msg.m_src_channel_id,msg.m_src_thread_id);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to update thread channel information: %s",OOBase::system_error_text(err)),false);
	}

	// Update timeout
	OOBase::Timeout old_timeout = pContext->m_timeout;
	pContext->m_timeout = msg.m_timeout;
	if (timeout < pContext->m_timeout)
		pContext->m_timeout = timeout;

	// Process the message...
	process_request(msg.m_payload,msg.m_src_channel_id,msg.m_src_thread_id,pContext->m_timeout,msg.m_attribs);
	
	pContext->m_timeout = old_timeout;

	// Restore old per channel thread id
	if (v)
	{
		v = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
		if (v)
			*v = old_thread_id;
	}
	else
		pContext->m_mapChannelThreads.remove(msg.m_src_channel_id);
	
	return true;
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::process_channel_close(Message& msg)
{
	Omega::uint32_t closed_channel_id;
	if (!msg.m_payload.read(closed_channel_id))
		LOG_ERROR_RETURN(("Failed to read channel close payload: %s",OOBase::system_error_text(msg.m_payload.last_error())),io_result::failed);

	// Close the channel
	channel_closed(closed_channel_id,msg.m_src_channel_id);
	return io_result::success;
}

bool OOServer::MessageHandler::process_async_function(Message& msg)
{
	OOBase::LocalString strFn;
	msg.m_payload.read(strFn);

	void (*pfnCall)(void*,OOBase::CDRStream&);
	msg.m_payload.read(pfnCall);

	void* pParam = 0;
	msg.m_payload.read(pParam);

	if (msg.m_payload.buffer()->length() > 0)
		msg.m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

	int err = msg.m_payload.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Message reading failed: %s",OOBase::system_error_text(err)),false);

	try
	{
		// Make the call...
		(*pfnCall)(pParam,msg.m_payload);
		return true;
	}
	catch (...)
	{
		LOG_ERROR_RETURN(("Unhandled exception in %s",strFn.c_str()),false);
	}
}

void OOServer::MessageHandler::send_channel_close(Omega::uint32_t dest_channel_id, Omega::uint32_t closed_channel_id)
{
	OOBase::CDRStream msg;
	if (msg.write(closed_channel_id))
		send_request(dest_channel_id,&msg,NULL,OOBase::Timeout(),Message_t::asynchronous | Message_t::channel_close);
}

bool OOServer::MessageHandler::call_async_function_i(const char* pszFn, void (*pfnCall)(void*,OOBase::CDRStream&), void* pParam, const OOBase::CDRStream* stream)
{
	if (!pfnCall)
		return false;

	// Create a new message
	MessageHandler::Message msg(sizeof(pszFn) + sizeof(pfnCall) + sizeof(pParam) + (stream ? stream->buffer()->length() : 0));
	msg.m_src_channel_id = m_uChannelId;
	msg.m_dest_thread_id = 0;
	msg.m_src_thread_id = 0;
	msg.m_attribs = Message_t::asynchronous | Message_t::async_function;
	msg.m_type = Message_t::Request;

	msg.m_payload.write(pszFn);
	msg.m_payload.write(pfnCall);
	msg.m_payload.write(pParam);

	if (stream)
	{
		msg.m_payload.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);
		msg.m_payload.write_buffer(stream->buffer());
	}

	int err = msg.m_payload.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Message writing failed: %s",OOBase::system_error_text(err)),false);

	return (queue_message(msg) == io_result::success);
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_request(Omega::uint32_t dest_channel_id, const OOBase::CDRStream* request, OOBase::CDRStream* response, const OOBase::Timeout& timeout, Omega::uint32_t attribs)
{
	// Build a header
	Message msg(request ? *request : OOBase::CDRStream());
	msg.m_dest_thread_id = 0;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = 0;
	msg.m_timeout = timeout;
	msg.m_attribs = attribs;
	msg.m_type = Message_t::Request;
	
	// Only use thread context if we are a synchronous call
	if (!(attribs & Message_t::asynchronous))
	{
		ThreadContext* pContext = ThreadContext::instance(this);

		pContext->m_mapChannelThreads.find(dest_channel_id,msg.m_dest_thread_id);
		
		msg.m_src_thread_id = pContext->m_thread_id;
		msg.m_timeout = pContext->m_timeout;
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
		io_result::type res = route_off(msg.m_payload,msg.m_src_channel_id,dest_channel_id,msg.m_timeout,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,msg.m_type);
		if (res != io_result::success)
			return res;
	}
	else
	{
		// Send upstream
		io_result::type res = send_message(actual_dest_channel_id,dest_channel_id,msg);
		if (res != io_result::success)
			return res;
	}

	if (attribs & Message_t::asynchronous)
		return io_result::success;

	// Wait for response...
	return wait_for_response(*response,msg.m_timeout,actual_dest_channel_id);
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_response(Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream& response, const OOBase::Timeout& timeout, Omega::uint32_t attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a message block
	Message msg(response);
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = m_uChannelId;
	msg.m_src_thread_id = 0;//pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_timeout = pContext->m_timeout;
	msg.m_type = Message_t::Response;
	if (timeout < msg.m_timeout)
		msg.m_timeout = timeout;

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
		return route_off(response,msg.m_src_channel_id,dest_channel_id,msg.m_timeout,attribs,msg.m_dest_thread_id,msg.m_src_thread_id,msg.m_type);
	}
	else
	{
		// Send upstream
		return send_message(actual_dest_channel_id,dest_channel_id,msg);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::forward_message(Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::Timeout& timeout, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Message_t::Type type, OOBase::CDRStream& message)
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
		return route_off(message,src_channel_id,dest_channel_id,timeout,attribs,dest_thread_id,src_thread_id,type);
	}

	if (!bRoute)
	{
		// If it's our message, route it

		MessageHandler::Message msg(message);
		msg.m_dest_thread_id = dest_thread_id;
		msg.m_src_channel_id = src_channel_id;
		msg.m_src_thread_id = src_thread_id;
		msg.m_attribs = attribs;
		msg.m_timeout = timeout;
		msg.m_type = type;

		// Route it correctly...
		return handle_message(msg);
	}
	else
	{
		// Build a header
		Message msg(message);
		msg.m_dest_thread_id = dest_thread_id;
		msg.m_src_channel_id = src_channel_id;
		msg.m_src_thread_id = src_thread_id;
		msg.m_attribs = attribs;
		msg.m_timeout = timeout;
		msg.m_type = type;
		
		return send_message(actual_dest_channel_id,dest_channel_id,msg);
	}
}

OOServer::MessageHandler::io_result::type OOServer::MessageHandler::send_message(Omega::uint32_t actual_dest_channel_id, Omega::uint32_t dest_channel_id, Message& msg)
{
	// Write the header info
	OOBase::CDRStream header(36);
	header.write_endianess();
	header.write(Omega::byte_t(1));  // version
	header.write(Omega::byte_t('S'));  // signature

	// Write out the header length and remember where we wrote it
	size_t msg_len_mark = header.buffer()->mark_wr_ptr();
	header.write(Omega::uint32_t(0));

	header.write(dest_channel_id);
	header.write(msg.m_src_channel_id);
	header.write(msg.m_timeout);
	header.write(msg.m_attribs);
	header.write(msg.m_dest_thread_id);
	header.write(msg.m_src_thread_id);
	header.write(msg.m_type == Message_t::Request ? true : false);

	size_t len = 0;
	size_t msg_len = msg.m_payload.buffer()->length();
	if (msg_len == 0)
		len = header.buffer()->length() - s_header_len;
	else
	{
		header.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

		len = header.buffer()->length() - s_header_len;

		// Check the size
		if (msg.m_payload.buffer()->length() > 0xFFFFFFFF - len)
			LOG_ERROR_RETURN(("Message too big"),io_result::failed);

		len += msg_len;
	}

	int err = header.last_error();
	if (err != 0)
		LOG_ERROR_RETURN(("Message writing failed: %s",OOBase::system_error_text(err)),io_result::failed);

	// Update the total length
	header.replace(static_cast<Omega::uint32_t>(len),msg_len_mark);

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::RefPtr<MessageConnection> ptrMC;
	if (!m_mapChannelIds.find(actual_dest_channel_id,ptrMC))
		return io_result::channel_closed;

	// Check the timeout
	if (msg.m_timeout.has_expired())
		return io_result::timedout;

	return ((ptrMC->send(header.buffer(),msg_len ? msg.m_payload.buffer() : NULL) == 0) ? io_result::success : io_result::failed);
}
