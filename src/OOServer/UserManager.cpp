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

User::Manager* User::Manager::s_instance = 0;

User::Manager::Manager() :
		m_nIPSCookie(0),
		m_bIsSandbox(false),
		m_nNextRemoteChannel(0),
		m_nNextService(0)
{
	s_instance = this;
}

User::Manager::~Manager()
{
	s_instance = 0;
}

void User::Manager::run()
{
	// Wait for stop
	wait_for_quit();

	{
		void* TODO; // All this needs to be in an async_function()

		// Stop accepting new clients
		m_acceptor.stop();

		// Close all the sinks
		close_all_remotes();

		// Stop services
		stop_services();

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

	// Close the user pipes
	close_channels();
}

bool User::Manager::fork_slave(const std::string& strPipe)
{
	// Connect to the root

#if defined(_WIN32)
	// Use a named pipe
	int err = 0;
	OOBase::timeval_t wait(20);
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance()->connect_local_socket(strPipe,&err,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err).c_str()),false);

#elif defined(HAVE_UNISTD_H)
	// Use the passed fd
	int fd = atoi(strPipe.c_str());

	// Add FD_CLOEXEC to fd
	int err = OOBase::POSIX::set_close_on_exec(fd,true);
	if (err != 0)
	{
		::close(fd);
		LOG_ERROR_RETURN(("set_close_on_exec failed: %s",OOBase::system_error_text(err).c_str()),false);
	}

	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance()->attach_local_socket(fd,&err);
	if (err != 0)
	{
		::close(fd);
		LOG_ERROR_RETURN(("Failed to attach to root pipe: %s",OOBase::system_error_text(err).c_str()),false);
	}

#endif

	// Invent a new pipe name...
	std::string strNewPipe = Acceptor::unique_name();
	if (strNewPipe.empty())
		return false;

	return handshake_root(local_socket,strNewPipe);
}

bool User::Manager::session_launch(const std::string& strPipe)
{
#if defined(_WIN32)
	OMEGA_UNUSED_ARG(strPipe);

	LOG_ERROR_RETURN(("Somehow got into session_launch!"),false);
#else

	// Use the passed fd
	int fd = atoi(strPipe.c_str());

	// Invent a new pipe name...
	std::string strNewPipe = Acceptor::unique_name();
	if (strNewPipe.empty())
		return false;

	pid_t pid = getpid();
	if (write(fd,&pid,sizeof(pid)) != sizeof(pid))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text(errno).c_str()),false);

	// Then send back our port name
	size_t uLen = strNewPipe.length()+1;
	if (write(fd,&uLen,sizeof(uLen)) != sizeof(uLen))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text(errno).c_str()),false);

	if (write(fd,strNewPipe.c_str(),uLen) != static_cast<ssize_t>(uLen))
		LOG_ERROR_RETURN(("Failed to write session data: %s",OOBase::system_error_text(errno).c_str()),false);

	// Make sure we set our OMEGA_SESSION_ADDRESS
	if (setenv("OMEGA_SESSION_ADDRESS",strNewPipe.c_str(),1) != 0)
		LOG_ERROR_RETURN(("Failed to set OMEGA_SESSION_ADDRESS: %s",OOBase::system_error_text(errno).c_str()),false);

	// Done with the port...
	close(fd);

	// Now connect to ooserverd
	int err = 0;
	OOBase::timeval_t wait(20);
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> local_socket = Proactor::instance()->connect_local_socket("/tmp/omegaonline",&err,&wait);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err).c_str()),false);

	// Send version information
	uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;

	OOBase::CDRStream stream;
	if (!stream.write(version))
		LOG_ERROR_RETURN(("Failed to write root data: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);

	err = local_socket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOBase::system_error_text(err).c_str()),false);

	// Connect up
	return handshake_root(local_socket,strNewPipe);

#endif
}

bool User::Manager::handshake_root(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& local_socket, const std::string& strPipe)
{
	OOBase::CDRStream stream;

	// Read the sandbox channel
	int err = local_socket->recv(stream.buffer(),sizeof(Omega::uint32_t));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOBase::system_error_text(err).c_str()),false);

	Omega::uint32_t sandbox_channel = 0;
	if (!stream.read(sandbox_channel))
		LOG_ERROR_RETURN(("Failed to decode root pipe packet: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);

	// Set the sandbox flag
	m_bIsSandbox = (sandbox_channel == 0);

	// Then send back our port name
	stream.reset();
	if (!stream.write(strPipe))
		LOG_ERROR_RETURN(("Failed to encode root pipe packet: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);

	err = local_socket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to write to root pipe: %s",OOBase::system_error_text(err).c_str()),false);

	// Read our channel id
	stream.reset();
	err = local_socket->recv(stream.buffer(),sizeof(Omega::uint32_t));
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to read from root pipe: %s",OOBase::system_error_text(err).c_str()),false);

	Omega::uint32_t our_channel = 0;
	if (!stream.read(our_channel))
		LOG_ERROR_RETURN(("Failed to decode root pipe packet: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);

	// Init our channel id
	set_channel(our_channel,0xFF000000,0x00FFF000,m_root_channel);

	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
	OOBASE_NEW(ptrMC,OOServer::MessageConnection(this,local_socket));
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Attach it to ourselves
	if (register_channel(ptrMC,m_root_channel) == 0)
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
	stream.reset();
	if (!stream.write(sandbox_channel) || !stream.write(strPipe))
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("Failed to write bootstrap data: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);
	}

	if (!call_async_function_i(&do_bootstrap,this,&stream))
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
	Omega::uint32_t sandbox_channel = 0;
	input.read(sandbox_channel);
	std::string strPipe;
	input.read(strPipe);
	if (input.last_error() != 0)
	{
		LOG_ERROR(("Failed to read bootstrap data: %s",OOBase::system_error_text(input.last_error()).c_str()));
		bQuit = true;
	}

	bQuit = bQuit ||
			!pThis->bootstrap(sandbox_channel) ||
			!pThis->m_acceptor.start(pThis,strPipe) ||
			!pThis->start_services();

	if (bQuit)
		pThis->quit();
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

bool User::Manager::on_accept(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket)
{
	// Create a new MessageConnection
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
	OOBASE_NEW(ptrMC,OOServer::MessageConnection(this,ptrSocket));
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Attach it to ourselves
	uint32_t channel_id = register_channel(ptrMC,0);
	if (channel_id == 0)
	{
		ptrMC->close();
		return false;
	}

	// Send the channel id...
	OOBase::CDRStream stream;
	if (!stream.write(channel_id))
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("Failed to encode channel_id: %s",OOBase::system_error_text(stream.last_error()).c_str()),false);
	}

	if (!ptrMC->send(stream.buffer()))
		return false;

	// Start I/O
	return ptrMC->read();
}

void User::Manager::on_channel_closed(Omega::uint32_t channel)
{
	// Close the corresponding Object Manager
	/*try
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
				// Close all compartments if 0 cmpt dies
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
	{}*/

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
		LOG_ERROR(("Bad request: %s",OOBase::system_error_text(request.last_error()).c_str()));
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
		response.write(Omega::int32_t(EINVAL));
		LOG_ERROR(("Bad request op_code: %u",op_code));
		break;
	}

	if (response.last_error() == 0 && !(attribs & TypeInfo::Asynchronous))
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

ObjectPtr<Remoting::IObjectManager> User::Manager::create_object_manager(Omega::uint32_t src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel_i(src_channel_id,message_oid);

	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel(Omega::uint32_t src_channel_id, const guid_t& message_oid)
{
	return s_instance->create_channel_i(src_channel_id,message_oid);
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
