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
#include "IPS.h"
#include "LoopChannel.h"

using namespace Omega;
using namespace OTL;

OOCore::UserSession::UserSession() :
		m_channel_id(0),
		m_nIPSCookie(0),
		m_next_compartment(0)
{
}

OOCore::UserSession::~UserSession()
{
	// Clear the thread id's of the ThreadContexts
	for (std::map<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
		i->second->m_thread_id = 0;
}

IException* OOCore::UserSession::init(bool bStandalone, const std::map<string_t,string_t>& args)
{
	UserSession* pThis = USER_SESSION::instance();

	// Don't double init...
	if (pThis->m_initcount++ != 0)
		return 0;

#if defined(OMEGA_DEBUG) && defined(_WIN32)
	// If this event exists, then we are being debugged
	OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Local\\OOCORE_DEBUG_MUTEX"));
	if (hDebugEvent)
	{
		// Wait for a bit, letting the caller attach a debugger
		WaitForSingleObject(hDebugEvent,5000);
	}
#endif

	try
	{
		pThis->init_i(bStandalone,args);
	}
	catch (IException* pE)
	{
		term();
		return pE;
	}

	return 0;
}

void OOCore::UserSession::init_i(bool bStandalone, const std::map<string_t,string_t>& args)
{
	bool bStandaloneAlways = false;
	std::map<string_t,string_t>::const_iterator i = args.find(L"standalone_always");
	if (i != args.end() && i->second == L"true")
		bStandaloneAlways = true;

	std::string strPipe;
	if (!bStandaloneAlways)
		strPipe = discover_server_port(bStandalone);

	if (!bStandalone)
	{
		// Connect up to the root process...
		OOBase::timeval_t wait(5);
		OOBase::Countdown countdown(&wait);
		int err = 0;
		do
		{
			m_stream = OOBase::LocalSocket::connect_local(strPipe,&err,&wait);
			if (m_stream)
				break;

			// We ignore the error, and try again until we timeout
			countdown.update();
		}
		while (wait != OOBase::timeval_t::Zero);

		if (!m_stream)
			OMEGA_THROW(err);

		// Send version information
		uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
		err = m_stream->send(version);
		if (err)
			OMEGA_THROW(err);

		// Read our channel id
		err = m_stream->recv(m_channel_id);
		if (err != 0)
			OMEGA_THROW(err);

		// Spawn off the io worker thread
		m_worker_thread.run(io_worker_fn,this);
	}

	// Create the zero compartment
	OOBase::SmartPtr<Compartment> ptrZeroCompt;
	OMEGA_NEW(ptrZeroCompt,Compartment(this,0));

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapCompartments.insert(std::map<uint16_t,OOBase::SmartPtr<Compartment> >::value_type(0,ptrZeroCompt));

	guard.release();

	// Remove standalone support eventually...
	void* TODO;

	ObjectPtr<IInterProcessService> ptrIPS;
	if (!bStandalone)
	{
		// Create a new object manager for the user channel on the zero compartment
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrZeroCompt->get_channel_om(m_channel_id & 0xFF000000);

		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->GetRemoteInstance(OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);

		ptrIPS.Attach(static_cast<IInterProcessService*>(pIPS));
	}
	else
	{
		// Load up OOSvrLite and get the IPS from there...
		int err = m_lite_dll.load("oosvrlite");
		if (err != 0)
			OMEGA_THROW(err);

		typedef const System::Internal::SafeShim* (OMEGA_CALL *pfnOOSvrLite_GetIPS_Safe)(System::Internal::marshal_info<IInterProcessService*&>::safe_type::type OOSvrLite_GetIPS_RetVal, System::Internal::marshal_info<const init_arg_map_t&>::safe_type::type args);

		pfnOOSvrLite_GetIPS_Safe pfn = (pfnOOSvrLite_GetIPS_Safe)(m_lite_dll.symbol("OOSvrLite_GetIPS_Safe"));
		if (!pfn)
			OMEGA_THROW("Corrupt OOSvrLite");

		IInterProcessService* pIPS = 0;
		const System::Internal::SafeShim* pSE = (*pfn)(System::Internal::marshal_info<IInterProcessService*&>::safe_type::coerce(pIPS),System::Internal::marshal_info<const init_arg_map_t&>::safe_type::coerce(args));
		if (pSE)
			System::Internal::throw_correct_exception(pSE);

		ptrIPS.Attach(pIPS);
	}

	// Register locally...
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

	m_nIPSCookie = ptrROT->RegisterObject(OID_InterProcessService,ptrIPS,Activation::ProcessLocal | Activation::MultipleUse);
}

std::string OOCore::UserSession::discover_server_port(bool& bStandalone)
{
#if defined(_WIN32)
	const char* name = "OmegaOnline";

	int err = 0;
	OOBase::SmartPtr<OOBase::LocalSocket> local_socket = OOBase::LocalSocket::connect_local(name,&err);
	if (!local_socket)
	{
		if (bStandalone)
			return std::string();
		else
			throw IInternalException::Create("Failed to connect to network daemon","Omega::Initialize");
	}
	bStandalone = false;

	// Send version information
	uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
	err = local_socket->send(version);
	if (err)
		OMEGA_THROW(err);

	// Read the string length
	uint32_t uLen = 0;
	err = local_socket->recv(uLen);
	if (err)
		OMEGA_THROW(err);

	// Read the string
	OOBase::SmartPtr<char,OOBase::ArrayDestructor<char> > buf = 0;
	OMEGA_NEW(buf,char[uLen]);

	local_socket->recv(buf,uLen,&err);
	if (err)
		OMEGA_THROW(err);

	return std::string(buf);

#elif defined(HAVE_UNISTD_H)

	return std::string(getenv("OMEGA_SESSION_ADDRESS"));

#else
#error Fix me!
#endif
}

void OOCore::UserSession::term()
{
	UserSession* pThis = USER_SESSION::instance();
	if (pThis->m_initcount != 0 && --pThis->m_initcount == 0)
	{
		pThis->term_i();
	}
}

void OOCore::UserSession::term_i()
{
	// Unregister InterProcessService
	if (m_nIPSCookie)
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

		ptrROT->RevokeObject(m_nIPSCookie);
		m_nIPSCookie = 0;
	}

	// Shut down the socket...
	if (m_stream)
		m_stream->close();

	// Wait for the io worker thread to finish
	m_worker_thread.join();

	// Close all compartments
	for (std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator j=m_mapCompartments.begin(); j!=m_mapCompartments.end(); ++j)
	{
		j->second->close();
	}
	m_mapCompartments.clear();

	// Close all singletons
	close_singletons_i();

	// Unload the OOSvrLite dll if loaded
	m_lite_dll.unload();
}

void OOCore::UserSession::close_singletons()
{
	USER_SESSION::instance()->close_singletons_i();
}

void OOCore::UserSession::close_singletons_i()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Copy the list so we can delete outside the lock
	std::list<std::pair<void (OMEGA_CALL*)(void*),void*> > list(m_listUninitCalls);

	m_listUninitCalls.clear();

	guard.release();

	for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=list.begin(); i!=list.end(); ++i)
	{
		(*(i->first))(i->second);
	}
}

Omega::uint32_t OOCore::UserSession::get_channel_id() const
{
	return m_channel_id;
}

void OOCore::UserSession::add_uninit_call(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	USER_SESSION::instance()->add_uninit_call_i(pfn_dctor,param);
}

void OOCore::UserSession::add_uninit_call_i(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_listUninitCalls.push_front(std::pair<void (OMEGA_CALL*)(void*),void*>(pfn_dctor,param));
}

void OOCore::UserSession::remove_uninit_call(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	USER_SESSION::instance()->remove_uninit_call_i(pfn_dctor,param);
}

void OOCore::UserSession::remove_uninit_call_i(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=m_listUninitCalls.begin(); i!=m_listUninitCalls.end(); ++i)
	{
		if (i->first == pfn_dctor && i->second == param)
		{
			m_listUninitCalls.erase(i);
			break;
		}
	}
}

int OOCore::UserSession::io_worker_fn(void* pParam)
{
	try
	{
		return static_cast<UserSession*>(pParam)->run_read_loop();
	}
	catch (std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
		return -1;
	}
}

void OOCore::UserSession::wait_or_alert(const OOBase::AtomicInt<size_t>& usage)
{
	// Make this value configurable somehow...
	OOBase::timeval_t wait(0,500000);
	OOBase::Countdown countdown(&wait);
	do
	{
		// The tinyest sleep
		OOBase::Thread::yield();

		countdown.update();
	}
	while (usage.value() == 0 && wait != OOBase::timeval_t::Zero);

	if (usage.value() == 0)
	{
		// This incoming request may not be processed for some time...
		void* TICKET_91;    // Alert!
	}
}

int OOCore::UserSession::run_read_loop()
{
	static const size_t s_initial_read = sizeof(uint32_t) * 2;
	OOBase::CDRStream header(s_initial_read);

	int err = 0;
	for (;;)
	{
		// Read the header
		header.reset();
		err = m_stream->recv_buffer(header.buffer(),s_initial_read);
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

		err = header.last_error();
		if (err != 0)
			OOBase_CallCriticalFailure(err);

		// Subtract what we have already read
		nReadLen -= s_initial_read;

		// Create a new Message struct
		OOBase::SmartPtr<Message> msg = 0;
		OOBASE_NEW(msg,Message(nReadLen));
		if (!msg)
			OOBase_OutOfMemory();

		// Issue another read for the rest of the data
		err = m_stream->recv_buffer(msg->m_payload.buffer(),nReadLen);
		if (err != 0)
			OOBase_CallCriticalFailure(err);

		// Reset the byte order
		msg->m_payload.big_endian(big_endian);

		// Read the destination and source channels
		uint32_t dest_channel_id;
		msg->m_payload.read(dest_channel_id);
		msg->m_payload.read(msg->m_src_channel_id);

		// Read the deadline
		int64_t req_dline_secs;
		msg->m_payload.read(req_dline_secs);
		int32_t req_dline_usecs;
		msg->m_payload.read(req_dline_usecs);
		msg->m_deadline = OOBase::timeval_t(req_dline_secs,req_dline_usecs);

		// Read the rest of the message
		msg->m_payload.read(msg->m_attribs);
		msg->m_payload.read(msg->m_dest_thread_id);
		msg->m_payload.read(msg->m_src_thread_id);
		msg->m_payload.read(msg->m_seq_no);
		msg->m_payload.read(msg->m_type);

		// Align to the next boundary
		if (msg->m_payload.buffer()->length() > 0)
		{
			// 6 Bytes padding here!
			msg->m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);
		}

		// Did everything make sense?
		err = msg->m_payload.last_error();
		if (err != 0)
			OOBase_CallCriticalFailure(err);

		// Just skip any misdirected packets
		if ((dest_channel_id & 0xFFFFF000) != m_channel_id)
			continue;

		// Unpack the compartment...
		msg->m_dest_cmpt_id = static_cast<uint16_t>(dest_channel_id & 0x00000FFF);

		if ((msg->m_attribs & Message::system_message) && msg->m_type == Message::Request)
		{
			// Process system messages here... because the main message pump may not be running currently
			if ((msg->m_attribs & Message::system_message)==Message::channel_close)
			{
				uint32_t closed_channel_id;
				if (!msg->m_payload.read(closed_channel_id))
					err = msg->m_payload.last_error();
				else
					process_channel_close(closed_channel_id);
			}
			else if ((msg->m_attribs & Message::system_message)==Message::channel_reflect)
			{
				// Send back the src_channel_id
				OOBase::CDRStream response(sizeof(msg->m_src_channel_id));
				if (!response.write(msg->m_src_channel_id))
					err = response.last_error();
				else
					send_response(msg->m_dest_cmpt_id,msg->m_seq_no,msg->m_src_channel_id,msg->m_src_thread_id,&response,msg->m_deadline,Message::synchronous | Message::channel_reflect);
			}
			else if ((msg->m_attribs & Message::system_message)==Message::channel_ping)
			{
				OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

				// Send back 1 byte
				byte_t res = (m_mapCompartments.find(msg->m_dest_cmpt_id) == m_mapCompartments.end() ? 0 : 1);

				guard.release();

				OOBase::CDRStream response(sizeof(byte_t));
				if (!response.write(res))
					err = response.last_error();
				else
					send_response(msg->m_dest_cmpt_id,msg->m_seq_no,msg->m_src_channel_id,msg->m_src_thread_id,&response,msg->m_deadline,Message::synchronous | Message::channel_ping);
			}
		}
		else if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			std::map<uint16_t,ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
			if (i != m_mapThreadContexts.end())
			{
				size_t waiting = i->second->m_usage_count.value();

				OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = i->second->m_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);
				if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success)
				{
					if (waiting == 0)
						wait_or_alert(i->second->m_usage_count);
				}
			}
		}
		else
		{
			// Cannot have a response to 0 thread!
			if (msg->m_type == Message::Request)
			{
				size_t waiting = m_usage_count.value();

				OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = m_default_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);
				if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success)
				{
					if (waiting == 0)
						wait_or_alert(m_usage_count);
				}
			}
		}
	}

	m_stream->close();

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_stream = 0;

	// Tell all worker threads that we are done with them...
	for (std::map<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
	{
		i->second->m_msg_queue.pulse();
	}

	// Stop the message queue
	m_default_msg_queue.close();

	return err;
}

bool OOCore::UserSession::pump_request(const OOBase::timeval_t* wait)
{
	for (;;)
	{
		// Check we still have a receiving stream
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
		if (!m_stream)
			throw Remoting::IChannelClosedException::Create();

		guard.release();

		// Increment the consumers...
		++m_usage_count;

		// Get the next message
		OOBase::SmartPtr<Message> msg;
		OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = m_default_msg_queue.pop(msg,wait);

		// Decrement the consumers...
		--m_usage_count;

		if (res != OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success)
		{
			// We didn't process anything
			return false;
		}

		if (msg->m_type == Message::Request)
		{
			ThreadContext* pContext = ThreadContext::instance();

			// Don't confuse the wait deadline with the message processing deadline
			process_request(pContext,msg,0);

			// Clear the channel/threads map
			pContext->m_mapChannelThreads.clear();

			// We processed something
			return true;
		}
	}
}

void OOCore::UserSession::process_channel_close(uint32_t closed_channel_id)
{
	// Pass on the message to the compartments
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator j=m_mapCompartments.begin(); j!=m_mapCompartments.end(); ++j)
		{
			j->second->process_channel_close(closed_channel_id);
		}
	}
	catch (std::exception&)
	{}

	try
	{
		// Unblock all waiting threads
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
		{
			if (i->second->m_usage_count.value() > 0)
				i->second->m_msg_queue.pulse();
		}
	}
	catch (std::exception&)
	{}
}

OOBase::CDRStream* OOCore::UserSession::wait_for_response(uint32_t seq_no, const OOBase::timeval_t* deadline, uint32_t from_channel_id)
{
	ThreadContext* pContext = ThreadContext::instance();

	OOBase::CDRStream* response = 0;
	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Check we still have a receiving stream
		if (!m_stream)
		{
			// Compartment has gone!
			throw Remoting::IChannelClosedException::Create();
		}

		OOBase::SmartPtr<Compartment> ptrCompt;
		std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i=m_mapCompartments.find(pContext->m_current_cmpt);
		if (i == m_mapCompartments.end())
		{
			// Compartment has gone!
			throw Remoting::IChannelClosedException::Create();
		}
		ptrCompt = i->second;

		guard.release();

		if (!ptrCompt->is_channel_open(from_channel_id))
		{
			// Channel has gone!
			throw Remoting::IChannelClosedException::Create();
		}

		// Increment the usage count
		++pContext->m_usage_count;

		// Get the next message
		OOBase::SmartPtr<Message> msg;
		OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::Result res = pContext->m_msg_queue.pop(msg,const_cast<OOBase::timeval_t*>(deadline));

		// Decrement the usage count
		--pContext->m_usage_count;

		if (res == OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::success)
		{
			if (msg->m_type == Message::Request)
			{
				process_request(pContext,msg,deadline);
			}
			else if (msg->m_type == Message::Response && msg->m_seq_no == seq_no)
			{
				OMEGA_NEW(response,OOBase::CDRStream(msg->m_payload));
				break;
			}
		}
		else if (res != OOBase::BoundedQueue<OOBase::SmartPtr<Message> >::pulsed)
			break;
	}

	return response;
}

OOCore::UserSession::ThreadContext* OOCore::UserSession::ThreadContext::instance()
{
	ThreadContext* pThis = OOBase::TLSSingleton<ThreadContext,OOCore::DLL>::instance();
	if (pThis->m_thread_id == 0)
		pThis->m_thread_id = UserSession::USER_SESSION::instance()->insert_thread_context(pThis);

	return pThis;
}

OOCore::UserSession::ThreadContext::ThreadContext() :
		m_thread_id(0),
		m_deadline(OOBase::timeval_t::MaxTime),
		m_seq_no(0),
		m_current_cmpt(0)
{
}

OOCore::UserSession::ThreadContext::~ThreadContext()
{
	if (m_thread_id)
		UserSession::USER_SESSION::instance()->remove_thread_context(m_thread_id);
}

// Accessors for ThreadContext
uint16_t OOCore::UserSession::insert_thread_context(OOCore::UserSession::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	try
	{
		for (uint16_t i=1; i<=0xFFF; ++i)
		{
			if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
			{
				m_mapThreadContexts.insert(std::map<uint16_t,ThreadContext*>::value_type(i,pContext));
				return i;
			}
		}
	}
	catch (std::exception&)
	{}

	OOBase_CallCriticalFailure("Too many threads");
	return 0;
}

void OOCore::UserSession::remove_thread_context(uint16_t thread_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.erase(thread_id);
}

OOBase::CDRStream* OOCore::UserSession::send_request(uint32_t dest_channel_id, const OOBase::CDRStream* request, uint32_t timeout, uint32_t attribs)
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
		std::map<uint32_t,uint16_t>::const_iterator i=pContext->m_mapChannelThreads.find(dest_channel_id);
		if (i != pContext->m_mapChannelThreads.end())
			dest_thread_id = i->second;

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
	OOBase::CDRStream header = build_header(seq_no,m_channel_id | pContext->m_current_cmpt,src_thread_id,dest_channel_id,dest_thread_id,request,deadline,Message::Request,attribs);

	// Send to the handle
	OOBase::timeval_t wait = deadline;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::gettimeofday();
		if (deadline <= now)
			throw ITimeoutException::Create();

		wait = deadline - now;
	}

	// Check we still have a receiving stream
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	if (!m_stream)
		throw Remoting::IChannelClosedException::Create();

	int err = m_stream->send_buffer(header.buffer(),wait != OOBase::timeval_t::MaxTime ? &wait : 0);
	if (err != 0)
		OMEGA_THROW(err);

	guard.release();

	if (attribs & TypeInfo::Asynchronous)
		return 0;
	else
		// Wait for response...
		return wait_for_response(seq_no,deadline != OOBase::timeval_t::MaxTime ? &deadline : 0,dest_channel_id);
}

void OOCore::UserSession::send_response(Omega::uint16_t src_cmpt_id, uint32_t seq_no, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* response, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	ThreadContext* pContext = ThreadContext::instance();

	// Write the header info
	OOBase::CDRStream header = build_header(seq_no,m_channel_id | src_cmpt_id,pContext->m_thread_id,dest_channel_id,dest_thread_id,response,deadline,Message::Response,attribs);

	OOBase::timeval_t wait = deadline;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::gettimeofday();
		if (deadline <= now)
			throw ITimeoutException::Create();

		wait = deadline - now;
	}

	// Check we still have a receiving stream
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	if (!m_stream)
		throw Remoting::IChannelClosedException::Create();

	int err = m_stream->send_buffer(header.buffer(),wait != OOBase::timeval_t::MaxTime ? &wait : 0);
	if (err != 0)
		OMEGA_THROW(err);
}

OOBase::CDRStream OOCore::UserSession::build_header(uint32_t seq_no, uint32_t src_channel_id, uint16_t src_thread_id, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* msg, const OOBase::timeval_t& deadline, uint16_t flags, uint32_t attribs)
{
	OOBase::CDRStream header(48 + (msg ? msg->buffer()->length() : 0));
	header.write(header.big_endian());
	header.write(byte_t(1));     // version

	// Write out the header length and remember where we wrote it
	header.write(uint32_t(0));
	size_t msg_len_point = header.buffer()->mark_wr_ptr() - sizeof(uint32_t);

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

	if (msg)
	{
		header.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

		// Check the size
		if (msg->buffer()->length() - header.buffer()->length() > 0xFFFFFFFF)
			OMEGA_THROW("Message too big");

		// Write the request stream
		header.write_buffer(msg->buffer());
		if (header.last_error())
			OMEGA_THROW(header.last_error());
	}

	// Update the total length
	header.replace(header.buffer()->length(),msg_len_point);

	return header;
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

void OOCore::UserSession::process_request(ThreadContext* pContext, const Message* pMsg, const OOBase::timeval_t* deadline)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<Compartment> ptrCompt;
	std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i=m_mapCompartments.find(pMsg->m_dest_cmpt_id);
	if (i != m_mapCompartments.end())
		ptrCompt = i->second;

	if (!ptrCompt)
		return;

	guard.release();

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = pMsg->m_dest_cmpt_id;

	// Update deadline
	OOBase::timeval_t old_deadline = pContext->m_deadline;
	pContext->m_deadline = pMsg->m_deadline;
	if (deadline && *deadline < pContext->m_deadline)
		pContext->m_deadline = *deadline;

	try
	{
		// Set per channel thread id
		std::map<uint32_t,uint16_t>::iterator i = pContext->m_mapChannelThreads.insert(std::map<uint32_t,uint16_t>::value_type(pMsg->m_src_channel_id,0)).first;

		uint16_t old_thread_id = i->second;
		i->second = pMsg->m_src_thread_id;

		try
		{
			// Process the message...
			ptrCompt->process_request(pMsg,pContext->m_deadline);
		}
		catch (IException* pOuter)
		{
			// Just drop the exception, and let it pass...
			pOuter->Release();
		}
		catch (...)
		{
			pContext->m_deadline = old_deadline;
			i->second = old_thread_id;
			throw;
		}

		// Restore old context
		i->second = old_thread_id;
	}
	catch (...)
	{
		pContext->m_deadline = old_deadline;
		pContext->m_current_cmpt = old_id;
		throw;
	}

	pContext->m_deadline = old_deadline;
	pContext->m_current_cmpt = old_id;
}

bool OOCore::UserSession::handle_request(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);

	return USER_SESSION::instance()->pump_request((timeout ? &wait : 0));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Omega_HandleRequest,1,((in),uint32_t,timeout))
{
	if (OOCore::HostedByOOServer())
	{
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
		if (!ptrIPS)
			throw IInternalException::Create("Omega::Initialize not called","OOCore");

		return ptrIPS->HandleRequest(timeout);
	}
	else
		return OOCore::UserSession::handle_request(timeout);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const Omega::guid_t&,message_oid,(in),Omega::Remoting::IChannelSink*,pSink))
{
	ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
	if (!ptrIPS)
		throw IInternalException::Create("Omega::Initialize not called","OOCore");

	return ptrIPS->OpenServerSink(message_oid,pSink);
}

ObjectPtr<ObjectImpl<OOCore::ComptChannel> > OOCore::UserSession::create_compartment()
{
	return USER_SESSION::instance()->create_compartment_i();
}

ObjectPtr<ObjectImpl<OOCore::ComptChannel> > OOCore::UserSession::create_compartment_i()
{
	// Create a new Compartment object
	OOBase::SmartPtr<Compartment> ptrCompt;
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Select a new compartment id
		uint16_t cmpt_id;
		do
		{
			cmpt_id = ++m_next_compartment;
			if (cmpt_id > 0xFFF)
				cmpt_id = m_next_compartment = 1;

		}
		while (m_mapCompartments.find(cmpt_id) != m_mapCompartments.end());

		// Create the new object
		OMEGA_NEW(ptrCompt,Compartment(this,cmpt_id));

		// Add it to the map
		m_mapCompartments.insert(std::map<uint16_t,OOBase::SmartPtr<Compartment> >::value_type(cmpt_id,ptrCompt));
	}

	// Now a new ComptChannel for the new compartment connecting to this cmpt
	return ptrCompt->create_compartment(ThreadContext::instance()->m_current_cmpt,guid_t::Null());
}

OOBase::SmartPtr<OOCore::Compartment> OOCore::UserSession::get_compartment(uint16_t id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(id);
	if (i == m_mapCompartments.end())
		throw Remoting::IChannelClosedException::Create();

	return i->second;
}

void OOCore::UserSession::remove_compartment(uint16_t id)
{
	OOBase::SmartPtr<Compartment> ptrCompt;
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(id);
		if (i == m_mapCompartments.end())
			return;

		ptrCompt = i->second;
		m_mapCompartments.erase(i);
	}

	ptrCompt->close();
}

uint16_t OOCore::UserSession::update_state(uint16_t compartment_id, uint32_t* pTimeout)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = compartment_id;

	if (pTimeout)
	{
		OOBase::timeval_t now = OOBase::gettimeofday();
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
	return USER_SESSION::instance()->create_channel_i(src_channel_id,message_oid,iid);
}

IObject* OOCore::UserSession::create_channel_i(uint32_t src_channel_id, const guid_t& message_oid, const guid_t& iid)
{
	// Create a channel in the context of the current compartment
	const ThreadContext* pContext = ThreadContext::instance();

	OOBase::SmartPtr<Compartment> ptrCompt;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::map<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i=m_mapCompartments.find(pContext->m_current_cmpt);
		if (i == m_mapCompartments.end())
		{
			// Compartment has gone!
			throw Remoting::IChannelClosedException::Create();
		}
		ptrCompt = i->second;
	}

	switch (classify_channel(src_channel_id))
	{
	case Remoting::Same:
		return LoopChannel::create(src_channel_id,message_oid,iid);

	case Remoting::Compartment:
		return ptrCompt->create_compartment(static_cast<uint16_t>(src_channel_id & 0xFFF),message_oid)->QueryInterface(iid);

	default:
		return ptrCompt->create_channel(src_channel_id,message_oid)->QueryInterface(iid);
	}
}

Activation::IRunningObjectTable* OOCore::UserSession::get_rot()
{
	return USER_SESSION::instance()->get_rot_i();
}

Activation::IRunningObjectTable* OOCore::UserSession::get_rot_i()
{
	const ThreadContext* pContext = ThreadContext::instance();

	if (pContext->m_current_cmpt != 0)
	{
		OOBase::SmartPtr<OOCore::Compartment> ptrCompt = get_compartment(pContext->m_current_cmpt);

		ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel = ptrCompt->create_compartment(0,guid_t::Null());

		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel->GetObjectManager();

		IObject* pObject = 0;
		ptrOM->GetRemoteInstance(OID_ServiceManager,Activation::ProcessLocal,OMEGA_GUIDOF(Activation::IRunningObjectTable),pObject);
		return static_cast<Activation::IRunningObjectTable*>(pObject);
	}

	return SingletonObjectImpl<ServiceManager>::CreateInstance();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IRunningObjectTable*,OOCore_Activation_GetRunningObjectTable,0,())
{
	return OOCore::UserSession::get_rot();
}
