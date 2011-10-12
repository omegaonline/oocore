///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "UserSession.h"
#include "Activation.h"
#include "StdObjectManager.h"
#include "LoopChannel.h"

#include <signal.h>

using namespace Omega;
using namespace OTL;

namespace
{
	static const size_t s_header_len = sizeof(uint32_t) * 2;
}

OOCore::UserSession::UserSession() :
		m_worker_thread(false),
		m_channel_id(0),
		m_nIPSCookie(0),
		m_init_count(0),
		m_init_state(eStopped),
		m_usage_count(0),
		m_mapThreadContexts(1),
		m_mapCompartments(0)
{
}

OOCore::UserSession::~UserSession()
{
	// Clear the thread id's of the ThreadContexts
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Tell every thread context that we have gone...
	for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
		(*m_mapThreadContexts.at(i))->m_thread_id = 0;
}

IException* OOCore::UserSession::init(const string_t& args)
{
	try
	{
		USER_SESSION::instance().init_i(args);
	}
	catch (IException* pE)
	{
		term();
		return pE;
	}

	return 0;
}

void OOCore::UserSession::term()
{
	USER_SESSION::instance().term_i();
}

void OOCore::UserSession::init_i(const string_t& args)
{
#if defined(_WIN32)
	// If this event exists, then we are being debugged
	OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Local\\OOCORE_DEBUG_MUTEX"));
	if (hDebugEvent)
	{
		// Wait for a bit, letting the caller attach a debugger
		WaitForSingleObject(hDebugEvent,5000);
	}
#endif

	OOBase::Guard<OOBase::Condition::Mutex> guard(m_cond_mutex);

	for (;;)
	{
		switch (m_init_state)
		{
		case eStopped:
			{
				m_init_state = eStarting;
				guard.release();

				try
				{
					start(args);

					guard.acquire();
					m_init_state = eStarted;
					m_cond.broadcast();
				}
				catch (...)
				{
					guard.acquire();
					m_init_state = eStopped;
					m_cond.signal();
					throw;
				}
			}
			break;

		case eStarting:
		case eStopping:
			// Wait for a change in state
			m_cond.wait(m_cond_mutex);
			break;

		case eStarted:
			// Inc init count if we have started...
			++m_init_count;
			return;
		}
	}
}

void OOCore::UserSession::term_i()
{
	OOBase::Guard<OOBase::Condition::Mutex> guard(m_cond_mutex);

	for (;;)
	{
		switch (m_init_state)
		{
		case eStarted:
			if (m_init_count > 0 && --m_init_count == 0)
			{
				m_init_state = eStopping;
				guard.release();

				stop();

				guard.acquire();
				m_init_state = eStopped;
				m_cond.broadcast();
			}
			break;

		case eStarting:
		case eStopping:
			// Wait for a change in state
			m_cond.wait(m_cond_mutex);
			break;

		case eStopped:
			return;
		}
	}
}

void OOCore::UserSession::stop()
{
	// Unregister InterProcessService
	try
	{
		OOCore_RevokeIPS(m_nIPSCookie);
		m_nIPSCookie = 0;
	}
	catch (IException* pE)
	{
		pE->Release();
	}

	// Close all singletons
	close_singletons_i();

	// Close compartments
	close_compartments();

	// Stop the io thread...
	if (m_stream)
	{
		OOBase::CDRStream header;
		header.write(header.big_endian());
		header.write(byte_t(1));     // version
		header.write(byte_t('o'));   // signature
		header.write(byte_t('c'));   // signature
		header.write(uint32_t(0));
		m_stream->send(header.buffer());
	}

	// Wait for the io worker thread to finish
	m_worker_thread.join();

	// Unload the OOSvrLite dll if loaded
	m_lite_dll.unload();
	
	// Clear our environment variable
#if defined(_WIN32)
	SetEnvironmentVariable("OMEGA_SESSION_ADDRESS",NULL);
#else
	unsetenv("OMEGA_SESSION_ADDRESS");
#endif
}

void OOCore::UserSession::close_singletons()
{
	USER_SESSION::instance().close_singletons_i();
}

void OOCore::UserSession::close_singletons_i()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (Uninit uninit;m_listUninitCalls.pop(&uninit);)
	{
		guard.release();

		try
		{
			OOBase::Guard<OOBase::SpinLock> guard(OOBase::Singleton<OOBase::SpinLock,OOCore::DLL>::instance());

			(*uninit.pfn_dctor)(uninit.param);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
		catch (...)
		{}

		guard.acquire();
	}
}

void OOCore::UserSession::close_compartments()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Do these in reverse order...
	OOBase::Stack<OOBase::SmartPtr<Compartment>,OOBase::LocalAllocator> vecCompts;
	for (size_t i = m_mapCompartments.begin();i!=m_mapCompartments.npos;i=m_mapCompartments.next(i))
		vecCompts.push(*m_mapCompartments.at(i));

	guard.release();

	for (OOBase::SmartPtr<Compartment> ptrCmpt;vecCompts.pop(&ptrCmpt);)
	{
		try
		{
			ptrCmpt->shutdown();
		}
		catch (IException* pE)
		{
			pE->Release();
		}
	}
}

uint32_t OOCore::UserSession::get_channel_id() const
{
	return m_channel_id;
}

void OOCore::UserSession::add_uninit_call(Threading::DestructorCallback pfn, void* param)
{
	USER_SESSION::instance().add_uninit_call_i(pfn,param);
}

void OOCore::UserSession::add_uninit_call_i(Threading::DestructorCallback pfn, void* param)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Uninit uninit = { pfn, param };

	int err = m_listUninitCalls.push(uninit);
	if (err != 0)
		OMEGA_THROW(err);
}

void OOCore::UserSession::remove_uninit_call(Threading::DestructorCallback pfn, void* param)
{
	UserSession* pThis = USER_SESSION::instance_ptr();
	if (pThis)
		pThis->remove_uninit_call_i(pfn,param);
}

void OOCore::UserSession::remove_uninit_call_i(Threading::DestructorCallback pfn, void* param)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Uninit uninit = { pfn, param };

	m_listUninitCalls.erase(uninit);
}

int OOCore::UserSession::io_worker_fn(void* pParam)
{
#if defined(HAVE_UNISTD_H)

	// Block all signals here...
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGCHLD);

	pthread_sigmask(SIG_BLOCK, &set, NULL);

#endif

	return static_cast<UserSession*>(pParam)->run_read_loop();
}

void OOCore::UserSession::wait_or_alert(const OOBase::Atomic<size_t>& usage)
{
	// Make this value configurable somehow...
	OOBase::Countdown countdown(0,500000);
	do
	{
		// The tinyest sleep
		OOBase::Thread::yield();
	}
	while (usage == 0 && !countdown.has_ended());

	if (usage == 0)
	{
		// This incoming request may not be processed for some time...
		void* ISSUE_9;    // Alert!
	}
}

int OOCore::UserSession::run_read_loop()
{
	OOBase::CDRStream header(s_header_len);

	int err = 0;
	for (;;)
	{
		// Read the header
		header.reset();
		err = m_stream->recv(header.buffer(),s_header_len);
		if (err != 0)
			break;

		// Read the payload specific data
		bool big_endian;
		header.read(big_endian);

		// Set the read for the right endianess
		header.big_endian(big_endian);

		// Read the version byte
		byte_t version;
		header.read(version);
		if (version != 1)
			OOBase_CallCriticalFailure("Invalid protocol version");

		// Room for 2 bytes here!

		// Read the length
		uint32_t nReadLen = 0;
		header.read(nReadLen);

		// If we add anything extra here to the header,
		// it must be padded to 8 bytes.

		if ((err = header.last_error()) != 0)
			break;

		// Check for disconnect
		if (nReadLen == 0)
			break;

		// Create a new Message struct
		Message msg(nReadLen);
		
		// Issue another read for the rest of the data
		err = m_stream->recv(msg.m_payload.buffer(),nReadLen);
		if (err != 0)
			break;

		// Reset the byte order
		msg.m_payload.big_endian(big_endian);

		// Read the destination and source channels
		uint32_t dest_channel_id;
		msg.m_payload.read(dest_channel_id);
		msg.m_payload.read(msg.m_src_channel_id);

		// Read the deadline
		int64_t req_dline_secs;
		msg.m_payload.read(req_dline_secs);
		int32_t req_dline_usecs;
		msg.m_payload.read(req_dline_usecs);
		msg.m_deadline = OOBase::timeval_t(req_dline_secs,req_dline_usecs);

		// Read the rest of the message
		msg.m_payload.read(msg.m_attribs);
		msg.m_payload.read(msg.m_dest_thread_id);
		msg.m_payload.read(msg.m_src_thread_id);
		msg.m_payload.read(msg.m_seq_no);
		msg.m_payload.read(msg.m_type);

		// Align to the next boundary
		if (msg.m_payload.buffer()->length() > 0)
		{
			// 6 Bytes padding here!
			msg.m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);
		}

		// Did everything make sense?
		err = msg.m_payload.last_error();
		if (err != 0)
			break;

		// Just skip any misdirected packets
		if ((dest_channel_id & 0xFFFFF000) != m_channel_id)
			continue;

		// Unpack the compartment...
		msg.m_dest_cmpt_id = static_cast<uint16_t>(dest_channel_id & 0x00000FFF);

		if ((msg.m_attribs & Message::system_message) && msg.m_type == Message::Request)
		{
			// Process system messages here... because the main message pump may not be running currently
			if ((msg.m_attribs & Message::system_message)==Message::channel_close)
			{
				uint32_t closed_channel_id;
				if (!msg.m_payload.read(closed_channel_id))
					msg.m_payload.last_error();
				else
					process_channel_close(closed_channel_id);
			}
			else if ((msg.m_attribs & Message::system_message)==Message::channel_reflect)
			{
				// Send back the src_channel_id
				OOBase::CDRStream response(sizeof(msg.m_src_channel_id));
				if (response.write(msg.m_src_channel_id))
					send_response_catch(msg.m_dest_cmpt_id,msg.m_seq_no,msg.m_src_channel_id,msg.m_src_thread_id,&response,msg.m_deadline,Message::synchronous | Message::channel_reflect);
			}
			else if ((msg.m_attribs & Message::system_message)==Message::channel_ping)
			{
				OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

				// Send back 1 byte
				byte_t res = (m_mapCompartments.exists(msg.m_dest_cmpt_id) ? 1 : 0);

				guard.release();

				OOBase::CDRStream response(sizeof(byte_t));
				if (response.write(res))
					send_response_catch(msg.m_dest_cmpt_id,msg.m_seq_no,msg.m_src_channel_id,msg.m_src_thread_id,&response,msg.m_deadline,Message::synchronous | Message::channel_ping);
			}
		}
		else if (msg.m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			ThreadContext* pContext = NULL;
			if (m_mapThreadContexts.find(msg.m_dest_thread_id,pContext))
			{
				size_t waiting = pContext->m_usage_count;

				OOBase::BoundedQueue<Message>::Result res = pContext->m_msg_queue.push(msg,msg.m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg.m_deadline);
				if (res == OOBase::BoundedQueue<Message>::error)
				{
					err = pContext->m_msg_queue.last_error();
					break;
				}
				else if (res == OOBase::BoundedQueue<Message>::success)
				{
					if (waiting == 0)
						wait_or_alert(pContext->m_usage_count);
				}
			}
		}
		else
		{
			// Cannot have a response to 0 thread!
			if (msg.m_type == Message::Request)
			{
				size_t waiting = m_usage_count;

				OOBase::BoundedQueue<Message>::Result res = m_default_msg_queue.push(msg,msg.m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg.m_deadline);
				if (res == OOBase::BoundedQueue<Message>::error)
				{
					err = m_default_msg_queue.last_error();
					break;
				}
				else if (res == OOBase::BoundedQueue<Message>::success)
				{
					if (waiting == 0)
						wait_or_alert(m_usage_count);
				}
			}
		}
	}

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Tell all worker threads that we are done with them...
	for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
		(*m_mapThreadContexts.at(i))->m_msg_queue.close();

	// Stop the default message queue
	m_default_msg_queue.close();

	return err;
}

bool OOCore::UserSession::pump_request(const OOBase::timeval_t* wait)
{
	for (;;)
	{
		// Increment the consumers...
		++m_usage_count;

		// Get the next message
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = m_default_msg_queue.pop(msg,wait);

		// Decrement the consumers...
		--m_usage_count;

		if (res == OOBase::BoundedQueue<Message>::error)
			OOBase_CallCriticalFailure(m_default_msg_queue.last_error());
		else if (res == OOBase::BoundedQueue<Message>::timedout)
			return false;
		else if (res != OOBase::BoundedQueue<Message>::success)
		{
			// Its gone... user process has terminated
			throw Remoting::IChannelClosedException::Create();
		}

		if (msg.m_type == Message::Request)
		{
			// Don't confuse the wait deadline with the message processing deadline
			process_request(ThreadContext::instance(),msg,0);

			// We processed something
			return true;
		}
	}
}

void OOCore::UserSession::process_channel_close(uint32_t closed_channel_id)
{
	// Pass on the message to the compartments
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	bool bPulse = false;
	for (size_t j=m_mapCompartments.begin(); j!=m_mapCompartments.npos; j=m_mapCompartments.next(j))
	{
		if ((*m_mapCompartments.at(j))->process_channel_close(closed_channel_id))
			bPulse = true;
	}

	if (bPulse)
	{
		for (size_t i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.npos; i=m_mapThreadContexts.next(i))
			(*m_mapThreadContexts.at(i))->m_msg_queue.pulse();
	}
}

void OOCore::UserSession::wait_for_response(OOBase::CDRStream& response, uint32_t seq_no, const OOBase::timeval_t* deadline, uint32_t from_channel_id)
{
	ThreadContext* pContext = ThreadContext::instance();

	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		OOBase::SmartPtr<Compartment> ptrCompt;
		m_mapCompartments.find(pContext->m_current_cmpt,ptrCompt);

		guard.release();

		if (!ptrCompt || !ptrCompt->is_channel_open(from_channel_id))
		{
			// Channel has closed
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Other compartment closed while waiting for response"));
		}

		// Increment the usage count
		++pContext->m_usage_count;

		// Get the next message
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = pContext->m_msg_queue.pop(msg,deadline);

		// Decrement the usage count
		--pContext->m_usage_count;

		if (res == OOBase::BoundedQueue<Message>::error)
			OOBase_CallCriticalFailure(pContext->m_msg_queue.last_error());
		else if (res == OOBase::BoundedQueue<Message>::success)
		{
			if (msg.m_type == Message::Request)
			{
				process_request(pContext,msg,deadline);
			}
			else if (msg.m_type == Message::Response && msg.m_seq_no == seq_no)
			{
				response = msg.m_payload;
				break;
			}
		}
		else if (res == OOBase::BoundedQueue<Message>::closed)
		{
			// I/O socket has closed
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Thread queue closed while waiting for response"));
		}
		else if (res == OOBase::BoundedQueue<Message>::timedout)
			break;
	}
}

OOCore::UserSession::ThreadContext* OOCore::UserSession::ThreadContext::instance()
{
	ThreadContext* pThis = OOBase::TLSSingleton<ThreadContext,OOCore::DLL>::instance();
	if (pThis->m_thread_id == 0)
		pThis->m_thread_id = UserSession::USER_SESSION::instance().insert_thread_context(pThis);

	return pThis;
}

OOCore::UserSession::ThreadContext::ThreadContext() :
		m_thread_id(0),
		m_usage_count(0),
		m_deadline(OOBase::timeval_t::MaxTime),
		m_seq_no(0),
		m_current_cmpt(0)
{
}

OOCore::UserSession::ThreadContext::~ThreadContext()
{
	if (m_thread_id)
		UserSession::USER_SESSION::instance().remove_thread_context(m_thread_id);
}

// Accessors for ThreadContext
uint16_t OOCore::UserSession::insert_thread_context(OOCore::UserSession::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	uint16_t id = 0;
	int err = m_mapThreadContexts.insert(pContext,id,1,0xFFF);
	if (err != 0)
		OMEGA_THROW(err);

	return id;
}

void OOCore::UserSession::remove_thread_context(uint16_t thread_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.erase(thread_id);
}

void OOCore::UserSession::send_request(uint32_t dest_channel_id, const OOBase::CDRStream* request, OOBase::CDRStream* response, uint32_t timeout, uint32_t attribs)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t src_thread_id = 0;
	uint16_t dest_thread_id = 0;
	OOBase::timeval_t deadline = OOBase::timeval_t::MaxTime;

	uint32_t seq_no = 0;

	// Only use thread context if we are a synchronous call
	if (!(attribs & Message::asynchronous))
	{
		// Determine dest_thread_id
		pContext->m_mapChannelThreads.find(dest_channel_id,dest_thread_id);

		src_thread_id = pContext->m_thread_id;
		deadline = pContext->m_deadline;

		while (!seq_no)
		{
			seq_no = ++pContext->m_seq_no;
		}
	}

	if (timeout > 0)
	{
		OOBase::timeval_t deadline2 = OOBase::timeval_t::deadline(timeout);
		if (deadline2 < deadline)
			deadline = deadline2;
	}

	// Write the header info
	OOBase::CDRStream header;
	build_header(header,seq_no,m_channel_id | pContext->m_current_cmpt,src_thread_id,dest_channel_id,dest_thread_id,request,deadline,Message::Request,attribs);

	// Send to the handle
	OOBase::timeval_t wait = deadline;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		if (deadline <= now)
			throw ITimeoutException::Create();

		wait = deadline - now;
	}

	int err = m_stream->send(header.buffer(),wait != OOBase::timeval_t::MaxTime ? &wait : NULL);
	if (err != 0)
	{
		ObjectPtr<IException> ptrE = ISystemException::Create(err,OMEGA_CREATE_INTERNAL("Failed to send message buffer"));
		throw Remoting::IChannelClosedException::Create(ptrE);
	}

	if (!(attribs & TypeInfo::Asynchronous))
	{
		// Wait for response...
		wait_for_response(*response,seq_no,deadline != OOBase::timeval_t::MaxTime ? &deadline : 0,dest_channel_id);
	}
}

void OOCore::UserSession::send_response_catch(uint16_t src_cmpt_id, uint32_t seq_no, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* response, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	try
	{
		send_response(src_cmpt_id,seq_no,dest_channel_id,dest_thread_id,response,deadline,attribs);
	}
	catch (IException* pE)
	{
		// Just ignore the error
		pE->Release();
	}
}

void OOCore::UserSession::send_response(uint16_t src_cmpt_id, uint32_t seq_no, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* response, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	ThreadContext* pContext = ThreadContext::instance();

	// Write the header info
	OOBase::CDRStream header;
	build_header(header,seq_no,m_channel_id | src_cmpt_id,pContext->m_thread_id,dest_channel_id,dest_thread_id,response,deadline,Message::Response,attribs);

	OOBase::timeval_t wait = deadline;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		if (deadline <= now)
			throw ITimeoutException::Create();

		wait = deadline - now;
	}

	int err = m_stream->send(header.buffer(),wait != OOBase::timeval_t::MaxTime ? &wait : NULL);
	if (err != 0)
	{
		ObjectPtr<IException> ptrE = ISystemException::Create(err,OMEGA_CREATE_INTERNAL("Failed to send message buffer"));
		throw Remoting::IChannelClosedException::Create(ptrE);
	}
}

void OOCore::UserSession::build_header(OOBase::CDRStream& header, uint32_t seq_no, uint32_t src_channel_id, uint16_t src_thread_id, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* request, const OOBase::timeval_t& deadline, uint16_t flags, uint32_t attribs)
{
	header.write(header.big_endian());
	header.write(byte_t(1));     // version
	header.write(byte_t('o'));   // signature
	header.write(byte_t('c'));   // signature

	// Write out the header length and remember where we wrote it
	size_t msg_len_mark = header.buffer()->mark_wr_ptr();
	header.write(uint32_t(0));

	header.write(dest_channel_id);
	header.write(src_channel_id);

	header.write(static_cast<int64_t>(deadline.tv_sec()));
	header.write(static_cast<int32_t>(deadline.tv_usec()));

	header.write(attribs);
	header.write(dest_thread_id);
	header.write(src_thread_id);
	header.write(seq_no);
	header.write(flags);

	if (header.last_error() != 0)
		OMEGA_THROW(header.last_error());

	if (request)
	{
		header.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

		// Check the size
		if (request->buffer()->length() > 0xFFFFFFFF - header.buffer()->length())
			OMEGA_THROW("Message too big");

		// Write the request stream
		header.write_buffer(request->buffer());
		if (header.last_error())
			OMEGA_THROW(header.last_error());
	}

	// Update the total length
	header.replace(static_cast<uint32_t>(header.buffer()->length() - s_header_len),msg_len_mark);
}

Remoting::MarshalFlags_t OOCore::UserSession::classify_channel(uint32_t channel)
{
	Remoting::MarshalFlags_t mflags;
	if (channel == m_channel_id)
		mflags = Remoting::Same;
	else if ((channel & 0xFFFFF000) == (m_channel_id & 0xFFFFF000))
		mflags = Remoting::Compartment;
	else if ((channel & 0xFF000000) == (m_channel_id & 0xFF000000))
		mflags = Remoting::InterProcess;
	else if ((channel & 0x80000000) == (m_channel_id & 0x80000000))
		mflags = Remoting::InterUser;
	else
		mflags = Remoting::RemoteMachine;

	return mflags;
}

void OOCore::UserSession::process_request(ThreadContext* pContext, const Message& msg, const OOBase::timeval_t* deadline)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<Compartment> ptrCompt;
	m_mapCompartments.find(msg.m_dest_cmpt_id,ptrCompt);

	guard.release();

	if (!ptrCompt)
	{
		// Send a channel close back to the sender
		OOBase::CDRStream response;
		if (response.write(msg.m_dest_cmpt_id | m_channel_id))
			send_request(msg.m_src_channel_id,&response,NULL,(deadline ? deadline->msec() : 0),Message::asynchronous | Message::channel_close);

		return;
	}

	// Set per channel thread id
	uint16_t old_thread_id = 0;

	uint16_t* v = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
	if (v)
	{
		old_thread_id = *v;
		*v = msg.m_src_thread_id;
	}
	else
	{
		int err = pContext->m_mapChannelThreads.insert(msg.m_src_channel_id,msg.m_src_thread_id);
		if (err != 0)
			OMEGA_THROW(err);
	}

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = msg.m_dest_cmpt_id;

	// Update deadline
	OOBase::timeval_t old_deadline = pContext->m_deadline;
	pContext->m_deadline = msg.m_deadline;
	if (deadline && *deadline < pContext->m_deadline)
		pContext->m_deadline = *deadline;

	try
	{
		// Process the message...
		ptrCompt->process_request(msg,pContext->m_deadline);
	}
	catch (...)
	{
		if (v)
		{
			v = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
			if (v)
				*v = old_thread_id;
		}
		else
			pContext->m_mapChannelThreads.erase(msg.m_src_channel_id);

		pContext->m_deadline = old_deadline;
		pContext->m_current_cmpt = old_id;
		throw;
	}

	// Restore old context
	if (v)
	{
		v = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
		if (v)
			*v = old_thread_id;
	}
	else
		pContext->m_mapChannelThreads.erase(msg.m_src_channel_id);

	pContext->m_deadline = old_deadline;
	pContext->m_current_cmpt = old_id;
}

bool OOCore::UserSession::handle_request(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);

	return USER_SESSION::instance().pump_request((timeout ? &wait : 0));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Omega_HandleRequest,1,((in),uint32_t,timeout))
{
	if (OOCore::HostedByOOServer())
	{
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
		if (ptrIPS)
			return ptrIPS->HandleRequest(timeout);
	}
		
	return OOCore::UserSession::handle_request(timeout);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
{
	ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
	return ptrIPS->OpenServerSink(message_oid,pSink);
}

ObjectImpl<OOCore::ComptChannel>* OOCore::UserSession::create_compartment()
{
	return USER_SESSION::instance().create_compartment_i();
}

ObjectImpl<OOCore::ComptChannel>* OOCore::UserSession::create_compartment_i()
{
	// Create the new object
	OOBase::SmartPtr<Compartment> ptrCompt = new (OOCore::throwing) Compartment(this);
	
	// Create a new Compartment object
	OOBase::Guard<OOBase::RWMutex> write_guard(m_lock);

	// Select a new compartment id
	uint16_t cmpt_id;
	int err = m_mapCompartments.insert(ptrCompt,cmpt_id,1,0xFFF);
	if (err != 0)
		OMEGA_THROW(err);

	ptrCompt->set_id(cmpt_id);

	write_guard.release();
	
	return ptrCompt->create_compartment_channel(ThreadContext::instance()->m_current_cmpt,guid_t::Null());
}

OOBase::SmartPtr<OOCore::Compartment> OOCore::UserSession::get_compartment(uint16_t id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<OOCore::Compartment> ptrCompt;
	m_mapCompartments.find(id,ptrCompt);

	return ptrCompt;
}

void OOCore::UserSession::remove_compartment(uint16_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapCompartments.erase(id);
}

uint16_t OOCore::UserSession::update_state(uint16_t compartment_id, uint32_t* pTimeout)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = compartment_id;

	if (pTimeout)
	{
		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		OOBase::timeval_t deadline = pContext->m_deadline;
		if (*pTimeout > 0)
		{
			OOBase::timeval_t deadline2 = now + OOBase::timeval_t(*pTimeout / 1000,(*pTimeout % 1000) * 1000);
			if (deadline2 < deadline)
				deadline = deadline2;
		}

		if (deadline != OOBase::timeval_t::MaxTime)
			*pTimeout = (deadline - now).msec();
	}

	return old_id;
}

IObject* OOCore::UserSession::create_channel(uint32_t src_channel_id, const guid_t& message_oid, const guid_t& iid)
{
	return USER_SESSION::instance().create_channel_i(src_channel_id,message_oid,iid);
}

IObject* OOCore::UserSession::create_channel_i(uint32_t src_channel_id, const guid_t& message_oid, const guid_t& iid)
{
	// Create a channel in the context of the current compartment
	const ThreadContext* pContext = ThreadContext::instance();

	OOBase::SmartPtr<Compartment> ptrCompt;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		if (!m_mapCompartments.find(pContext->m_current_cmpt,ptrCompt))
		{
			// Compartment has gone!
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to find compartment for new channel"));
		}
	}

	switch (classify_channel(src_channel_id))
	{
	case Remoting::Same:
		return LoopChannel::create(src_channel_id,message_oid,iid);

	case Remoting::Compartment:
		{
			ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel = ptrCompt->create_compartment_channel(static_cast<uint16_t>(src_channel_id & 0xFFF),message_oid);
			return ptrChannel->QueryInterface(iid);
		}

	default:
		{
			ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = ptrCompt->create_channel(src_channel_id,message_oid);
			return ptrChannel->QueryInterface(iid);
		}
	}
}
