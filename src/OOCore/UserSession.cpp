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
#include "LocalTransport.h"

#include <signal.h>

using namespace Omega;
using namespace OTL;

namespace
{
	static const size_t s_header_len = sizeof(uint32_t) * 2;

	typedef OOBase::Singleton<OOCore::UserSession,OOCore::DLL> USER_SESSION;
}

namespace OOCore
{
	void CloseSingletons();
}

template class OOBase::TLSSingleton<OOCore::UserSession::ThreadContext,OOCore::DLL>;
template class OOBase::Singleton<OOCore::UserSession,OOCore::DLL>;

OOCore::UserSession::UserSession() :
		m_worker_thread(false),
		m_channel_id(0),
		m_proactor(NULL),
		m_init_count(0),
		m_init_state(eStopped),
		m_usage_count(0)
{
}

OOCore::UserSession::~UserSession()
{
	// Clear the thread id's of the ThreadContexts
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Tell every thread context that we have gone...
	for (OOBase::HandleTable<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
		i->value->m_thread_id = 0;
}

void OOCore::UserSession::init(const byte_t* data, size_t len)
{
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
					start(data,len);

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

void OOCore::UserSession::start(const byte_t* data, size_t len)
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

	// Create the proactor
	int err = 0;
	m_proactor = OOBase::Proactor::create(err);
	if (err)
		OMEGA_THROW(err);

	// Spool off some threads to service the proactor
	err = m_thread_pool.run(&UserSession::io_worker_fn,this,2);
	if (err)
		OMEGA_THROW(err);

	// Create the zero compartment
	OOBase::SmartPtr<Compartment> ptrZeroCompt = new Compartment(this);
	ptrZeroCompt->set_id(0);

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	err = m_mapCompartments.force_insert(0,ptrZeroCompt);
	if (err)
		OMEGA_THROW(err);

	// Register our local channel factory
	m_rot_cookies.push_back(OTL::GetModule()->RegisterAutoObjectFactory<OOCore::ChannelMarshalFactory>());

	guard.release();

#if defined(NDEBUG)
	OOBase::Timeout timeout(15,0);
#else
	OOBase::Timeout timeout;
#endif

	// Now connect to the root process
	OOBase::CDRStream stream;
	if (!data)
		connect_root(stream,timeout);
	else if (!stream.write_bytes(data,len))
		OMEGA_THROW(stream.last_error());

	// Load up the raw transport from the stream
	ObjectPtr<Remoting::ITransport> ptrTransport = create_local_transport(stream,timeout);



	void* TODO; // Finish this off!

	// Spawn off the io worker thread
	//m_worker_thread.run(io_worker_fn,this);

		
	// And unmarshal the new ObjectManager
	ObjectPtr<Remoting::IObjectManager> ptrOM;

	// Create a proxy to the server interface
	IObject* pIPS = NULL;
	ptrOM->GetRemoteInstance(OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);
	ObjectPtr<IInterProcessService> ptrIPS = static_cast<IInterProcessService*>(pIPS);

	// And register the Registry
	ObjectPtr<Registry::IKey> ptrReg;
	ptrReg.GetObject(Registry::OID_Registry_Instance);
	if (ptrReg)
	{
		// Re-register the proxy locally.. it saves a lot of time!
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.GetObject(Activation::OID_RunningObjectTable_Instance);

		guard.acquire();

		m_rot_cookies.push_back(ptrROT->RegisterObject(Registry::OID_Registry_Instance,ptrReg,Activation::ProcessScope));
		m_rot_cookies.push_back(ptrROT->RegisterObject(string_t::constant("Omega.Registry"),ptrReg,Activation::ProcessScope));
	}
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
	sigaddset(&set, SIGPIPE);

	pthread_sigmask(SIG_BLOCK, &set, NULL);

#endif

	int err = 0;
	return (static_cast<UserSession*>(pParam)->m_proactor->run(err) == 1 ? 0 : err);
}

void OOCore::UserSession::term()
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
	try
	{
		// Close compartments
		close_compartments();

		// Revoke our registered objects factories
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.GetObject(Activation::OID_RunningObjectTable_Instance);

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		uint32_t cookie;
		while (m_rot_cookies.pop_back(&cookie))
			ptrROT->RevokeObject(cookie);

		// Now close Initialise singletons
		OOCore::CloseSingletons();
	}
	catch (IException* pE)
	{
		pE->Release();
	}

	// Stop the io thread...
	if (m_stream)
	{
		OOBase::CDRStream header;
		header.write_endianess();
		header.write(byte_t(1));     // version
		header.write(byte_t('C'));   // signature
		header.write(uint32_t(0));
		m_stream->send(header.buffer());
	}

	// Wait for the io worker thread to finish
	OOBase::Timeout wait(5,0);
	if (!m_worker_thread.join(wait))
	{
		m_stream->close();
		m_worker_thread.join();
	}
}

void OOCore::UserSession::close_compartments()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Do these in reverse order...
	OOBase::StackAllocator<128> allocator;
	OOBase::Vector<OOBase::SmartPtr<Compartment>,OOBase::AllocatorInstance> vecCompts(allocator);
	for (OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.begin();i!=m_mapCompartments.end();++i)
		vecCompts.push_back(i->value);

	guard.release();

	for (OOBase::SmartPtr<Compartment> ptrCmpt;vecCompts.pop_back(&ptrCmpt);)
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

Remoting::ITransport* OOCore::UserSession::create_local_transport(OOBase::CDRStream& stream, OOBase::Timeout& timeout)
{
	// Load up the raw transport from the stream
	ObjectPtr<NoLockObjectImpl<OOCore::LocalTransport> > ptrTransport = NoLockObjectImpl<OOCore::LocalTransport>::CreateObject();
	ptrTransport->init(stream,m_proactor,timeout);

	return ptrTransport.AddRef();
}

void OOCore::UserSession::wait_or_alert(const OOBase::Atomic<size_t>& usage)
{
	// Make this value configurable somehow...
	void* ISSUE_9;    // Alert!

	OOBase::Timeout timeout(0,50000);
	do
	{
		// The tiniest sleep
		OOBase::Thread::yield();
	}
	while (usage == 0 && !timeout.has_expired());

	if (usage == 0)
	{
		// This incoming request may not be processed for some time...
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
		header.read_endianess();

		// Read the version byte
		byte_t version = 1;
		header.read(version);
		if (version != 1)
			OOBase_CallCriticalFailure("Invalid protocol version");

		// Room for 1 byte here!

		// Read the length
		uint32_t nReadLen = 0;
		header.read(nReadLen);

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
		msg.m_payload.endianess(header.endianess());

		// Read the destination and source channels
		uint32_t dest_channel_id;
		msg.m_payload.read(dest_channel_id);
		msg.m_payload.read(msg.m_src_channel_id);

		// Read the rest of the message
		msg.m_payload.read(msg.m_attribs);
		msg.m_payload.read(msg.m_dest_thread_id);
		msg.m_payload.read(msg.m_src_thread_id);

		bool t;
		msg.m_payload.read(t);
		msg.m_type = (t ? Message::Request : Message::Response);

		// Align to the next boundary
		if (msg.m_payload.length() > 0)
			msg.m_payload.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);
		
		// Did everything make sense?
		err = msg.m_payload.last_error();
		if (err != 0)
			continue;

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
					send_response_catch(msg,&response);
			}
			else if ((msg.m_attribs & Message::system_message)==Message::channel_ping)
			{
				OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

				// Send back 1 byte
				byte_t res = (m_mapCompartments.exists(msg.m_dest_cmpt_id) ? 1 : 0);

				guard.release();

				OOBase::CDRStream response(sizeof(byte_t));
				if (response.write(res))
					send_response_catch(msg,&response);
			}
		}
		else if (msg.m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			OOBase::HandleTable<uint16_t,ThreadContext*>::iterator i = m_mapThreadContexts.find(msg.m_dest_thread_id);
			if (i != m_mapThreadContexts.end())
			{
				OOBase::BoundedQueue<Message>::Result res = i->value->m_msg_queue.push(msg);
				if (res == OOBase::BoundedQueue<Message>::error)
				{
					err = i->value->m_msg_queue.last_error();
					break;
				}
			}
		}
		else
		{
			// Cannot have a response to 0 thread!
			if (msg.m_type == Message::Request)
			{
				size_t waiting = m_usage_count;

				OOBase::BoundedQueue<Message>::Result res = m_default_msg_queue.push(msg);
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
	for (OOBase::HandleTable<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
		i->value->m_msg_queue.close();

	// Stop the default message queue
	m_default_msg_queue.close();

	return err;
}

bool OOCore::UserSession::pump_request(const OOBase::Timeout& timeout)
{
	for (;;)
	{
		// Increment the consumers...
		++m_usage_count;

		// Get the next message - use a fresh Timeout each time
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = m_default_msg_queue.pop(msg,timeout);

		// Decrement the consumers...
		--m_usage_count;

		switch (res)
		{
		case OOBase::BoundedQueue<Message>::error:
			OOBase_CallCriticalFailure(m_default_msg_queue.last_error());
			break;

		case OOBase::BoundedQueue<Message>::timedout:
			return false;

		case OOBase::BoundedQueue<Message>::closed:
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Omega user process terminated."));

		case OOBase::BoundedQueue<Message>::success:
			if (msg.m_type != Message::Request)
				OOBase_CallCriticalFailure("Response in default queue!");

			process_request(ThreadContext::instance(),msg);

			// We processed something
			return true;

		default:
			break;
		}
	}
}

void OOCore::UserSession::process_channel_close(uint32_t closed_channel_id)
{
	// Pass on the message to the compartments
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	bool bPulse = false;
	for (OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator j=m_mapCompartments.begin(); j!=m_mapCompartments.end(); ++j)
	{
		if (j->value->process_channel_close(closed_channel_id))
			bPulse = true;
	}

	if (bPulse)
	{
		for (OOBase::HandleTable<uint16_t,ThreadContext*>::iterator i=m_mapThreadContexts.begin(); i!=m_mapThreadContexts.end(); ++i)
			i->value->m_msg_queue.pulse();
	}
}

void OOCore::UserSession::wait_for_response(ThreadContext* pContext, OOBase::CDRStream& response, uint32_t from_channel_id)
{
	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		OOBase::SmartPtr<Compartment> ptrCompt;
		OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(pContext->m_current_cmpt);
		if (i != m_mapCompartments.end())
			ptrCompt = i->value;

		guard.release();

		if (!ptrCompt || !ptrCompt->is_channel_open(from_channel_id))
		{
			// Channel has closed
			return respond_exception(response,Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Other compartment closed while waiting for response")));
		}

		// Increment the usage count
		++pContext->m_usage_count;

		// Get the next message
		Message msg;
		OOBase::BoundedQueue<Message>::Result res = pContext->m_msg_queue.pop(msg);

		// Decrement the usage count
		--pContext->m_usage_count;

		switch (res)
		{
		case OOBase::BoundedQueue<Message>::error:
			OOBase_CallCriticalFailure(pContext->m_msg_queue.last_error());
			break;

		case OOBase::BoundedQueue<Message>::success:
			if (msg.m_type == Message::Response)
			{
				response = msg.m_payload;
				return;
			}
			process_request(pContext,msg);
			break;

		case OOBase::BoundedQueue<Message>::closed:
			return respond_exception(response,Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Thread queue closed while waiting for response")));

		default:
			break;
		}
	}
}

OOCore::UserSession::ThreadContext* OOCore::UserSession::ThreadContext::instance()
{
	ThreadContext* pThis = OOBase::TLSSingleton<ThreadContext,OOCore::DLL>::instance();
	if (pThis->m_thread_id == 0)
		pThis->m_thread_id = USER_SESSION::instance().insert_thread_context(pThis);

	return pThis;
}

OOCore::UserSession::ThreadContext::ThreadContext() :
		m_thread_id(0),
		m_usage_count(0),
		m_current_cmpt(0)
{
}

OOCore::UserSession::ThreadContext::~ThreadContext()
{
	if (m_thread_id)
		USER_SESSION::instance().remove_thread_context(m_thread_id);
}

// Accessors for ThreadContext
uint16_t OOCore::UserSession::insert_thread_context(OOCore::UserSession::ThreadContext* pContext)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	uint16_t id = 0;
	int err = m_mapThreadContexts.insert(pContext,id,1,0xFFFF);
	if (err != 0)
		OMEGA_THROW(err);

	return id;
}

void OOCore::UserSession::remove_thread_context(uint16_t thread_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapThreadContexts.remove(thread_id);
}

void OOCore::UserSession::send_request(uint32_t dest_channel_id, OOBase::CDRStream* request, OOBase::CDRStream* response, uint32_t attribs)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t src_thread_id = pContext->m_thread_id;

	// Determine dest_thread_id
	uint16_t dest_thread_id = 0;

	OOBase::HashTable<uint32_t,uint16_t,OOBase::ThreadLocalAllocator>::iterator i = pContext->m_mapChannelThreads.find(dest_channel_id);
	if (i != pContext->m_mapChannelThreads.end())
		dest_thread_id = i->value;

	// Write the header info
	OOBase::CDRStream header;
	build_header(header,m_channel_id | pContext->m_current_cmpt,src_thread_id,dest_channel_id,dest_thread_id,request,Message::Request,attribs);

	// Send to the handle
	int err = 0;
	if (request)
	{
		OOBase::Buffer* bufs[2] = { header.buffer(), request->buffer() };
		err = m_stream->send_v(bufs,2);
	}
	else
		err = m_stream->send(header.buffer());

	if (err != 0)
	{
		ObjectPtr<IException> ptrE = ISystemException::Create(err,OMEGA_CREATE_INTERNAL("Failed to send message buffer"));
		throw Remoting::IChannelClosedException::Create(ptrE);
	}

	if (!(attribs & TypeInfo::Asynchronous))
	{
		// Wait for response...
		wait_for_response(pContext,*response,dest_channel_id);
	}
}

void OOCore::UserSession::send_response_catch(const Message& msg, OOBase::CDRStream* response)
{
	try
	{
		send_response(msg,response);
	}
	catch (IException* pE)
	{
		// Just ignore the error
		pE->Release();
	}
}

void OOCore::UserSession::send_response(const Message& msg, OOBase::CDRStream* response)
{
	ThreadContext* pContext = ThreadContext::instance();

	// Write the header info
	OOBase::CDRStream header;
	build_header(header,m_channel_id | msg.m_dest_cmpt_id,pContext->m_thread_id,msg.m_src_channel_id,msg.m_src_thread_id,response,Message::Response,msg.m_attribs);

	OOBase::Buffer* bufs[2] = { header.buffer(), response->buffer() };
	int err = m_stream->send_v(bufs,2);
	if (err != 0)
	{
		ObjectPtr<IException> ptrE = ISystemException::Create(err,OMEGA_CREATE_INTERNAL("Failed to send message buffer"));
		throw Remoting::IChannelClosedException::Create(ptrE);
	}
}

void OOCore::UserSession::build_header(OOBase::CDRStream& header, uint32_t src_channel_id, uint16_t src_thread_id, uint32_t dest_channel_id, uint16_t dest_thread_id, const OOBase::CDRStream* request, Message::Type type, uint32_t attribs)
{
	header.write_endianess();
	header.write(byte_t(1));     // version
	header.write(byte_t('C'));   // signature

	// Write out the header length and remember where we wrote it
	size_t msg_len_mark = header.buffer()->mark_wr_ptr();
	header.write(uint32_t(0));

	header.write(dest_channel_id);
	header.write(src_channel_id);
	header.write(attribs);
	header.write(dest_thread_id);
	header.write(src_thread_id);
	header.write(type == Message::Request ? true : false);

	if (header.last_error() != 0)
		OMEGA_THROW(header.last_error());

	size_t len = 0;
	if (!request)
		len = header.length() - s_header_len;
	else
	{
		header.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

		len = header.length() - s_header_len;

		size_t request_len = request->length();

		// Check the size
		if (request_len > 0xFFFFFFFF - len)
			OMEGA_THROW("Message too big");

		len += request_len;
	}

	// Update the total length
	header.replace(static_cast<uint32_t>(len),msg_len_mark);
}

void OOCore::UserSession::respond_exception(OOBase::CDRStream& response, IException* pE)
{
	// Make sure the exception is released
	ObjectPtr<IException> ptrE = pE;
	ObjectPtr<ObjectImpl<CDRMessage> > ptrResponse = ObjectImpl<CDRMessage>::CreateObject();

	OOCore_RespondException(ptrResponse,pE);
	
	response = *ptrResponse->GetCDRStream();
}

void OOCore::UserSession::process_request(ThreadContext* pContext, const Message& msg)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<Compartment> ptrCompt;
	OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(msg.m_dest_cmpt_id);
	if (i != m_mapCompartments.end())
		ptrCompt = i->value;

	guard.release();

	if (!ptrCompt)
	{
		if (!(msg.m_attribs & Message::asynchronous))
		{
			OOBase::CDRStream response;
			respond_exception(response,Remoting::IChannelClosedException::Create());
			send_response_catch(msg,&response);
		}
		return;
	}

	// Set per channel thread id
	bool prev_thread_id = false;
	uint16_t old_thread_id = 0;
	OOBase::HashTable<uint32_t,uint16_t,OOBase::ThreadLocalAllocator>::iterator j = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
	if (j != pContext->m_mapChannelThreads.end())
	{
		prev_thread_id = true;
		old_thread_id = j->value;
		j->value = msg.m_src_thread_id;
	}
	else
	{
		int err = pContext->m_mapChannelThreads.insert(msg.m_src_channel_id,msg.m_src_thread_id);
		if (err != 0)
		{
			if (!(msg.m_attribs & Message::asynchronous))
			{
				OOBase::CDRStream response;
				respond_exception(response,ISystemException::Create(err));
				send_response_catch(msg,&response);
			}
			return;
		}
	}

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = msg.m_dest_cmpt_id;

	try
	{
		// Process the message...
		ptrCompt->process_request(msg);
	}
	catch (IException* pE)
	{
		if (!(msg.m_attribs & Message::asynchronous))
		{
			OOBase::CDRStream response;
			respond_exception(response,pE);
			send_response_catch(msg,&response);
		}
		else
		{
			pE->Release();
		}
	}

	// Restore old context
	if (prev_thread_id)
	{
		j = pContext->m_mapChannelThreads.find(msg.m_src_channel_id);
		if (j != pContext->m_mapChannelThreads.end())
			j->value = old_thread_id;
	}
	else
		pContext->m_mapChannelThreads.remove(msg.m_src_channel_id);

	pContext->m_current_cmpt = old_id;
}

ObjectImpl<OOCore::ComptChannel>* OOCore::UserSession::create_compartment(const guid_t& channel_oid)
{
	return USER_SESSION::instance().create_compartment_i(channel_oid);
}

ObjectImpl<OOCore::ComptChannel>* OOCore::UserSession::create_compartment_i(const guid_t& channel_oid)
{
	// Create a new Compartment object
	OOBase::SmartPtr<Compartment> ptrCompt = new Compartment(this);

	OOBase::Guard<OOBase::RWMutex> write_guard(m_lock);

	// Select a new compartment id
	uint16_t cmpt_id;
	int err = m_mapCompartments.insert(ptrCompt,cmpt_id,1,0xFFF);
	if (err != 0)
		OMEGA_THROW(err);

	write_guard.release();

	ptrCompt->set_id(cmpt_id);
	
	return ptrCompt->create_compartment_channel(ThreadContext::instance()->m_current_cmpt,channel_oid);
}

OOBase::SmartPtr<OOCore::Compartment> OOCore::UserSession::get_compartment(uint16_t id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<OOCore::Compartment> ptrCompt;
	OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(id);
	if (i != m_mapCompartments.end())
		ptrCompt = i->value;

	return ptrCompt;
}

void OOCore::UserSession::remove_compartment(uint16_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapCompartments.remove(id);
}

uint16_t OOCore::UserSession::update_state(uint16_t compartment_id)
{
	ThreadContext* pContext = ThreadContext::instance();

	uint16_t old_id = pContext->m_current_cmpt;
	pContext->m_current_cmpt = compartment_id;

	return old_id;
}

IObject* OOCore::UserSession::create_channel(uint32_t src_channel_id, const guid_t& message_oid, const guid_t& iid, Remoting::MarshalFlags_t flags)
{
	return USER_SESSION::instance().create_channel_i(src_channel_id,message_oid,iid,flags);
}

IObject* OOCore::UserSession::create_channel_i(uint32_t src_channel_id, const guid_t& message_oid, const guid_t& iid, Remoting::MarshalFlags_t flags)
{
	// Create a channel in the context of the current compartment
	const ThreadContext* pContext = ThreadContext::instance();

	OOBase::SmartPtr<Compartment> ptrCompt;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		OOBase::HandleTable<uint16_t,OOBase::SmartPtr<Compartment> >::iterator i = m_mapCompartments.find(pContext->m_current_cmpt);
		if (i == m_mapCompartments.end())
		{
			// Compartment has gone!
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to find compartment for new channel"));
		}

		ptrCompt = i->value;
	}

	switch (flags)
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
			ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = ptrCompt->create_channel(src_channel_id,message_oid,flags);
			return ptrChannel->QueryInterface(iid);
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Initialize,3,((in),uint32_t,version,(in),const byte_t*,data,(in),size_t,len))
{
	// Check the versions are correct
	if (version > ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
		throw IInternalException::Create(OOCore::get_text("This component requires a later version of OOCore"),"Omega::Initialize");

	USER_SESSION::instance().init(data,len);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
{
	USER_SESSION::instance().term();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Omega_HandleRequest,1,((in),uint32_t,millisecs))
{
	OOBase::Timeout timeout;
	if (millisecs != 0)
		timeout = OOBase::Timeout(millisecs/1000,(millisecs % 1000) * 1000);

	return USER_SESSION::instance().pump_request(timeout);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::ITransport*,OOCore_CreateLocalTransport,3,((in),const byte_t*,data,(in),size_t,len,(in),uint32_t,millisecs))
{
	OOBase::CDRStream stream;
	if (!stream.write_bytes(data,len))
		OMEGA_THROW(stream.last_error());

	OOBase::Timeout timeout;
	if (millisecs != 0)
		timeout = OOBase::Timeout(millisecs/1000,(millisecs % 1000) * 1000);

	return USER_SESSION::instance().create_local_transport(stream,timeout);
}
