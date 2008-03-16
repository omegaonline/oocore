///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

#include "OOServer.h"

#include "./UserManager.h"
#include "./InterProcessService.h"
#include "./Channel.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

BEGIN_PROCESS_OBJECT_MAP(L"")
	OBJECT_MAP_ENTRY_UNNAMED(User::ChannelMarshalFactory)
END_PROCESS_OBJECT_MAP()

int UserMain(const ACE_TString& strPipe)
{
	u_long options = ACE_Log_Msg::SYSLOG;

#if defined(OMEGA_DEBUG)
	// If this event exists, then we are being debugged
	HANDLE hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
	if (hDebugEvent)
	{
		options = ACE_Log_Msg::STDERR;

		// Wait for a bit, letting the caller attach a debugger
		WaitForSingleObject(hDebugEvent,60000);
		CloseHandle(hDebugEvent);
	}
#endif

	if (ACE_LOG_MSG->open(ACE_TEXT("OOServer"),options,ACE_TEXT("OOServer")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"Error opening logger"),-1);

	return User::Manager::run(strPipe);
}

using namespace Omega;
using namespace OTL;

// UserManager
User::Manager::Manager() :
	m_root_channel(0x80000000),  m_nIPSCookie(0)
{
}

User::Manager::~Manager()
{
}

int User::Manager::run(const ACE_TString& strPipe)
{
	return USER_MANAGER::instance()->run_event_loop_i(strPipe);
}

int User::Manager::run_event_loop_i(const ACE_TString& strPipe)
{
	int ret = -1;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 1)
		threads = 1;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,request_worker_fn,this);
	if (req_thrd_grp_id == -1)
		ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"Error spawning threads"));
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"Error spawning threads"));
		else
		{
			if (init(strPipe))
			{
				// Wait for stop
				ret = ACE_Reactor::instance()->run_reactor_event_loop();

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
			}

			// Stop the proactor
			ACE_Proactor::instance()->proactor_end_event_loop();
			
			// Wait for all the request threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop the MessageHandler
		stop();

		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	return ret;
}

bool User::Manager::channel_open(ACE_CDR::ULong channel)
{
	if (channel != m_root_channel)
	{
		try
		{
			create_object_manager(channel,Remoting::inter_process);
		}
		catch (IException* pE)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: Exception thrown: %ls - %ls\n",pE->Description().c_str(),pE->Source().c_str()));
			pE->Release();
			return false;
		}
	}
	return true;
}

bool User::Manager::init(const ACE_TString& strPipe)
{
	// Connect to the root
	ACE_Time_Value wait(5);
	ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Null_Mutex> pipe;
	if (Root::MessagePipe::connect(pipe,strPipe,&wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"Root::MessagePipe::connect() failed"),false);

	// Read the sandbox channel
	ACE_CDR::ULong sandbox_channel = 0;
	if (pipe->recv(&sandbox_channel,sizeof(sandbox_channel)) != static_cast<ssize_t>(sizeof(sandbox_channel)))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"Root::MessagePipe::recv() failed"));
		pipe->close();
		return false;
	}

	// Invent a new pipe name..
	ACE_TString strNewPipe = Root::MessagePipe::unique_name(ACE_TEXT("oou"));

	// Then send back our port name
	size_t uLen = strNewPipe.length()+1;
	if (pipe->send(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)) ||
		pipe->send(strNewPipe.c_str(),uLen*sizeof(wchar_t)) != static_cast<ssize_t>(uLen*sizeof(wchar_t)))
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"pipe.send() failed"),false);
	}

	// Read our channel id
	ACE_CDR::ULong our_channel = 0;
	if (pipe->recv(&our_channel,sizeof(our_channel)) != static_cast<ssize_t>(sizeof(our_channel)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"Root::MessagePipe::recv() failed"),false);
		
	// Create a new MessageConnection
	Root::MessageConnection* pMC = 0;
	ACE_NEW_RETURN(pMC,Root::MessageConnection(this),false);
		
	// Open the root connection
	if (pMC->open(pipe,m_root_channel) == 0)
	{
		delete pMC;
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

	return true;
}

bool User::Manager::bootstrap(ACE_CDR::ULong sandbox_channel)
{
	// Register our service
	try
	{
		ObjectPtr<Remoting::IObjectManager> ptrOMSb;
		if (sandbox_channel != 0)
			ptrOMSb = create_object_manager(sandbox_channel,Remoting::inter_user);

		ObjectPtr<Remoting::IObjectManager> ptrOMUser;
				
		ObjectPtr<ObjectImpl<InterProcessService> > ptrIPS = ObjectImpl<InterProcessService>::CreateInstancePtr();
		ptrIPS->Init(ptrOMSb,ptrOMUser,this);

		// Register our interprocess service InProcess so we can react to activation requests
		m_nIPSCookie = Activation::RegisterObject(Remoting::OID_InterProcessService,ptrIPS,Activation::InProcess,Activation::MultipleUse);

		// Now we have a ROT, register everything else
		GetModule()->RegisterObjectFactories();
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: Exception thrown: %ls - %ls\n",pE->Description().c_str(),pE->Source().c_str()));
		pE->Release();
		return false;
	}

	return true;
}

void User::Manager::end_event_loop()
{
	// Stop accepting new clients
	m_process_acceptor.stop();

	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();
}

int User::Manager::on_accept(const ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Null_Mutex>& pipe)
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

void User::Manager::channel_closed(ACE_CDR::ULong channel)
{
	// Close the corresponding Object Manager
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
		try
		{
			for (std::map<ACE_CDR::ULong,OMInfo>::iterator i=m_mapOMs.begin();i!=m_mapOMs.end();)
			{
				bool bErase = false;
				if ((i->first & 0xFFFFF000) == channel)
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
					i->second.m_ptrOM->Disconnect();
					m_mapOMs.erase(i++);
				}
				else
					++i;
			}
		}
		catch (...)
		{}
	}

	// If the root closes, we should end!
	if (channel == m_root_channel)
		end_event_loop();
}

ACE_THR_FUNC_RETURN User::Manager::proactor_worker_fn(void*)
{
	ACE_Proactor::instance()->proactor_run_event_loop();
	return 0;
}

ACE_THR_FUNC_RETURN User::Manager::request_worker_fn(void* pParam)
{
	static_cast<Manager*>(pParam)->pump_requests();
	return 0;
}

void User::Manager::process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	if (src_channel_id == m_root_channel)
		process_root_request(request,seq_no,deadline,attribs);
	else
		process_user_request(request,seq_no,src_channel_id,src_thread_id,deadline,attribs);
}

void User::Manager::process_root_request(ACE_InputCDR& request, ACE_CDR::ULong /*seq_no*/, const ACE_Time_Value& /*deadline*/, ACE_CDR::ULong /*attribs*/)
{
	Root::RootOpCode_t op_code;
	request >> op_code;

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"Bad request"));
		return;
	}

	switch (op_code)
	{
	case 0:
	default:
		ACE_ERROR((LM_ERROR,L"%N:%l: Bad request op_code\n"));
		return;
	}
}

void User::Manager::process_user_request(const ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id,classify_channel(src_channel_id));

		// Wrap up the request
		ObjectPtr<ObjectImpl<InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<InputCDR>::CreateInstancePtr();
		ptrRequest->init(request);

		// Create a response if required
		ObjectPtr<ObjectImpl<OutputCDR> > ptrResponse;
		if (!(attribs & Remoting::asynchronous))
		{
			ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
			ptrResponse->WriteByte(0);
		}

		// Check timeout
		if (deadline != ACE_Time_Value::max_time)
		{
			if (deadline <= ACE_OS::gettimeofday())
			{
				ACE_OS::last_error(ETIMEDOUT);
				return;
			}
		}
		uint64_t deadline_secs = deadline.sec();
		int32_t deadline_usecs = deadline.usec();

		try
		{
			ptrOM->Invoke(ptrRequest,ptrResponse,deadline_secs,deadline_usecs,src_channel_id,classify_channel(src_channel_id));
		}
		catch (IException* pInner)
		{
			// Make sure we release the exception
			ObjectPtr<IException> ptrInner;
			ptrInner.Attach(pInner);

			// Reply with an exception if we can send replies...
			if (!(attribs & Remoting::asynchronous))
			{
				// Dump the previous output and create a fresh output
				ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ptrOM->MarshalInterface(ptrResponse,pInner->ActualIID(),pInner);
			}
		}

		if (!(attribs & Remoting::asynchronous))
		{
			ACE_Message_Block* mb = static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			send_response(seq_no,src_channel_id,src_thread_id,mb,deadline,attribs);
			mb->release();
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (!(attribs & Remoting::asynchronous))
		{
			ACE_OutputCDR error;

			// Error code 1 - Exception raw
			error.write_octet(1);
			error.write_string(pOuter->Description().ToUTF8().c_str());
			error.write_string(pOuter->Source().ToUTF8().c_str());

			send_response(seq_no,src_channel_id,src_thread_id,error.begin(),deadline,attribs);
		}
	}
}

ObjectPtr<Remoting::IObjectManager> User::Manager::create_object_manager(ACE_CDR::ULong src_channel_id, Remoting::MarshalFlags_t marshal_flags)
{
	try
	{
		// Lookup existing..
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<ACE_CDR::ULong,OMInfo>::iterator i=m_mapOMs.find(src_channel_id);
			if (i != m_mapOMs.end())
			{
				if (i->second.m_marshal_flags == marshal_flags)
					return i->second.m_ptrOM;

				OMEGA_THROW(EINVAL);
			}
		}

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
		ptrChannel->init(src_channel_id);

		// Create a new OM
		OMInfo info;
		info.m_marshal_flags = marshal_flags;
		info.m_ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);

		// Associate it with the channel
		info.m_ptrOM->Connect(ptrChannel,marshal_flags);

		// And add to the map
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<ACE_CDR::ULong,OMInfo>::iterator,bool> p = m_mapOMs.insert(std::map<ACE_CDR::ULong,OMInfo>::value_type(src_channel_id,info));
		if (!p.second)
		{
			if (p.first->second.m_marshal_flags != info.m_marshal_flags)
				OMEGA_THROW(EINVAL);		

			info.m_ptrOM = p.first->second.m_ptrOM;
		}

		return info.m_ptrOM;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

ACE_InputCDR* User::Manager::sendrecv_root(const ACE_OutputCDR& request)
{
	ACE_InputCDR* response = 0;
	if (!send_request(m_root_channel,request.begin(),response,0,Remoting::synchronous))
	{
		if (ACE_OS::last_error() == ENOENT)
		{
			void* TODO;  // Throw a remoting error
		}
		OOSERVER_THROW_LASTERROR();
	}

	return response;
}
