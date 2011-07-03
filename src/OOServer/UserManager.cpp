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

#include <stdlib.h>

#if defined(_WIN32)
#include <aclapi.h>
#include <sddl.h>
#endif

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

namespace
{
	bool unique_name(OOBase::LocalString& name)
	{
		// Create a new unique pipe

	#if defined(_WIN32)
		// Get the current user's Logon SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

		char* pszSid;
		if (!ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
			LOG_ERROR_RETURN(("ConvertSidToStringSidA failed: %s",OOBase::system_error_text()),false);
		
		int err = name.printf("OOU%s-%ld",pszSid,GetCurrentProcessId());

		LocalFree(pszSid);
			
	#elif defined(HAVE_UNISTD_H)

		int err = name.printf("/tmp/oo-%d-%d",getuid(),getpid());

	#else
	#error Fix me!
	#endif

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err)),false);		

		return true;
	}
}

// UserManager

User::Manager* User::Manager::s_instance = NULL;

User::Manager::Manager() :
		m_nIPSCookie(0),
		m_bIsSandbox(false),
		m_mapRemoteChannelIds(1),
		m_mapServices(1)
{
	s_instance = this;
}

User::Manager::~Manager()
{
	s_instance = NULL;
}

void User::Manager::run()
{
	// Wait for stop
	wait_for_quit();

	// Close the user pipes
	close_channels();
}

bool User::Manager::fork_slave(const OOBase::String& strPipe)
{
	// Connect to the root

#if defined(_WIN32)
	// Use a named pipe
	int err = 0;
	OOBase::timeval_t wait(20);
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance().connect_local_socket(strPipe.c_str(),err,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err)),false);

#elif defined(HAVE_UNISTD_H)
	// Use the passed fd
	int fd = atoi(strPipe);

	// Add FD_CLOEXEC to fd
	int err = OOBase::POSIX::set_close_on_exec(fd,true);
	if (err != 0)
	{
		::close(fd);
		LOG_ERROR_RETURN(("set_close_on_exec failed: %s",OOBase::system_error_text(err)),false);
	}

	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance().attach_local_socket(fd,&err);
	if (err != 0)
	{
		::close(fd);
		LOG_ERROR_RETURN(("Failed to attach to root pipe: %s",OOBase::system_error_text(err)),false);
	}

#endif

	// Invent a new pipe name...
	OOBase::LocalString strNewPipe;
	if (!unique_name(strNewPipe))
		return false;

	return handshake_root(local_socket,strNewPipe);
}

bool User::Manager::session_launch(const OOBase::String& strPipe)
{
#if defined(_WIN32)
	OMEGA_UNUSED_ARG(strPipe);

	LOG_ERROR_RETURN(("Somehow got into session_launch!"),false);
#else

	// Use the passed fd
	int fd = atoi(strPipe);

	// Invent a new pipe name...
	OOBase::LocalString strNewPipe;
	if (!unique_name(strNewPipe))
		return false;

	pid_t pid = getpid();
	if (write(fd,&pid,sizeof(pid)) != sizeof(pid))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text()),false);

	// Then send back our port name
	size_t uLen = strNewPipe.length()+1;
	if (write(fd,&uLen,sizeof(uLen)) != sizeof(uLen))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text()),false);

	if (write(fd,strNewPipe.c_str(),uLen) != static_cast<ssize_t>(uLen))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text()),false);

	// Make sure we set our OMEGA_SESSION_ADDRESS
	if (setenv("OMEGA_SESSION_ADDRESS",strNewPipe.c_str(),1) != 0)
		LOG_ERROR_RETURN(("Failed to set OMEGA_SESSION_ADDRESS: %s",OOBase::system_error_text()),false);

	// Done with the port...
	close(fd);

	// Now connect to ooserverd
	int err = 0;
	OOBase::timeval_t wait(20);
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance().connect_local_socket("/tmp/omegaonline",&err,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err)),false);

	// Send version information
	uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;

	OOBase::CDRStream stream;
	if (!stream.write(version))
		LOG_ERROR_RETURN(("Failed to write root data: %s",OOBase::system_error_text(stream.last_error())),false);

	err = local_socket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOBase::system_error_text(err)),false);

	// Connect up
	return handshake_root(local_socket,strNewPipe);

#endif
}

bool User::Manager::start_proactor_threads()
{
	int err = m_proactor_pool.run(run_proactor,NULL,2);
	if (err != 0)
	{
		m_proactor_pool.join();
		LOG_ERROR_RETURN(("Thread pool create failed: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

int User::Manager::run_proactor(void*)
{
	int err = 0;
	return Proactor::instance().run(err);
}

bool User::Manager::handshake_root(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket, const OOBase::LocalString& strPipe)
{
	OOBase::CDRStream stream;

	// Read the sandbox channel
	int err = local_socket->recv(stream.buffer(),sizeof(uint32_t));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOBase::system_error_text(err)),false);

	uint32_t sandbox_channel = 0;
	if (!stream.read(sandbox_channel))
		LOG_ERROR_RETURN(("Failed to decode root pipe packet: %s",OOBase::system_error_text(stream.last_error())),false);

	// Set the sandbox flag
	m_bIsSandbox = (sandbox_channel == 0);

	// Then send back our port name
	if (!stream.write(strPipe.c_str()))
		LOG_ERROR_RETURN(("Failed to encode root pipe packet: %s",OOBase::system_error_text(stream.last_error())),false);

	err = local_socket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOBase::system_error_text(err)),false);

	// Read our channel id
	err = local_socket->recv(stream.buffer(),sizeof(uint32_t));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOBase::system_error_text(err)),false);

	uint32_t our_channel = 0;
	if (!stream.read(our_channel))
		LOG_ERROR_RETURN(("Failed to decode root pipe packet: %s",OOBase::system_error_text(stream.last_error())),false);

	// Init our channel id
	set_channel(our_channel,0xFF000000,0x00FFF000,0x80000000);

	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC = new (std::nothrow) OOServer::MessageConnection(this,local_socket);
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Attach it to ourselves
	if (register_channel(ptrMC,m_uUpstreamChannel) == 0)
	{
		ptrMC->close();
		return false;
	}

	// Start I/O with root
	if (!ptrMC->read())
	{
		ptrMC->close();
		return false;
	}

	// Now bootstrap
	if (!stream.write(sandbox_channel) || !stream.write(strPipe.c_str()))
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("Failed to write bootstrap data: %s",OOBase::system_error_text(stream.last_error())),false);
	}

	if (!call_async_function_i("do_bootstrap",&do_bootstrap,this,&stream))
	{
		ptrMC->close();
		return false;
	}

	return true;
}

void User::Manager::do_bootstrap(void* pParams, OOBase::CDRStream& input)
{
	Manager* pThis = static_cast<Manager*>(pParams);

	bool bQuit = false;

	uint32_t sandbox_channel = 0;
	input.read(sandbox_channel);

	OOBase::LocalString strPipe;
	input.read(strPipe);

	if (input.last_error() != 0)
	{
		LOG_ERROR(("Failed to read bootstrap data: %s",OOBase::system_error_text(input.last_error())));
		bQuit = true;
	}
	else
	{
		bQuit = !pThis->bootstrap(sandbox_channel) ||
			!pThis->start_acceptor(strPipe) ||
			!pThis->start_services();
	}

	if (bQuit)
		pThis->call_async_function_i("do_quit",&do_quit,pThis,0);
}

bool User::Manager::bootstrap(uint32_t sandbox_channel)
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

		// Register our interprocess service so we can react to activation requests
		m_nIPSCookie = OOCore_RegisterIPS(ptrIPS);

		// Now we have a ROT, register everything else
		GetModule()->RegisterObjectFactories();

		return true;
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
		pE->Release();

		return false;
	}
}

bool User::Manager::start_acceptor(const OOBase::LocalString& strPipe)
{
#if defined(_WIN32)

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

	// Set full control for the Logon SID only
	EXPLICIT_ACCESSW ea = {0};
	ea.grfAccessPermissions = FILE_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = (LPWSTR)ptrSIDLogon;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(1,&ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwErr)),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(m_sa);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

#elif defined(HAVE_UNISTD_H)

	m_sa.mode = 0700;

#else
#error set security on pipe_name
#endif

	int err = 0;
	m_ptrAcceptor = Proactor::instance().accept_local(this,&Manager::on_accept,strPipe.c_str(),err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: %s",OOBase::system_error_text(err)),false);

	err = m_ptrAcceptor->listen();
	if (err != 0)
	{
		m_ptrAcceptor = NULL;
		LOG_ERROR_RETURN(("listen failed: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void User::Manager::on_accept(OOSvrBase::AsyncLocalSocket* pSocket, int err)
{
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = pSocket;

	if (err != 0)
	{
		LOG_ERROR(("Accept failure: %s",OOBase::system_error_text(err)));
		return;
	}

	// Read 4 bytes - This forces credential passing
	OOBase::CDRStream stream;
	err = ptrSocket->recv(stream.buffer(),sizeof(Omega::uint32_t));
	if (err != 0)
	{
		LOG_WARNING(("Receive failure: %s",OOBase::system_error_text(err)));
		return;
	}

	// Check the versions are correct
	Omega::uint32_t version = 0;
	if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
	{
		LOG_WARNING(("Version received too early: %u",version));
		return;
	}

#if defined(HAVE_UNISTD_H)

	// Check to see if the connection came from a process with our uid
	OOSvrBase::AsyncLocalSocket::uid_t uid;
	err = ptrSocket->get_uid(uid);
	if (err != 0)
	{
		LOG_WARNING(("get_uid failure: %s",OOBase::system_error_text(err)));
		return;
	}

	if (getuid() != uid)
	{
		LOG_WARNING(("Attempt to connect by invalid user"));
		return;
	}

#endif

	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC = new (std::nothrow) OOServer::MessageConnection(this,ptrSocket);
	if (!ptrMC)
	{
		LOG_ERROR(("Out of memory"));
		return;
	}

	// Attach it to ourselves
	uint32_t channel_id = register_channel(ptrMC,0);
	if (channel_id == 0)
	{
		ptrMC->close();
		return;
	}

	// Send the channel id...
	if (!stream.write(channel_id))
	{
		ptrMC->close();
		LOG_ERROR(("Failed to encode channel_id: %s",OOBase::system_error_text(stream.last_error())));
		return;
	}

	if (!ptrMC->send(stream.buffer()))
		return;

	// Start I/O
	ptrMC->read();
}

void User::Manager::on_channel_closed(uint32_t channel)
{
	OOBase::CDRStream stream;
	if (!stream.write(channel))
		LOG_ERROR(("Failed to write channel_close data: %s",OOBase::system_error_text(stream.last_error())));
	else
		call_async_function_i("do_channel_closed",&do_channel_closed,this,&stream);
}

void User::Manager::do_channel_closed(void* pParams, OOBase::CDRStream& stream)
{
	uint32_t channel_id = 0;
	if (!stream.read(channel_id))
		LOG_ERROR(("Failed to read channel_close data: %s",OOBase::system_error_text(stream.last_error())));
	else
		static_cast<Manager*>(pParams)->do_channel_closed_i(channel_id);
}

void User::Manager::do_channel_closed_i(uint32_t channel_id)
{
	// Close the corresponding Object Manager
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		OOBase::Stack<uint32_t,OOBase::LocalAllocator> dead_channels;
		for (size_t i=m_mapChannels.begin(); i!=m_mapChannels.npos;i=m_mapChannels.next(i))
		{
			uint32_t k = *m_mapChannels.key_at(i);
			bool bErase = false;
			if (k == channel_id)
			{
				// Close if its an exact match
				bErase = true;
			}
			else if (!(channel_id & 0xFFF) && (k & 0xFFFFF000) == channel_id)
			{
				// Close all compartments if 0 cmpt dies
				bErase = true;
			}
			else if (channel_id == m_uUpstreamChannel && classify_channel(k) > 2)
			{
				// If the root channel closes, close all upstream OMs
				bErase = true;
			}

			if (bErase)
			{
				(*m_mapChannels.at(i))->disconnect();

				dead_channels.push(k);

				m_mapChannels.erase(k);
			}
		}

		// Give the remote layer a chance to close channels
		if (!dead_channels.empty())
			local_channel_closed(dead_channels);

	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
		pE->Release();
	}

	// If the root closes, we should end!
	if (channel_id == m_uUpstreamChannel)
		do_quit_i();
}

void User::Manager::do_quit(void* pParams, OOBase::CDRStream&)
{
	static_cast<Manager*>(pParams)->do_quit_i();
}

void User::Manager::do_quit_i()
{
	try
	{
		// Stop accepting new clients
		if (m_ptrAcceptor)
			m_ptrAcceptor->stop();

		// Close all the sinks
		close_all_remotes();

		// Stop services
		stop_services();

		try
		{
			// Unregister our object factories
			GetModule()->UnregisterObjectFactories();

			// Unregister InterProcessService
			OOCore_RevokeIPS(m_nIPSCookie);
			m_nIPSCookie = 0;
		}
		catch (IException* pE)
		{
			LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
			pE->Release();
		}

		// Close the OOCore
		Uninitialize();
	}
	catch (...)
	{
		LOG_ERROR(("Unrecognised exception thrown"));
	}

	// And now call quit()
	quit();
}

void User::Manager::process_request(OOBase::CDRStream& request, uint32_t seq_no, uint32_t src_channel_id, uint16_t src_thread_id, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	if (src_channel_id == m_uUpstreamChannel)
		process_root_request(request,seq_no,src_thread_id,deadline,attribs);
	else
		process_user_request(request,seq_no,src_channel_id,src_thread_id,deadline,attribs);
}

void User::Manager::process_root_request(OOBase::CDRStream& request, uint32_t seq_no, uint16_t src_thread_id, const OOBase::timeval_t& deadline, uint32_t attribs)
{
	OOServer::RootOpCode_t op_code;
	if (!request.read(op_code))
	{
		LOG_ERROR(("Bad request: %s",OOBase::system_error_text(request.last_error())));
		return;
	}

	OOBase::CDRStream response;
	switch (op_code)
	{
	case OOServer::OnSocketAccept:
		on_socket_accept(request,response);
		break;

	case OOServer::OnSocketRecv:
		on_socket_recv(request);
		break;

	case OOServer::OnSocketSent:
		on_socket_sent(request);
		break;

	case OOServer::OnSocketClose:
		on_socket_close(request);
		break;

	default:
		response.write(int32_t(EINVAL));
		LOG_ERROR(("Bad request op_code: %u",op_code));
		break;
	}

	if (response.last_error() == 0 && !(attribs & TypeInfo::Asynchronous))
	{
		OOServer::MessageHandler::io_result::type res = send_response(seq_no,m_uUpstreamChannel,src_thread_id,response,deadline,attribs);
		if (res == OOServer::MessageHandler::io_result::failed)
			LOG_ERROR(("Root response sending failed"));
	}
}

void User::Manager::process_user_request(OOBase::CDRStream& request, uint32_t seq_no, uint32_t src_channel_id, uint16_t src_thread_id, const OOBase::timeval_t& deadline, uint32_t attribs)
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
			OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
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
				if (deadline <= OOBase::timeval_t::gettimeofday())
					return;
			}

			// Wrap the response...
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
			ptrMarshaller->MarshalInterface(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

			// Send it back...
			OOServer::MessageHandler::io_result::type res = send_response(seq_no,src_channel_id,src_thread_id,*ptrResponse->GetCDRStream(),deadline,attribs);
			if (res != OOServer::MessageHandler::io_result::success)
				ptrMarshaller->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
		}
	}
	catch (IException* pOuter)
	{
		// Just drop the exception, and let it pass...
		pOuter->Release();
	}
}

ObjectPtr<Remoting::IObjectManager> User::Manager::create_object_manager(uint32_t src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel_i(src_channel_id,message_oid);

	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel(uint32_t src_channel_id, const guid_t& message_oid)
{
	return s_instance->create_channel_i(src_channel_id,message_oid);
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel_i(uint32_t src_channel_id, const guid_t& message_oid)
{
	assert(classify_channel(src_channel_id) > 1);

	// Lookup existing..
	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		if (m_mapChannels.find(src_channel_id,ptrChannel))
			return ptrChannel;
	}

	// Create a new channel
	ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
	ptrChannel->init(this,src_channel_id,classify_channel(src_channel_id),message_oid);

	// And add to the map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	int err = m_mapChannels.insert(src_channel_id,ptrChannel);
	if (err == EEXIST)
		m_mapChannels.find(src_channel_id,ptrChannel);
	else if (err != 0)
		OMEGA_THROW(err);

	return ptrChannel;
}

OOBase::SmartPtr<OOBase::CDRStream> User::Manager::sendrecv_root(OOBase::CDRStream& request, TypeInfo::MethodAttributes_t attribs)
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
	OOServer::MessageHandler::io_result::type res = send_request(m_uUpstreamChannel,&request,response,deadline == OOBase::timeval_t::MaxTime ? 0 : &deadline,attribs);
	if (res != OOServer::MessageHandler::io_result::success)
	{
		if (res == OOServer::MessageHandler::io_result::timedout)
			throw ITimeoutException::Create();
		else if (res == OOServer::MessageHandler::io_result::channel_closed)
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to send root request"));
		else
			OMEGA_THROW("Internal server exception");
	}

	return response;
}
