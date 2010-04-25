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
	m_next_apartment(0)
{
}

OOCore::UserSession::~UserSession()
{
	// Clear the thread id's of the ThreadContexts
	for (std::map<Omega::uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		i->second->m_thread_id = 0;
}

IException* OOCore::UserSession::init(bool bStandalone)
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
		pThis->init_i(bStandalone);
	}
	catch (IException* pE)
	{
		term();
		return pE;
	}

	return 0;
}

void OOCore::UserSession::init_i(bool bStandalone)
{
	std::string strPipe = discover_server_port(bStandalone);
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
			OOBase::sleep(OOBase::timeval_t(0,100000));

			countdown.update();
		} while (wait != OOBase::timeval_t::Zero);

		if (!m_stream)
			OMEGA_THROW(err);

		// Read our channel id
		err = m_stream->recv(m_channel_id);
		if (err != 0)
			OMEGA_THROW(err);

		countdown.update();

		// Spawn off the io worker thread
		m_worker_thread.run(io_worker_fn,this);
	}

	// Create the zero apartment
	OOBase::SmartPtr<Apartment> ptrZeroApt;
	OMEGA_NEW(ptrZeroApt,Apartment(this,0));

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapApartments.insert(std::map<uint16_t,OOBase::SmartPtr<Apartment> >::value_type(0,ptrZeroApt));
	
	guard.release();

	// Remove standalone support eventually...
	void* TODO;

	ObjectPtr<IInterProcessService> ptrIPS;
	if (!bStandalone)
	{
		// Create a new object manager for the user channel on the zero apartment
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrZeroApt->get_channel_om(m_channel_id & 0xFF000000);

		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->GetRemoteInstance(OID_InterProcessService.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);

		ptrIPS.Attach(static_cast<IInterProcessService*>(pIPS));
	}
	else
	{
		// Load up OOSvrLite and get the IPS from there...
		int err = m_lite_dll.load("oosvrlite");
		if (err != 0)
			OMEGA_THROW(err);

		typedef const System::Internal::SafeShim* (OMEGA_CALL *pfnOOSvrLite_GetIPS_Safe)(const Omega::System::Internal::SafeShim** retval);

		pfnOOSvrLite_GetIPS_Safe pfn = (pfnOOSvrLite_GetIPS_Safe)(m_lite_dll.symbol("OOSvrLite_GetIPS_Safe"));
		if (!pfn)
			OMEGA_THROW(L"Corrupt OOSvrLite");

		IInterProcessService* pIPS = 0;
		const Omega::System::Internal::SafeShim* pSE = (*pfn)(System::Internal::marshal_info<IInterProcessService*&>::safe_type::coerce(pIPS));
		if (pSE)
			Omega::System::Internal::throw_correct_exception(pSE);

		ptrIPS.Attach(pIPS);
	}

	// Register locally...
	m_nIPSCookie = Activation::RegisterObject(OID_InterProcessService,ptrIPS,Activation::InProcess,Activation::MultipleUse);
}

std::string OOCore::UserSession::discover_server_port(bool& bStandalone)
{
#if defined(_WIN32)
	const char* name = "OOServer";
#elif defined(HAVE_UNISTD_H)
	const char* name = "/tmp/omegaonline/ooserverd";
#else
#error Fix me!
#endif

	OOBase::SmartPtr<OOBase::LocalSocket> local_socket;
	OOBase::timeval_t wait(5);
	OOBase::Countdown countdown(&wait);
	int err = 0;
	while (wait != OOBase::timeval_t::Zero)
	{
		local_socket = OOBase::LocalSocket::connect_local(name,&err,&wait);
		if (local_socket || bStandalone)
			break;

		// We ignore the error, and try again until we timeout
		OOBase::sleep(OOBase::timeval_t(0,10000));

		countdown.update();
	}
	if (!local_socket)
	{
		if (bStandalone)
			return std::string();
		else
			throw Omega::ISystemException::Create(L"Failed to connect to network daemon",L"Omega::Initialize");
	}
	bStandalone = false;

	countdown.update();

	// Send nothing, but we must send...
	uint32_t duff = 0;
	err = local_socket->send(duff);
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

	countdown.update();

	local_socket->recv(buf,uLen,&err);
	if (err)
		OMEGA_THROW(err);

	return std::string(buf);
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
	    Activation::RevokeObject(m_nIPSCookie);
		m_nIPSCookie = 0;
	}

	// Shut down the socket...
	if (m_stream)
		m_stream->close();

	// Wait for the io worker thread to finish
	m_worker_thread.join();

	// Close all apartments
	for (std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator j=m_mapApartments.begin();j!=m_mapApartments.end();++j)
	{
		j->second->close();
	}
	m_mapApartments.clear();

	// Close all singletons
	close_singletons();

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

	for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=list.begin();i!=list.end();++i)
	{
		(*(i->first))(i->second);
	}
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

	for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=m_listUninitCalls.begin();i!=m_listUninitCalls.end();++i)
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
	return static_cast<UserSession*>(pParam)->run_read_loop();
}

int OOCore::UserSession::run_read_loop()
{
	static const size_t s_initial_read = sizeof(Omega::uint32_t) * 2;
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
		Omega::int64_t req_dline_secs;
		msg->m_payload.read(req_dline_secs);
		Omega::int32_t req_dline_usecs;
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

		// Unpack the apartment...
		msg->m_apartment_id = static_cast<uint16_t>(dest_channel_id & 0x00000FFF);

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
					send_response(msg->m_apartment_id,msg->m_seq_no,msg->m_src_channel_id,msg->m_src_thread_id,&response,msg->m_deadline,Message::synchronous | Message::channel_reflect);
			}
		}
		else if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			try
			{
				std::map<uint16_t,ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
				if (i == m_mapThreadContexts.end())
					err = EACCES;
				else
				{
					size_t waiting = i->second->m_usage_count.value();

					OOBase::BoundedQueue<Message*>::Result res = i->second->m_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);
					if (res == OOBase::BoundedQueue<Message*>::success)
					{
						msg.detach();

						if (waiting == 0)
						{
							// This incoming request may not be processed for some time...
							void* TICKET_91;	// Alert!
						}
					}
				}
			}
			catch (std::exception& e)
			{
				OOBase_CallCriticalFailure(e.what());
			}
		}
		else
		{
			// Cannot have a response to 0 thread!
			if (msg->m_type == Message::Request)
			{
				size_t waiting = m_usage_count.value();

				OOBase::BoundedQueue<Message*>::Result res = m_default_msg_queue.push(msg,msg->m_deadline==OOBase::timeval_t::MaxTime ? 0 : &msg->m_deadline);
				if (res == OOBase::BoundedQueue<Message*>::success)
				{
					msg.detach();

					if (waiting == 0)
					{
						// This incoming request may not be processed for some time...
						void* TICKET_91;	// Alert!
					}
				}
			}
		}
	}

	m_stream->close();

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_stream = 0;

	// Tell all worker threads that we are done with them...
	for (std::map<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
	{
		i->second->m_msg_queue.pulse();
	}

	// Stop the message queue
	m_default_msg_queue.close();

	return err;
}

bool OOCore::UserSession::pump_requests(const OOBase::timeval_t* wait, bool bOnce)
{
	bool timedout = false;

	// Check we still have a receiving stream
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	if (!m_stream)
		throw Remoting::IChannelClosedException::Create();

	guard.release();

	// Increment the consumers...
	++m_usage_count;

	ThreadContext* pContext = ThreadContext::instance();

	do
	{
		// Get the next message
		Message* message = 0;
		OOBase::BoundedQueue<Message*>::Result res = m_default_msg_queue.pop(message,wait);
		if (res != OOBase::BoundedQueue<Message*>::success)
		{
			timedout = true;
			break;
		}

		OOBase::SmartPtr<Message> msg = message;

		// Set deadline
		// Dont confuse the wait deadline with the message processing deadline
		OOBase::timeval_t old_deadline = pContext->m_deadline;
		pContext->m_deadline = msg->m_deadline;

		try
		{
			// Set per channel thread id
			pContext->m_mapChannelThreads.insert(std::map<uint32_t,uint16_t>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
		}
		catch (std::exception&)
		{
			// This shouldn't ever occur, but that means it will ;)
			pContext->m_deadline = old_deadline;
			continue;
		}

		// Process the message...
		process_request(msg,pContext->m_deadline);

		// Clear the thread map
		pContext->m_mapChannelThreads.clear();

		// Reset the deadline
		pContext->m_deadline = old_deadline;

	} while (!bOnce);

	// Decrement the consumers...
	--m_usage_count;

	return timedout;
}

void OOCore::UserSession::process_channel_close(uint32_t closed_channel_id)
{
	// Pass on the message to the apartments
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator j=m_mapApartments.begin();j!=m_mapApartments.end();++j)
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

		for (std::map<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		{
			if (i->second->m_usage_count.value() > 0)
				i->second->m_msg_queue.pulse();
		}
	}
	catch (std::exception&)
	{}
}

OOBase::CDRStream* OOCore::UserSession::wait_for_response(uint16_t apartment_id, uint32_t seq_no, const OOBase::timeval_t* deadline, uint32_t from_channel_id)
{
	OOBase::CDRStream* response = 0;
	ThreadContext* pContext = ThreadContext::instance();

	// Increment the usage count
	++pContext->m_usage_count;

	try
	{
		for (;;)
		{
			// Check if the channel we are waiting on is still valid...
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			// Check we still have a receiving stream
			if (!m_stream)
			{
				// Apartment has gone!
				throw Remoting::IChannelClosedException::Create();
			}

			OOBase::SmartPtr<Apartment> ptrApt;
			std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator i=m_mapApartments.find(apartment_id);
			if (i == m_mapApartments.end())
			{
				// Apartment has gone!
				throw Remoting::IChannelClosedException::Create();
			}
			ptrApt = i->second;

			guard.release();

			if (!ptrApt->is_channel_open(from_channel_id))
			{
				// Channel has gone!
				throw Remoting::IChannelClosedException::Create();
			}

			// Get the next message
			Message* message = 0;
			OOBase::BoundedQueue<Message*>::Result res = pContext->m_msg_queue.pop(message,const_cast<OOBase::timeval_t*>(deadline));
			if (res == OOBase::BoundedQueue<Message*>::success)
			{
				OOBase::SmartPtr<Message> msg = message;

				if (msg->m_type == Message::Request)
				{
					// Update deadline
					OOBase::timeval_t old_deadline = pContext->m_deadline;
					pContext->m_deadline = msg->m_deadline;
					if (deadline && *deadline < pContext->m_deadline)
						pContext->m_deadline = *deadline;

					// Set per channel thread id
					std::map<uint32_t,uint16_t>::iterator i;
					try
					{
						i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
						if (i == pContext->m_mapChannelThreads.end())
							i = pContext->m_mapChannelThreads.insert(std::map<uint32_t,uint16_t>::value_type(msg->m_src_channel_id,0)).first;
					}
					catch (std::exception&)
					{
						// This shouldn't ever occur, but that means it will ;)
						pContext->m_deadline = old_deadline;
						continue;
					}

					uint16_t old_thread_id = i->second;
					i->second = msg->m_src_thread_id;

					// Process the message...
					process_request(msg,pContext->m_deadline);

					// Restore old context
					pContext->m_deadline = old_deadline;
					i->second = old_thread_id;
				}
				else if (msg->m_type == Message::Response && msg->m_seq_no == seq_no)
				{
					OMEGA_NEW(response,OOBase::CDRStream(msg->m_payload));
					break;
				}
			}
			else if (res != OOBase::BoundedQueue<Message*>::pulsed)
				break;
		}
	}
	catch (...)
	{
		// Decrement the usage count
		--pContext->m_usage_count;

		throw;
	}

	// Decrement the usage count
	--pContext->m_usage_count;

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
	m_current_apt(0)
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
		for (uint16_t i=1;i<=0xFFF;++i)
		{
			if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
			{
				m_mapThreadContexts.insert(std::map<uint16_t,ThreadContext*>::value_type(i,pContext));
				return i;
			}
		}
	}
	catch (std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}

	OOBase_CallCriticalFailure("Too many threads");
	return 0;
}

void OOCore::UserSession::remove_thread_context(uint16_t thread_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.erase(thread_id);
}

OOBase::CDRStream* OOCore::UserSession::send_request(uint16_t src_apartment_id, uint32_t dest_channel_id, const OOBase::CDRStream* request, uint32_t timeout, uint32_t attribs)
{
	uint16_t src_thread_id = 0;
	uint16_t dest_thread_id = 0;
	OOBase::timeval_t deadline = OOBase::timeval_t::MaxTime;

	uint32_t seq_no = 0;

	// Only use thread context if we are a synchronous call
	if (!(attribs & Message::asynchronous))
	{
		ThreadContext* pContext = ThreadContext::instance();

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
	OOBase::CDRStream header = build_header(seq_no,m_channel_id | src_apartment_id,src_thread_id,dest_channel_id,dest_thread_id,request,deadline,Message::Request,attribs);

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
		return wait_for_response(src_apartment_id,seq_no,deadline != OOBase::timeval_t::MaxTime ? &deadline : 0,dest_channel_id);
}

void OOCore::UserSession::send_response(uint16_t apartment_id, uint32_t seq_no, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* response, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	// Write the header info
	OOBase::CDRStream header = build_header(seq_no,m_channel_id | apartment_id,ThreadContext::instance()->m_thread_id,dest_channel_id,dest_thread_id,response,deadline,Message::Response,attribs);

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
	header.write(byte_t(1));	 // version

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
			OMEGA_THROW(L"Message too big");

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
		mflags = Remoting::Apartment;
	else if ((channel & 0xFF000000) == (m_channel_id & 0xFF000000))
		mflags = Remoting::InterProcess;
	else if ((channel & 0x80000000) == (m_channel_id & 0x80000000))
		mflags = Remoting::InterUser;
	else
		mflags = Remoting::RemoteMachine;

	return mflags;
}

void OOCore::UserSession::process_request(const Message* pMsg, const OOBase::timeval_t& deadline)
{
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		OOBase::SmartPtr<Apartment> ptrApt;
		std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator i=m_mapApartments.find(pMsg->m_apartment_id);
		if (i != m_mapApartments.end())
			ptrApt = i->second;

		guard.release();

		if (ptrApt)
			ptrApt->process_request(pMsg,deadline);
	}
	catch (std::exception&)
	{
		// Ignore
	}
	catch (IException* pOuter)
	{
		// Just drop the exception, and let it pass...
		pOuter->Release();
	}
}

bool OOCore::UserSession::handle_request(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);

	return USER_SESSION::instance()->pump_requests((timeout ? &wait : 0),false);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Omega_HandleRequest,1,((in),uint32_t,timeout))
{
	if (OOCore::HostedByOOServer())
		return OOCore::GetInterProcessService()->HandleRequest(timeout);
	else
		return OOCore::UserSession::handle_request(timeout);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const Omega::guid_t&,message_oid,(in),Omega::Remoting::IChannelSink*,pSink))
{
	return OOCore::GetInterProcessService()->OpenServerSink(message_oid,pSink);
}

Apartment::IApartment* OOCore::UserSession::create_apartment()
{
	return USER_SESSION::instance()->create_apartment_i();
}

Apartment::IApartment* OOCore::UserSession::create_apartment_i()
{
	// Create a new Apartment object
	OOBase::SmartPtr<Apartment> ptrApt;
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Select a new apartment id
		uint16_t apt_id;
		do
		{
			apt_id = ++m_next_apartment;
			if (apt_id > 0xFFF)
				apt_id = m_next_apartment = 1;

		} while (m_mapApartments.find(apt_id) != m_mapApartments.end());
		
		// Create the new object
		OMEGA_NEW(ptrApt,Apartment(this,apt_id));

		// Add it to the map
		m_mapApartments.insert(std::map<uint16_t,OOBase::SmartPtr<Apartment> >::value_type(apt_id,ptrApt));
	}

	// Now a new OM for the new apartment connecting to the zero apt
	ObjectPtr<Remoting::IObjectManager> ptrOM = ptrApt->get_apartment_om(0);

	// Now get the new apartment OM to create an IApartment
	IObject* pObject = 0;
	ptrOM->GetRemoteInstance(OID_StdApartment.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(Omega::Activation::IObjectFactory),pObject);
	ObjectPtr<Activation::IObjectFactory> ptrOF;
	ptrOF.Attach(static_cast<Activation::IObjectFactory*>(pObject));
	
	pObject = 0;
	ptrOF->CreateInstance(0,OMEGA_GUIDOF(Omega::Apartment::IApartment),pObject);

	return static_cast<Omega::Apartment::IApartment*>(pObject);
}

OOBase::SmartPtr<OOCore::Apartment> OOCore::UserSession::get_apartment(uint16_t id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator i = m_mapApartments.find(id);
	if (i == m_mapApartments.end())
		throw Remoting::IChannelClosedException::Create();

	return i->second;
}

uint16_t OOCore::UserSession::get_current_apartment()
{
	return ThreadContext::instance()->m_current_apt;
}

void OOCore::UserSession::remove_apartment(uint16_t id)
{
	UserSession* pThis = USER_SESSION::instance();

	OOBase::SmartPtr<Apartment> ptrApt;
	{
		OOBase::Guard<OOBase::RWMutex> guard(pThis->m_lock);

		std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator i = pThis->m_mapApartments.find(id);
		if (i == pThis->m_mapApartments.end())
			return;

		ptrApt = i->second;
		pThis->m_mapApartments.erase(i);
	}

	ptrApt->close();
}

uint16_t OOCore::UserSession::update_state(uint16_t apartment_id, uint32_t* pTimeout)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t old_id = pContext->m_current_apt;
	pContext->m_current_apt = apartment_id;

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
	// Create a channel in the context of the current apartment
	const ThreadContext* pContext = ThreadContext::instance();
	
	OOBase::SmartPtr<Apartment> ptrApt;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::map<uint16_t,OOBase::SmartPtr<Apartment> >::iterator i=m_mapApartments.find(pContext->m_current_apt);
		if (i == m_mapApartments.end())
		{
			// Apartment has gone!
			throw Remoting::IChannelClosedException::Create();
		}
		ptrApt = i->second;
	}

	switch (classify_channel(src_channel_id))
	{
	case Remoting::Same:
		return LoopChannel::create(src_channel_id,message_oid,iid);

	case Remoting::Apartment:
		return ptrApt->create_apartment(static_cast<uint16_t>(src_channel_id & 0xFFF),message_oid)->QueryInterface(iid);

	default:
		return ptrApt->create_channel(src_channel_id,message_oid)->QueryInterface(iid);
	} 
}
