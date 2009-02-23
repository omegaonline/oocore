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

#include "./OOServer_User.h"
#include "./UserManager.h"
#include "./InterProcessService.h"
#include "./Channel.h"
#include "./NetTcp.h"
#include "./NetHttp.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

namespace OTL
{
	// The following is an expansion of BEGIN_PROCESS_OBJECT_MAP
	// We don't use the macro as we overide some behaviours
	namespace 
	{
		class ProcessModuleImpl : public ProcessModule
		{
		public:
			void RegisterObjects(Omega::bool_t, Omega::bool_t, const Omega::string_t&)
				{ /* NOP */ }

			void Term()
				{ fini(); }

		private:
			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(User::ChannelMarshalFactory,0)
					OBJECT_MAP_ENTRY(User::TcpProtocolHandler,0)
					OBJECT_MAP_ENTRY(User::HttpProtocolHandler,0)	
					{ 0,0,0,0,0,0 } 
				}; 
				return CreatorEntries; 
			}
		};
	}
	ProcessModuleImpl& UserGetModule()
	{
		static ProcessModuleImpl i;
		return i;
	}

	OMEGA_PRIVATE ProcessModuleImpl* GetModule()
	{
		return &(UserGetModule());
	}

	OMEGA_PRIVATE ModuleBase* GetModuleBase()
		{ return GetModule(); }
}

using namespace Omega;
using namespace OTL;

// UserManager
const ACE_CDR::ULong User::Manager::m_root_channel = 0x80000000;

User::Manager::Manager() :
	m_nIPSCookie(0),
	m_bIsSandbox(false),
	m_nNextRemoteChannel(0)
{
}

User::Manager::~Manager()
{
}

int User::Manager::run(const ACE_CString& strPipe)
{
	return USER_MANAGER::instance()->run_event_loop_i(strPipe);
}

int User::Manager::run_event_loop_i(const ACE_CString& strPipe)
{
	int ret = start();
	if (ret != EXIT_FAILURE)
	{
		if (init(strPipe))
		{
			// Wait for stop
			if (ACE_Reactor::instance()->run_reactor_event_loop() != 0)
                ret = EXIT_FAILURE;

			// Stop the services
			stop_services();

			// Close all the sinks
			close_all_remotes();

			// Close all the http stuff
			close_all_http();

			// Close the user pipes
			close();

			// Unregister our object factories
			GetModule()->UnregisterObjectFactories();

			// Unregister InterProcessService
			if (m_nIPSCookie)
			{
				Activation::RevokeObject(m_nIPSCookie);
				m_nIPSCookie = 0;
			}

			// Close the OOCore
			Omega::Uninitialize();
		}

		// Delete our OTL module
		UserGetModule().Term();

		// Stop the MessageHandler
		stop();
	}

	return ret;
}

bool User::Manager::on_channel_open(ACE_CDR::ULong channel)
{
	if (channel != m_root_channel)
	{
		try
		{
			create_object_manager(channel,guid_t::Null());
		}
		catch (IException* pE)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Exception thrown: %W - %W\n"),pE->GetDescription().c_str(),pE->GetSource().c_str()));
			pE->Release();
			return false;
		}
	}
	return true;
}

bool User::Manager::init(const ACE_CString& strPipe)
{
	// Connect to the root
	ACE_Time_Value wait(5);
	ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Thread_Mutex> pipe;
	if (Root::MessagePipe::connect(pipe,strPipe,&wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Root::MessagePipe::connect() failed")),false);

	// Read the sandbox channel
	ACE_CDR::ULong sandbox_channel = 0;
	if (pipe->recv(&sandbox_channel,sizeof(sandbox_channel)) != static_cast<ssize_t>(sizeof(sandbox_channel)))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Root::MessagePipe::recv() failed")));
		pipe->close();
		return false;
	}

	// Set the sandbox flag
	m_bIsSandbox = (sandbox_channel == 0);

	// Invent a new pipe name...
	ACE_CString strNewPipe = Root::MessagePipe::unique_name("oou");
	if (strNewPipe.empty())
	{
        ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Failed to create unique domain socket name")));
        pipe->close();
		return false;
	}

	// Then send back our port name
	size_t uLen = strNewPipe.length()+1;
	if (pipe->send(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)) ||
		pipe->send(strNewPipe.c_str(),uLen) != static_cast<ssize_t>(uLen))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("pipe.send() failed")));
		pipe->close();
		return false;
	}

	// Read our channel id
	ACE_CDR::ULong our_channel = 0;
	if (pipe->recv(&our_channel,sizeof(our_channel)) != static_cast<ssize_t>(sizeof(our_channel)))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Root::MessagePipe::recv() failed")));
		pipe->close();
		return false;
	}

	// Create a new MessageConnection
	Root::MessageConnection* pMC = 0;
	ACE_NEW_NORETURN(pMC,Root::MessageConnection(this));
	if (!pMC)
	{
	    pipe->close();
		return false;
	}

	// Open the root connection
	if (pMC->open(pipe,m_root_channel) == 0)
	{
	    pipe->close();
		return false;
	}

	// Init the handler
	set_channel(our_channel,0xFF000000,0x00FFF000,m_root_channel);

	// Now bootstrap
	if (!bootstrap(sandbox_channel))
	{
		pipe->close();
		return false;
	}

	// Now start accepting client connections
	if (m_process_acceptor.start(this,strNewPipe) != 0)
	{
		pipe->close();
		return false;
	}

	// Now queue the service start function
	if (!call_async_function_i(&service_start,this,0))
	{
		pipe->close();
		return false;
	}

	return true;
}

bool User::Manager::bootstrap(ACE_CDR::ULong sandbox_channel)
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
		m_nIPSCookie = Activation::RegisterObject(System::OID_InterProcessService,ptrIPS,Activation::InProcess,Activation::MultipleUse);

		// Now we have a ROT, register everything else
		GetModule()->RegisterObjectFactories();
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Exception thrown: %W\nAt: %W\n"),pE->GetDescription().c_str(),pE->GetSource().c_str()));
		pE->Release();
		return false;
	}

	return true;
}

void User::Manager::service_start(void* pParam, ACE_InputCDR&)
{
	Manager* pThis = static_cast<Manager*>(pParam);
	try
	{
		pThis->service_start_i();
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
	}
	catch (...)
	{
	}
}

bool User::Manager::start_service(const string_t& strName, const guid_t& oid)
{
	try
	{
		ObjectPtr<System::IService> ptrService(oid,Activation::InProcess);
		ptrService->Start();

		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<string_t,ObjectPtr<System::IService> >::iterator,bool> p = m_mapServices.insert(std::map<string_t,ObjectPtr<System::IService> >::value_type(strName,ptrService));
		if (!p.second)
			OMEGA_THROW(L"Service with the same name already started!");
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception starting service %W: %W\n"),pE->GetSource().c_str(),strName.c_str(),pE->GetDescription().c_str()));

		pE->Release();

		return false;
	}

	return true;
}

void User::Manager::service_start_i()
{
	// Start the builtin services
	start_service(L"tcp",OID_TcpProtocolHandler);
	start_service(L"http",OID_HttpProtocolHandler);
}

void User::Manager::stop_services()
{
	try
	{
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		// Copy and clear the map
		std::map<string_t,ObjectPtr<System::IService> > mapServices = m_mapServices;
		m_mapServices.clear();

		guard.release();

		for (std::map<string_t,ObjectPtr<System::IService> >::iterator i=mapServices.begin();i!=mapServices.end();++i)
		{
			try
			{
				i->second->Stop();
			}
			catch (IException* pE)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception stopping service %W: %W\n"),pE->GetSource().c_str(),i->first.c_str(),pE->GetDescription().c_str()));

				pE->Release();
			}
		}
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception stopping services: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
	}
}

void User::Manager::end()
{
	// Stop accepting new clients
	m_process_acceptor.stop();

	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();
}

int User::Manager::on_accept(const ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Thread_Mutex>& pipe)
{
	Root::MessageConnection* pMC = 0;
	ACE_NEW_RETURN(pMC,Root::MessageConnection(this),-1);

	ACE_CDR::ULong channel_id = pMC->open(pipe,0,false);
	if (channel_id == 0)
	{
		delete pMC;
		return -1;
	}

	if (pipe->send(&channel_id,sizeof(channel_id)) != sizeof(channel_id))
	{
		delete pMC;
		return -1;
	}

	if (!pMC->read())
	{
		delete pMC;
		return -1;
	}

	return 0;
}

void User::Manager::on_channel_closed(ACE_CDR::ULong channel)
{
	// Close the corresponding Object Manager
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
		try
		{
			for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();)
			{
				bool bErase = false;
				if (i->first == channel)
				{
					// Close if its an exact match
					bErase = true;
				}
				else if ((i->first & 0xFFFFF000) == channel)
				{
					// Close all subchannels
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
	}

	// Give the remote layer a chance to close channels
	local_channel_closed(channel);

	// If the root closes, we should end!
	if (channel == m_root_channel)
		end();
}

void User::Manager::process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	if (src_channel_id == m_root_channel)
		process_root_request(request,seq_no,src_thread_id,deadline,attribs);
	else
		process_user_request(request,seq_no,src_channel_id,src_thread_id,deadline,attribs);
}

void User::Manager::process_root_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	Root::RootOpCode_t op_code;
	request >> op_code;

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Bad request")));
		return;
	}

	ACE_OutputCDR response;
	switch (op_code)
	{
	case Root::HttpOpen:
		open_http(request,response);
		break;

	case Root::HttpRecv:
		recv_http(request);
		break;

	case 0:
	default:
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Bad request op_code: %u\n"),op_code));
		return;
	}

	if (!(attribs & TypeInfo::Asynchronous) && response.good_bit())
		send_response(seq_no,m_root_channel,src_thread_id,response.begin(),deadline,attribs);
}

void User::Manager::process_user_request(const ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id,guid_t::Null());

		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrEnvelope;
		ptrEnvelope = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
		ptrEnvelope->init(request);

		// Unpack the payload
		IObject* pPayload = 0;
		ptrOM->UnmarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
		ObjectPtr<Remoting::IMessage> ptrRequest;
		ptrRequest.Attach(static_cast<Remoting::IMessage*>(pPayload));

		// Check timeout
		uint32_t timeout = 0;
		if (deadline != ACE_Time_Value::max_time)
		{
			ACE_Time_Value now = ACE_OS::gettimeofday();
			if (deadline <= now)
			{
				ACE_OS::last_error(ETIMEDOUT);
				return;
			}
			timeout = (deadline - now).msec();
		}

		// Make the call
		ObjectPtr<Remoting::IMessage> ptrResult;
		ptrResult.Attach(ptrOM->Invoke(ptrRequest,timeout));

		if (!(attribs & TypeInfo::Asynchronous))
		{
			// Wrap the response...
			ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
			ptrOM->MarshalInterface(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

			// Send it back...
			const ACE_Message_Block* mb = static_cast<const ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			if (!send_response(seq_no,src_channel_id,src_thread_id,mb,deadline,attribs))
				ptrOM->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
		}
	}
	catch (IException* pOuter)
	{
		// Just drop the exception, and let it pass...
		pOuter->Release();
	}
}

ObjectPtr<Remoting::IObjectManager> User::Manager::create_object_manager(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel(src_channel_id,message_oid);
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());
	return ptrOM;
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	return USER_MANAGER::instance()->create_channel_i(src_channel_id,message_oid);
}

ObjectPtr<ObjectImpl<User::Channel> > User::Manager::create_channel_i(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	try
	{
		// Lookup existing..
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(src_channel_id);
			if (i != m_mapChannels.end())
				return i->second;
		}

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
		ptrChannel->init(this,src_channel_id,classify_channel(src_channel_id),message_oid);

		// And add to the map
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator,bool> p = m_mapChannels.insert(std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::value_type(src_channel_id,ptrChannel));
		if (!p.second)
		{
			if (p.first->second->GetMarshalFlags() != ptrChannel->GetMarshalFlags())
				OMEGA_THROW(EINVAL);

			ptrChannel = p.first->second;
		}

		return ptrChannel;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

ACE_InputCDR* User::Manager::sendrecv_root(const ACE_OutputCDR& request, TypeInfo::MethodAttributes_t attribs)
{
	// The timeout needs to be related to the request timeout...
	ACE_Time_Value wait = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = ACE_OS::gettimeofday() + ACE_Time_Value(msecs / 1000,(msecs % 1000) * 1000);
	}

	ACE_InputCDR* response = 0;
	if (!send_request(m_root_channel,request.begin(),response,wait == ACE_Time_Value::max_time ? 0 : &wait,attribs))
		OMEGA_THROW(ACE_OS::last_error());

	return response;
}

bool User::Manager::call_async_function(void (*pfnCall)(void*,ACE_InputCDR&), void* pParam, const ACE_Message_Block* mb)
{
	return USER_MANAGER::instance()->call_async_function_i(pfnCall,pParam,mb);
}
