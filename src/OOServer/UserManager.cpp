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
#include "UserRootConn.h"
#include "UserIPS.h"
#include "UserRegistry.h"
#include "UserServices.h"

#include <stdlib.h>

#if defined(_WIN32)
#include <aclapi.h>
#include <sddl.h>
#endif

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
		m_bIsSandbox(false)
{
	s_instance = this;
}

User::Manager::~Manager()
{
	s_instance = NULL;
}

int User::Manager::run(const OOBase::LocalString& strPipe)
{
	int ret = EXIT_FAILURE;
	int err = 0;
	m_proactor = OOBase::Proactor::create(err);
	if (err)
		LOG_ERROR(("Failed to create proactor: %s",OOBase::system_error_text(err)));
	else
	{
		err = m_thread_pool.run(&run_proactor,m_proactor,1);
		if (err != 0)
			LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
		else
		{
			// Start the handler
			if (connect_root(strPipe))
			{
				OOBase::Logger::log(OOBase::Logger::Information,APPNAME " started successfully");

				// Wait for stop
				wait_for_quit();

				// Stop services (if any)
				stop_all_services();

				ret = EXIT_SUCCESS;
			}

			m_proactor->stop();
			m_thread_pool.join();
		}

		OOBase::Proactor::destroy(m_proactor);
	}

	if (User::is_debug() /*&& ret != EXIT_SUCCESS*/)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,"Pausing to let you read the messages...");

		// Give us a chance to read the errors!
		OOBase::Thread::sleep(15000);
	}

	return ret;
}

int User::Manager::run_proactor(void* param)
{
	int err = 0;
	return static_cast<OOBase::Proactor*>(param)->run(err);
}

int User::Manager::start(OOBase::RefPtr<OOBase::AsyncSocket>& ptrUserSocket, OOBase::RefPtr<OOBase::AsyncSocket>& ptrRootSocket, OOBase::CDRStream& stream)
{
	// Set the sandbox flag
	if (!ptrRootSocket)
		m_bIsSandbox = true;

	try
	{
		// Start the OOCore, in hosted mode
		IException* pE = OOCore_Omega_Initialize((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16),true);
		if (pE)
			throw pE;

		// Get the local ROT
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.GetObject(Activation::OID_RunningObjectTable_Instance);

		// Now get the upstream IPS...
		ObjectPtr<Remoting::IObjectManager> ptrOMSb;
		//if (sandbox_channel != 0)
		//	ptrOMSb = create_object_manager(sandbox_channel,guid_t::Null());

		ObjectPtr<ObjectImpl<InterProcessService> > ptrIPS = ObjectImpl<InterProcessService>::CreateObject();
		ptrIPS->init(ptrOMSb);

		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::RootKey> > ptrReg = ObjectImpl<User::Registry::RootKey>::CreateObject();
		ptrReg->init(string_t::constant("/"),0,0);

		// Registry registry
		ptrROT->RegisterObject(Omega::Registry::OID_Registry_Instance,ptrReg.QueryInterface<IObject>(),Activation::UserScope);

		// Now we have a ROT and a registry, register everything else
		GetModule()->RegisterObjectFactories();
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR_RETURN(("IException thrown: %s",recurse_log_exception(ptrE).c_str()),-1);
	}

	return m_thread_pool.run(&handle_events,this,1);
}

int User::Manager::handle_events(void* param)
{
	Manager* pThis = static_cast<Manager*>(param);

	// Initially wait 15 seconds for 1st message
	uint32_t msecs = 15000;
	try
	{
		while (Omega::HandleRequest(msecs) || GetModuleBase()->HaveLocks() || !Omega::CanUnload())
		{
			// Once we have the first message, we can then wait a very short time
			msecs = 500;
		}

		return 0;
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR_RETURN(("IException thrown: %s",recurse_log_exception(ptrE).c_str()),-1);
	}
	catch (...)
	{
		LOG_ERROR_RETURN(("Unrecognised exception thrown"),-1);
	}
}

/*void User::Manager::on_accept_i(OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket, int err)
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
	uid_t uid;
	//err = ptrSocket->get_uid(uid);
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
}*/

void User::Manager::do_quit()
{
	OOBase::Logger::log(OOBase::Logger::Information,APPNAME " closing");

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

	// And now call quit()
	quit();
}


/*void User::Manager::process_user_request(OOBase::CDRStream& request, uint32_t src_channel_id, uint16_t src_thread_id, uint32_t attribs)
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

		// Make the call
		ObjectPtr<Remoting::IMessage> ptrResult = ptrOM->Invoke(ptrRequest);

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
}*/

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
	OOBase::StackAllocator<256> allocator;
	OOBase::LocalString strVal(allocator);
	if (!response.read(err) || (!err && !response.read_string(strVal)))
		OMEGA_THROW(response.last_error());

	if (err)
		throw IInternalException::Create(string_t::constant("Failed to get server configuration parameter.  Check server log for details"),"get_root_config_arg");

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

		//if (send_request(m_uUpstreamChannel,&request,NULL,TypeInfo::Asynchronous) != OOServer::MessageHandler::io_result::success)
			LOG_ERROR_RETURN(("Failed to send start notification: %s",OOBase::system_error_text()),false);
	}

	return true;
}

void User::Manager::sendrecv_root(const OOBase::CDRStream& request, OOBase::CDRStream* response, Omega::TypeInfo::MethodAttributes_t attribs)
{

}
