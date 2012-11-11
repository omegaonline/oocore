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
#include "UserIPS.h"
#include "UserChannel.h"
#include "UserRegistry.h"
#include "UserServices.h"

#include <stdlib.h>

#if defined(_WIN32)
#include <aclapi.h>
#include <sddl.h>
#endif

template class OOBase::Singleton<OOSvrBase::Proactor,User::Manager>;

namespace OTL
{
	// The following is an expansion of BEGIN_PROCESS_OBJECT_MAP
	// We don't use the macro as we override some behaviours
	namespace Module
	{
		class OOSvrUser_ProcessModuleImpl : public ProcessModule
		{
		private:
			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(User::Registry::OverlayKeyFactory)
					OBJECT_MAP_ENTRY(User::ServiceController)
					{ 0,0,0,0 }
				};
				return CreatorEntries;
			}
		};
	}

	OMEGA_PRIVATE_FN_DECL(Module::OOSvrUser_ProcessModuleImpl*,GetModule())
	{
		return OOBase::Singleton<Module::OOSvrUser_ProcessModuleImpl,User::Manager>::instance_ptr();
	}

	namespace Module
	{
		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)()
		{
			return OMEGA_PRIVATE_FN_CALL(GetModule)();
		}
	}
}

template class OOBase::Singleton<OTL::Module::OOSvrUser_ProcessModuleImpl,User::Manager>;

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
		OOBase::SmartPtr<void,OOBase::FreeDestructor<OOBase::LocalAllocator> > ptrSIDLogon;
		DWORD dwRes = OOBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

		char* pszSid;
		if (!ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
			LOG_ERROR_RETURN(("ConvertSidToStringSidA failed: %s",OOBase::system_error_text()),false);
		
		int err = name.printf("OOU%s-%ld",pszSid,GetCurrentProcessId());

		LocalFree(pszSid);
			
	#elif defined(__linux__)

		int err = name.printf(" /org/omegaonline/user-%d-%d",getuid(),getpid());

	#elif defined(HAVE_UNISTD_H)

		#if defined(P_tmpdir)
			int err = name.printf(P_tmpdir "/oou-%d-%d",getuid(),getpid());
		#else
			int err = name.printf("/tmp/oou-%d-%d",getuid(),getpid());
		#endif

	#else
	#error Fix me!
	#endif

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err)),false);		

		return true;
	}
}

string_t User::recurse_log_exception(IException* pE)
{
	string_t msg = pE->GetDescription();
	if (!msg.IsEmpty() && msg[msg.Length()-1] != '.')
		msg += ".";

	ObjectPtr<IException> ptrCause = pE->GetCause();
	if (ptrCause)
		msg += "\nCause: " + recurse_log_exception(ptrCause);

	return msg;
}

User::Manager* User::Manager::s_instance = NULL;

User::Manager::Manager() :
		m_proactor(NULL),
		m_bIsSandbox(false),
		m_mapRemoteChannelIds(1)
{
	s_instance = this;
}

User::Manager::~Manager()
{
	s_instance = NULL;
}

int User::Manager::run(const char* pszPipe)
{
	int ret = EXIT_FAILURE;
	int err = 0;
	m_proactor = OOSvrBase::Proactor::create(err);
	if (err)
		LOG_ERROR(("Failed to create proactor: %s",OOBase::system_error_text(err)));
	else
	{
		err = m_proactor_pool.run(&run_proactor,m_proactor,2);
		if (err != 0)
			LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
		else
		{
			// Start the handler
			if (start_request_threads(2))
			{
				if (connect_root(pszPipe))
				{
					OOBase::Logger::log(OOBase::Logger::Information,APPNAME " started successfully");

					// Wait for stop
					wait_for_quit();

					// Stop services (if any)
					stop_all_services();

					ret = EXIT_SUCCESS;
				}

				// Stop the MessageHandler
				stop_request_threads();
			}

			m_proactor->stop();
			m_proactor_pool.join();
		}

		OOSvrBase::Proactor::destroy(m_proactor);
	}

	if (User::is_debug() && ret != EXIT_SUCCESS)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,"Pausing to let you read the messages...");

		// Give us a chance to read the errors!
		OOBase::Thread::sleep(15000);
	}

	return ret;
}

int User::Manager::run_proactor(void* p)
{
	int err = 0;
	return static_cast<OOSvrBase::Proactor*>(p)->run(err);
}

bool User::Manager::connect_root(const char* pszPipe)
{
#if defined(_WIN32)
	// Use a named pipe
	int err = 0;
	OOBase::Timeout timeout(20,0);
	OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> local_socket(m_proactor->connect_local_socket(pszPipe,err,timeout));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err)),false);

#else

	// Use the passed fd
	int fd = atoi(pszPipe);

	int err = 0;
	OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> local_socket(m_proactor->attach_local_socket(fd,err));
	if (err != 0)
	{
		OOBase::POSIX::close(fd);
		LOG_ERROR_RETURN(("Failed to attach to root pipe: %s",OOBase::system_error_text(err)),false);
	}

#endif

	// Invent a new pipe name...
	OOBase::LocalString strNewPipe;
	if (!unique_name(strNewPipe))
		return false;
		
	// Set our pipe name
#if defined(_WIN32)
	SetEnvironmentVariableA("OMEGA_SESSION_ADDRESS",strNewPipe.c_str());
#else
	setenv("OMEGA_SESSION_ADDRESS",strNewPipe.c_str(),1);
#endif

	OOBase::CDRStream stream;

	// Send our port name
	if (!stream.write(strNewPipe.c_str()))
		LOG_ERROR_RETURN(("Failed to encode root pipe packet: %s",OOBase::system_error_text(stream.last_error())),false);

	if ((err = local_socket->send(stream.buffer())) != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOBase::system_error_text(err)),false);

	// Read the sandbox channel
	stream.reset();
	if ((err = local_socket->recv(stream.buffer(),2*sizeof(uint32_t))) != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOBase::system_error_text(err)),false);

	uint32_t sandbox_channel = 0;
	uint32_t our_channel = 0;
	if (!stream.read(sandbox_channel) || !stream.read(our_channel))
	{
		LOG_ERROR_RETURN(("Failed to decode root pipe packet: %s",OOBase::system_error_text(stream.last_error())),false);
	}

	// Set the sandbox flag
	m_bIsSandbox = (sandbox_channel == 0);

	// Init our channel id
	set_channel(our_channel,0xFF000000,0x00FFF000,0x80000000);

	// Create a new MessageConnection
	OOBase::RefPtr<OOServer::MessageConnection> ptrMC = new (std::nothrow) OOServer::MessageConnection(this,local_socket);
	if (!ptrMC)
		LOG_ERROR_RETURN(("Failed to allocate MessageConnection: %s",OOBase::system_error_text()),false);

	// Attach it to ourselves
	if (register_channel(ptrMC,m_uUpstreamChannel) == 0)
		return false;

	// Start I/O with root
	if (ptrMC->recv() != 0)
	{
		channel_closed(m_uUpstreamChannel,0);
		return false;
	}

	// Now bootstrap
	stream.reset();
	if (!stream.write(sandbox_channel) || !stream.write(strNewPipe.c_str()))
	{
		ptrMC->shutdown();
		LOG_ERROR_RETURN(("Failed to write bootstrap data: %s",OOBase::system_error_text(stream.last_error())),false);
	}

	if (!call_async_function_i("do_bootstrap",&do_bootstrap,this,&stream))
	{
		ptrMC->shutdown();
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
	input.read_string(strPipe);

	if (input.last_error() != 0)
	{
		LOG_ERROR(("Failed to read bootstrap data: %s",OOBase::system_error_text(input.last_error())));
		bQuit = true;
	}
	else
	{
		bQuit = !pThis->bootstrap(sandbox_channel) ||
			!pThis->start_acceptor(strPipe) ||
			!pThis->notify_started();
	}

	if (bQuit)
		pThis->call_async_function_i("do_quit",&do_quit,pThis,0);
}

bool User::Manager::bootstrap(uint32_t sandbox_channel)
{
	try
	{
		// Get the local ROT
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.GetObject(Activation::OID_RunningObjectTable_Instance);

		// Register our private factories...
		OTL::GetModule()->RegisterAutoObjectFactory<User::ChannelMarshalFactory>();

		// Now get the upstream IPS...
		ObjectPtr<Remoting::IObjectManager> ptrOMSb;
		if (sandbox_channel != 0)
			ptrOMSb = create_object_manager(sandbox_channel,guid_t::Null());

		ObjectPtr<ObjectImpl<InterProcessService> > ptrIPS = ObjectImpl<InterProcessService>::CreateObject();
		ptrIPS->init(ptrOMSb);

		// Register our interprocess service so we can react to activation requests
		OOCore_Omega_Initialize((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16),ptrIPS);

		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::RootKey> > ptrReg = ObjectImpl<User::Registry::RootKey>::CreateObject();
		ptrReg->init(string_t::constant("/"),0,0);

		// Registry registry
		ptrROT->RegisterObject(Omega::Registry::OID_Registry_Instance,ptrReg.QueryInterface<IObject>(),Activation::UserScope);

		// Now we have a ROT and a registry, register everything else
		GetModule()->RegisterObjectFactories();

		return true;
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
		return false;
	}
}

bool User::Manager::start_acceptor(OOBase::LocalString& strPipe)
{
#if defined(_WIN32)

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::FreeDestructor<OOBase::LocalAllocator> > ptrSIDLogon;
	DWORD dwRes = OOBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
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

	m_sa.mode = 0600;

#endif

	if (strPipe[0] == ' ')
		strPipe.replace_at(0,'\0');

	int err = 0;
	m_ptrAcceptor = m_proactor->accept_local(this,&on_accept,strPipe.c_str(),err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: %s",OOBase::system_error_text(err)),false);

	return true;
}

void User::Manager::on_accept(void* pThis, OOSvrBase::AsyncLocalSocket* pSocket, int err)
{
	OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = pSocket;

	static_cast<Manager*>(pThis)->on_accept_i(ptrSocket,err);
}

void User::Manager::on_accept_i(OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, int err)
{
	if (err != 0)
	{
		LOG_ERROR(("Accept failure: %s",OOBase::system_error_text(err)));
		return;
	}

	// Read 4 bytes - This forces credential passing
	OOBase::CDRStream stream;
	err = ptrSocket->recv(stream.buffer(),sizeof(uint32_t));
	if (err != 0)
	{
		LOG_WARNING(("Receive failure: %s",OOBase::system_error_text(err)));
		return;
	}

	// Check the versions are correct
	uint32_t version = 0;
	if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
	{
		LOG_WARNING(("Client is running a very old version: %u",version));
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
	OOBase::RefPtr<OOServer::MessageConnection> ptrMC = new (std::nothrow) OOServer::MessageConnection(this,ptrSocket);
	if (!ptrMC)
		LOG_ERROR(("Failed to allocate MessageConnection: %s",OOBase::system_error_text()));
	else
	{
		// Attach it to ourselves
		uint32_t channel_id = register_channel(ptrMC,0);
		if (channel_id != 0)
		{
			// Send the channel id...
			if (!stream.write(channel_id))
			{
				channel_closed(channel_id,0);
				LOG_ERROR(("Failed to encode channel_id: %s",OOBase::system_error_text(stream.last_error())));
			}
			else if ((err = ptrMC->send(stream.buffer(),NULL)) != 0)
				channel_closed(channel_id,0);
			else
			{
				// Start I/O
				if ((err = ptrMC->recv()) != 0)
					ptrMC->shutdown();
			}
		}
	}
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

				m_mapChannels.remove(k);
			}
		}

		// Give the remote layer a chance to close channels
		if (!dead_channels.empty())
			local_channel_closed(dead_channels);

	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
	}

	// If the root closes, we should end!
	if (channel_id == m_uUpstreamChannel)
	{
		LOG_DEBUG(("Upstream channel has closed"));
		do_quit_i();
	}
}

void User::Manager::do_quit(void* pParams, OOBase::CDRStream&)
{
	static_cast<Manager*>(pParams)->do_quit_i();
}

void User::Manager::do_quit_i()
{
	OOBase::Logger::log(OOBase::Logger::Information,APPNAME " closing");

	// Stop accepting new clients
	m_ptrAcceptor = NULL;

	// Close all the sinks
	close_all_remotes();

	try
	{
		// Unregister our object factories
		GetModule()->UnregisterObjectFactories();
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
	}

	// Close the OOCore
	Uninitialize();

	// Close all channels
	shutdown_channels();

	// And now call quit()
	quit();
}

void User::Manager::process_request(OOBase::CDRStream& request, uint32_t src_channel_id, uint16_t src_thread_id, const OOBase::Timeout& timeout, uint32_t attribs)
{
	if (src_channel_id == m_uUpstreamChannel)
		process_root_request(request,src_thread_id,timeout,attribs);
	else
		process_user_request(request,src_channel_id,src_thread_id,timeout,attribs);
}

void User::Manager::process_root_request(OOBase::CDRStream& request, uint16_t src_thread_id, const OOBase::Timeout& timeout, uint32_t attribs)
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
	case OOServer::Service_Start:
		start_service(request,attribs & OOServer::Message_t::asynchronous ? NULL : &response);
		break;

	case OOServer::Service_Stop:
		stop_service(request,response);
		break;

	case OOServer::Service_StopAll:
		stop_all_services(response);
		break;

	case OOServer::Service_IsRunning:
		service_is_running(request,response);
		break;

	case OOServer::Service_ListRunning:
		list_services(response);
		break;

	default:
		response.write(static_cast<OOServer::RootErrCode_t>(OOServer::Errored));
		LOG_ERROR(("Bad request op_code: %u",op_code));
		break;
	}

	if (!response.last_error() && !(attribs & OOServer::Message_t::asynchronous))
	{
		OOServer::MessageHandler::io_result::type res = send_response(m_uUpstreamChannel,src_thread_id,response,attribs);
		if (res == OOServer::MessageHandler::io_result::failed)
			LOG_ERROR(("Root response sending failed"));
	}
}

void User::Manager::process_user_request(OOBase::CDRStream& request, uint32_t src_channel_id, uint16_t src_thread_id, const OOBase::Timeout& timeout, uint32_t attribs)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id,guid_t::Null());
		if (!ptrOM)
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to find or create object manager for channel"));

		// QI for IMarshaller
		ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
		if (!ptrMarshaller)
			throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope;
		ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateObject();
		ptrEnvelope->init(request);

		// Unpack the payload
		ObjectPtr<Remoting::IMessage> ptrRequest;
		ptrRequest.Unmarshal(ptrMarshaller,string_t::constant("payload"),ptrEnvelope);

		// Check timeout
		if (timeout.has_expired())
			throw ITimeoutException::Create();

		// Make the call
		ObjectPtr<Remoting::IMessage> ptrResult = ptrOM->Invoke(ptrRequest,timeout.is_infinite() ? 0 : timeout.millisecs());

		if (!(attribs & OOServer::Message_t::asynchronous))
		{
			// Wrap the response...
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateObject();
			ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

			// Send it back...
			OOServer::MessageHandler::io_result::type res = send_response(src_channel_id,src_thread_id,*ptrResponse->GetCDRStream(),attribs);
			if (res != OOServer::MessageHandler::io_result::success)
				ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
		}
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		if (!(attribs & OOServer::Message_t::asynchronous))
		{
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateObject();

			OOCore_RespondException(ptrResponse,pE);

			send_response(src_channel_id,src_thread_id,*ptrResponse->GetCDRStream(),attribs);
		}
	}
}

Remoting::IObjectManager* User::Manager::create_object_manager(uint32_t src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel_i(src_channel_id,message_oid);

	return ptrChannel->GetObjectManager();
}

ObjectImpl<User::Channel>* User::Manager::create_channel(uint32_t src_channel_id, const guid_t& message_oid)
{
	return s_instance->create_channel_i(src_channel_id,message_oid);
}

ObjectImpl<User::Channel>* User::Manager::create_channel_i(uint32_t src_channel_id, const guid_t& message_oid)
{
	// Lookup existing..
	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		if (m_mapChannels.find(src_channel_id,ptrChannel))
			return ptrChannel.Detach();
	}

	// Create a new channel
	ptrChannel = ObjectImpl<Channel>::CreateObject();
	ptrChannel->init(src_channel_id,classify_channel(src_channel_id),message_oid);

	// And add to the map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	int err = m_mapChannels.insert(src_channel_id,ptrChannel);
	if (err == EEXIST)
		m_mapChannels.find(src_channel_id,ptrChannel);
	else if (err != 0)
		OMEGA_THROW(err);

	return ptrChannel.Detach();
}

void User::Manager::sendrecv_root(const OOBase::CDRStream& request, OOBase::CDRStream* response, TypeInfo::MethodAttributes_t attribs)
{
	OOServer::MessageHandler::io_result::type res = send_request(m_uUpstreamChannel,&request,response,attribs);
	if (res != OOServer::MessageHandler::io_result::success)
	{
		if (res == OOServer::MessageHandler::io_result::timedout)
			throw ITimeoutException::Create();
		else if (res == OOServer::MessageHandler::io_result::channel_closed)
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to send root request"));
		else
			OMEGA_THROW("Internal server exception");
	}
}

void User::Manager::get_root_config_arg(const char* key, Omega::string_t& strValue)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::User_GetConfigArg));
	request.write(key);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	sendrecv_root(request,&response,TypeInfo::Synchronous);

	OOServer::RootErrCode_t err;
	OOBase::LocalString strVal;
	if (!response.read(err) || (!err && !response.read_string(strVal)))
		OMEGA_THROW(response.last_error());

	if (err)
		throw IInternalException::Create(string_t::constant("Failed to get server configuration parameter.  Check server log for details"),"get_root_config_arg",0,NULL,NULL);

	strValue = strVal.c_str();
}

bool User::Manager::notify_started()
{
	if (m_bIsSandbox)
	{
		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::User_NotifyStarted));

		if (request.last_error() != 0)
			LOG_ERROR_RETURN(("Failed to write start notification arguments: %s",OOBase::system_error_text(request.last_error())),false);

		if (send_request(m_uUpstreamChannel,&request,NULL,TypeInfo::Asynchronous) != OOServer::MessageHandler::io_result::success)
			LOG_ERROR_RETURN(("Failed to send start notification: %s",OOBase::system_error_text()),false);
	}

	return true;
}
