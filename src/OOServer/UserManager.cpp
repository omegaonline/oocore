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

#include "OOServer_User.h"
#include "UserManager.h"
#include "InterProcessService.h"
#include "Channel.h"

#if defined(HAVE_EV_H)
#include <ev.h>
#endif

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#if defined(HAVE_SYS_FCNTL_H)
#include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

namespace OTL
{
	// The following is an expansion of BEGIN_PROCESS_OBJECT_MAP
	// We don't use the macro as we overide some behaviours
	namespace Module
	{
		class OOSvrUser_ProcessModuleImpl : public ProcessModule
		{
		private:
			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(User::ChannelMarshalFactory)
					{ 0,0,0,0 }
				};
				return CreatorEntries;
			}
		};

		OMEGA_PRIVATE_FN_DECL(Module::OOSvrUser_ProcessModuleImpl*,GetModule())
		{
			return Omega::Threading::Singleton<Module::OOSvrUser_ProcessModuleImpl,Omega::Threading::InitialiseDestructor<User::Module> >::instance();
		}

		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)()
		{
			return OMEGA_PRIVATE_FN_CALL(GetModule)();
		}
	}
}

using namespace Omega;
using namespace OTL;

// UserManager
User::Manager::Manager() :
		m_nIPSCookie(0),
		m_bIsSandbox(false),
		m_nNextRemoteChannel(0)
{
}

User::Manager::~Manager()
{
}

int User::Manager::run(const std::string& strPipe)
{
	return USER_MANAGER::instance()->run_i(strPipe);
}

int User::Manager::run_i(const std::string& strPipe)
{
	// Start the handler and init ourselves
	if (!start_request_threads())
		return EXIT_FAILURE;

	int res = EXIT_FAILURE;
	if (init(strPipe))
	{
		res = EXIT_SUCCESS;

		// Wait for stop
		wait_for_quit();

		// Stop accepting new clients
		m_acceptor.stop();

		// Close all the sinks
		close_all_remotes();

		// Close the user pipes
		close_channels();

		// Unregister our object factories
		GetModule()->UnregisterObjectFactories();

		// Unregister InterProcessService
		if (m_nIPSCookie)
		{
			ObjectPtr<Activation::IRunningObjectTable> ptrROT;
			ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

			ptrROT->RevokeObject(m_nIPSCookie);
			m_nIPSCookie = 0;
		}

		// Close the OOCore
		Omega::Uninitialize();
	}

	// Close the proactor
	//Proactor::close();

	// Stop the MessageHandler
	stop_request_threads();

	return res;
}

bool User::Manager::on_channel_open(Omega::uint32_t channel)
{
	if (channel != m_root_channel)
	{
		try
		{
			create_object_manager(channel,guid_t::Null());
		}
		catch (IException* pE)
		{
			LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_str()));
			pE->Release();
			return false;
		}
	}
	return true;
}

bool User::Manager::init(const std::string& strPipe)
{
	// Connect to the root
	OOBase::timeval_t wait(20);
	OOBase::Countdown countdown(&wait);

	int err = 0;
#if defined(_WIN32)
	// Use a named pipe
	OOBase::SmartPtr<OOBase::LocalSocket> local_socket = OOBase::LocalSocket::connect_local(strPipe,&err,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	countdown.update();
#else
	// Use the passed fd
	int fd = atoi(strPipe.c_str());

	// Add FD_CLOEXEC to fd
	int oldflags = fcntl(fd,F_GETFD);
	if (oldflags == -1 ||
			fcntl(fd,F_SETFD,oldflags | FD_CLOEXEC) == -1)
	{
		LOG_ERROR_RETURN(("fcntl() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);
	}

	OOBase::POSIX::LocalSocket* pLocal = 0;
	OOBASE_NEW(pLocal,OOBase::POSIX::LocalSocket(fd,""));
	if (!pLocal)
	{
		::close(fd);
		LOG_ERROR_RETURN(("Out of memory"),false);
	}
	OOBase::SmartPtr<OOBase::LocalSocket> local_socket(pLocal);
#endif

	// Read the sandbox channel
	Omega::uint32_t sandbox_channel = 0;
	err = local_socket->recv(sandbox_channel,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Set the sandbox flag
	m_bIsSandbox = (sandbox_channel == 0);

	// Invent a new pipe name...
	std::string strNewPipe = Acceptor::unique_name();
	if (strNewPipe.empty())
		return false;

	countdown.update();

	// Then send back our port name
	size_t uLen = strNewPipe.length()+1;
	err = local_socket->send(uLen,&wait);
	if (err == 0)
		err = local_socket->send(strNewPipe.c_str(),uLen);

	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Read our channel id
	Omega::uint32_t our_channel = 0;
	err = local_socket->recv(our_channel);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Init our channel id
	set_channel(our_channel,0xFF000000,0x00FFF000,m_root_channel);

	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
	OOBASE_NEW(ptrMC,OOServer::MessageConnection(this));
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Attach it to ourselves
	if (register_channel(ptrMC,m_root_channel) == 0)
		return false;

	countdown.update();

	// Open the root connection
	ptrMC->attach(Proactor::instance()->attach_socket(ptrMC,&err,local_socket));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Start I/O with root
	if (!ptrMC->read())
	{
		ptrMC->close();
		return false;
	}

	// Now bootstrap
	OOBase::CDRStream bs;
	bs.write(sandbox_channel);
	bs.write(strNewPipe);
	if (bs.last_error() != 0)
		LOG_ERROR_RETURN(("Failed to write bootstrap data: %s",OOSvrBase::Logger::format_error(bs.last_error()).c_str()),false);

	if (!call_async_function_i(&do_bootstrap,this,&bs))
		return false;

	return true;
}

void User::Manager::do_bootstrap(void* pParams, OOBase::CDRStream& input)
{
	Omega::uint32_t sandbox_channel = 0;
	input.read(sandbox_channel);
	std::string strNewPipe;
	input.read(strNewPipe);
	if (input.last_error() != 0)
	{
		LOG_ERROR(("Failed to read bootstrap data: %s",OOSvrBase::Logger::format_error(input.last_error()).c_str()));
		quit();
	}
	else
	{
		Manager* pThis = static_cast<Manager*>(pParams);

		if (!pThis->bootstrap(sandbox_channel) ||
				!pThis->m_acceptor.start(pThis,strNewPipe))
		{
			quit();
		}
	}
}

bool User::Manager::bootstrap(Omega::uint32_t sandbox_channel)
{
	// Register our service
	try
	{
		ObjectPtr<Remoting::IObjectManager> ptrOMSb;
		if (sandbox_channel != 0)
			ptrOMSb = create_object_manager(sandbox_channel,guid_t::Null());

		ObjectPtr<Remoting::IObjectManager> ptrOMUser;

		ObjectPtr<ObjectImpl<InterProcessService> > ptrIPS = ObjectImpl<InterProcessService>::CreateInstancePtr();
		ptrIPS->Init(ptrOMSb,ptrOMUser,this);

		// Register our interprocess service InProcess so we can react to activation requests
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

		m_nIPSCookie = ptrROT->RegisterObject(OOCore::OID_InterProcessService,ptrIPS,Activation::ProcessLocal | Activation::MultipleUse);

		// Now we have a ROT, register everything else
		GetModule()->RegisterObjectFactories();

		return true;
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_str()));
		pE->Release();

		return false;
	}
}

bool User::Manager::on_accept(OOBase::Socket* sock)
{
	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
	OOBASE_NEW(ptrMC,OOServer::MessageConnection(this));
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Attach it to ourselves
	uint32_t channel_id = register_channel(ptrMC,0);
	if (channel_id == 0)
		return false;

	// Send the channel id...
	int err = sock->send(channel_id);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to socket: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Attach the connection
	ptrMC->attach(Proactor::instance()->attach_socket(ptrMC,&err,static_cast<OOBase::LocalSocket*>(sock)));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Start I/O
	if (!ptrMC->read())
	{
		ptrMC->close();
		return false;
	}

	return true;
}

void User::Manager::on_channel_closed(Omega::uint32_t channel)
{
	// Close the corresponding Object Manager
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin(); i!=m_mapChannels.end();)
		{
			bool bErase = false;
			if (i->first == channel)
			{
				// Close if its an exact match
				bErase = true;
			}
			else if (!(channel & 0xFFF) && (i->first & 0xFFFFF000) == channel)
			{
				// Close all apartments if 0 apt dies
				bErase = true;
			}
			else if (channel == m_root_channel && classify_channel(i->first) > 2)
			{
				// If the root channel closes, close all upstream OMs
				bErase = true;
			}

			if (bErase)
			{
				i->second->disconnect();
				m_mapChannels.erase(i++);
			}
			else
				++i;
		}
	}
	catch (...)
	{}

	// Give the remote layer a chance to close channels
	local_channel_closed(channel);

	// If the root closes, we should end!
	if (channel == m_root_channel)
		quit();
}

void User::Manager::process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
{
	if (src_channel_id == m_root_channel)
		process_root_request(request,seq_no,src_thread_id,deadline,attribs);
	else
		process_user_request(request,seq_no,src_channel_id,src_thread_id,deadline,attribs);
}

void User::Manager::process_root_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
{
	OOServer::RootOpCode_t op_code;
	if (!request.read(op_code))
	{
		LOG_ERROR(("Bad request: %s",OOSvrBase::Logger::format_error(request.last_error()).c_str()));
		return;
	}

	OOBase::CDRStream response;
	switch (op_code)
	{
	case 0:
	default:
		response.write((int)EINVAL);
		LOG_ERROR(("Bad request op_code: %u",op_code));
		break;
	}

	if (!(attribs & TypeInfo::Asynchronous) && response.last_error()==0)
	{
		OOServer::MessageHandler::io_result::type res = send_response(seq_no,m_root_channel,src_thread_id,response,deadline,attribs);
		if (res == OOServer::MessageHandler::io_result::failed)
			LOG_ERROR(("Root response sending failed"));
	}
}

void User::Manager::process_user_request(const OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id,guid_t::Null());
		if (!ptrOM)
			return;

		// QI for IMarshaller
		ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
		if (!ptrMarshaller)
			return;

		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope;
		ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
		ptrEnvelope->init(request);

		// Unpack the payload
		ObjectPtr<Remoting::IMessage> ptrRequest = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrEnvelope);

		// Check timeout
		uint32_t timeout = 0;
		if (deadline != OOBase::timeval_t::MaxTime)
		{
			OOBase::timeval_t now = OOBase::gettimeofday();
			if (deadline <= now)
				return;

			timeout = (deadline - now).msec();
		}

		// Make the call
		ObjectPtr<Remoting::IMessage> ptrResult;
		ptrResult.Attach(ptrOM->Invoke(ptrRequest,timeout));

		if (!(attribs & TypeInfo::Asynchronous))
		{
			if (deadline != OOBase::timeval_t::MaxTime)
			{
				if (deadline <= OOBase::gettimeofday())
					return;
			}

			// Wrap the response...
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
			ptrMarshaller->MarshalInterface(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

			// Send it back...
			OOServer::MessageHandler::io_result::type res = send_response(seq_no,src_channel_id,src_thread_id,*ptrResponse->GetCDRStream(),deadline,attribs);
			if (res != OOServer::MessageHandler::io_result::success)
			{
				ptrMarshaller->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

				if (res == OOServer::MessageHandler::io_result::failed)
					LOG_ERROR(("Response sending failed"));
			}
		}
	}
	catch (IException* pOuter)
	{
		// Just drop the exception, and let it pass...
		pOuter->Release();
	}
}

ObjectPtr<Remoting::IObjectManager> User::Manager::create_object_manager(Omega::uint32_t src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel_i(src_channel_id,message_oid);

	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel(Omega::uint32_t src_channel_id, const guid_t& message_oid)
{
	return USER_MANAGER::instance()->create_channel_i(src_channel_id,message_oid);
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel_i(Omega::uint32_t src_channel_id, const guid_t& message_oid)
{
	assert(classify_channel(src_channel_id) > 1);

	// Lookup existing..
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(src_channel_id);
		if (i != m_mapChannels.end())
			return i->second;
	}

	// Create a new channel
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
	ptrChannel->init(this,src_channel_id,classify_channel(src_channel_id),message_oid);

	// And add to the map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	std::pair<std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator,bool> p = m_mapChannels.insert(std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::value_type(src_channel_id,ptrChannel));
	if (!p.second)
		ptrChannel = p.first->second;

	return ptrChannel;
}

OOBase::SmartPtr<OOBase::CDRStream> User::Manager::sendrecv_root(const OOBase::CDRStream& request, TypeInfo::MethodAttributes_t attribs)
{
	// The timeout needs to be related to the request timeout...
	OOBase::timeval_t deadline = OOBase::timeval_t::MaxTime;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			deadline = OOBase::timeval_t::deadline(msecs);
	}

	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	OOServer::MessageHandler::io_result::type res = send_request(m_root_channel,&request,response,deadline == OOBase::timeval_t::MaxTime ? 0 : &deadline,attribs);
	if (res != OOServer::MessageHandler::io_result::success)
	{
		if (res == OOServer::MessageHandler::io_result::timedout)
			throw Omega::ITimeoutException::Create();
		else if (res == OOServer::MessageHandler::io_result::channel_closed)
			throw Omega::Remoting::IChannelClosedException::Create();
		else
			OMEGA_THROW("Internal server exception");
	}

	return response;
}

#if defined(_WIN32)

namespace
{
	static HANDLE s_hEvent = NULL;

	static BOOL WINAPI control_c(DWORD)
	{
		// Just stop!
		if (s_hEvent)
			SetEvent(s_hEvent);

		return TRUE;
	}
}

void User::Manager::wait_for_quit()
{
	// Create the wait event
	s_hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
	if (!s_hEvent)
	{
		LOG_ERROR(("CreateEventW failed: %s",OOBase::Win32::FormatMessage().c_str()));
		return;
	}

	if (!SetConsoleCtrlHandler(control_c,TRUE))
		LOG_ERROR(("SetConsoleCtrlHandler failed: %s",OOBase::Win32::FormatMessage().c_str()));
	else
	{
		// Wait for the event to be signalled
		DWORD dwWait = WaitForSingleObject(s_hEvent,INFINITE);
		if (dwWait != WAIT_OBJECT_0)
			LOG_ERROR(("WaitForSingleObject failed: %s",OOBase::Win32::FormatMessage().c_str()));
	}

	CloseHandle(s_hEvent);
	s_hEvent = NULL;
}

void User::Manager::quit()
{
	// Just stop!
	if (s_hEvent)
		SetEvent(s_hEvent);
}

#elif defined(HAVE_EV_H)

namespace
{
	void on_sigint(struct ev_loop* pLoop, ev_signal*, int)
	{
		ev_unloop(pLoop,EVUNLOOP_ALL);
	}
}

void User::Manager::wait_for_quit()
{
	// Use libev to wait on the default loop
#if defined (EVFLAG_SIGNALFD)
	struct ev_loop* pLoop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOENV | EVFLAG_SIGNALFD);
#else
	struct ev_loop* pLoop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOENV);
#endif
	if (!pLoop)
	{
		LOG_ERROR(("ev_default_loop failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
		return;
	}

	// Add watchers for SIG_KILL, SIG_HUP, SIG_CHILD etc...
	ev_signal watcher;

	ev_signal_init(&watcher,&on_sigint,SIGINT);
	ev_signal_start(pLoop,&watcher);

	// Let ev loop...
	ev_loop(pLoop,0);
}

void User::Manager::quit()
{
	struct ev_loop* pLoop = ev_default_loop(0);
	if (!pLoop)
	{
		LOG_ERROR(("ev_default_loop failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
		return;
	}

	ev_unloop(pLoop,EVUNLOOP_ALL);
}

#else
#error Fix me!
#endif
