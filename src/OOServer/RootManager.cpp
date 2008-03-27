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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./RootManager.h"
#include "./SpawnedProcess.h"
#include "./Protocol.h"

Root::Manager::Manager() :
	m_sandbox_channel(0)
{
}

Root::Manager::~Manager()
{
}

bool Root::Manager::install(int argc, ACE_TCHAR* argv[])
{
	if (ROOT_MANAGER::instance()->init_database() != 0)
		return false;

	// Add the default keys
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive = ROOT_MANAGER::instance()->get_registry();
	ACE_INT64 key = 0;
	ptrHive->create_key(key,"All Users",false,7,0);
	ptrHive->set_description(key,0,"A common key for all users");
	key = 0;
	ptrHive->create_key(key,"Local User",false,7,0);
	ptrHive->set_description(key,0,"This is a unique key for each user of the local computer");
	key = 0;
	ptrHive->create_key(key,"Applications",false,5,0);
	key = 0;
	ptrHive->create_key(key,"Objects",false,5,0);
	ptrHive->create_key(key,"OIDs",false,5,0);
	key = 0;
	ptrHive->create_key(key,"Server",false,4,0);
	ptrHive->create_key(key,"Sandbox",false,4,0);

	// Set up the sandbox user
	if (!SpawnedProcess::InstallSandbox(argc,argv))
		return false;

	// Now secure the files we will use...
	if (!SpawnedProcess::SecureFile(ROOT_MANAGER::instance()->m_strRegistry))
		return false;

	return true;
}

bool Root::Manager::uninstall()
{
	if (ROOT_MANAGER::instance()->init_database() != 0)
		return false;

	if (!SpawnedProcess::UninstallSandbox())
		return false;

	return true;
}

int Root::Manager::run(int argc, ACE_TCHAR* argv[])
{
	return ROOT_MANAGER::instance()->run_event_loop_i(argc,argv);
}

ACE_Refcounted_Auto_Ptr<Root::RegistryHive,ACE_Null_Mutex> Root::Manager::get_registry()
{
	return ROOT_MANAGER::instance()->m_registry;
}

void Root::Manager::end()
{
	ROOT_MANAGER::instance()->end_event_loop_i();
}

int Root::Manager::run_event_loop_i(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
	int ret = -1;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 1)
		threads = 1;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,request_worker_fn,this);
	if (req_thrd_grp_id == -1)
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("spawn() failed")));
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("spawn() failed")));
		else
		{
			if (init())
			{
				// Now just process client requests
				ret = process_client_connects();
			}

			// Close all pipes
			close();

			// Stop the proactor
			ACE_Proactor::instance()->proactor_end_event_loop();

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Close the user processes
		close_users();

		// Stop the MessageHandler
		stop();

		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	return ret;
}

ACE_THR_FUNC_RETURN Root::Manager::proactor_worker_fn(void*)
{
	ACE_Proactor::instance()->proactor_run_event_loop();
	return 0;
}

ACE_THR_FUNC_RETURN Root::Manager::request_worker_fn(void* pParam)
{
	static_cast<Manager*>(pParam)->pump_requests();
	return 0;
}

bool Root::Manager::init()
{
	// Open the root registry
	if (init_database() != 0)
		return false;

	// Setup the handler
	set_channel(0x80000000,0x80000000,0x7F000000,0);

	// Spawn the sandbox
	ACE_CString strPipe;
	m_sandbox_channel = spawn_user(static_cast<user_id_type>(0),strPipe,0);
	if (!m_sandbox_channel)
		return false;

	return true;
}

int Root::Manager::init_database()
{
#if defined(ACE_WIN32)

	char szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if FAILED(hr)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: SHGetFolderPath failed: %#x\n"),GetLastError()),-1);
	else
	{
		char szBuf2[MAX_PATH] = {0};
		if (!PathCombineA(szBuf2,szBuf,"Omega Online"))
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: PathCombine failed: %#x\n"),GetLastError()),-1);
		else
		{
			if (!PathFileExistsA(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("mkdir failed")),-1);
			}

			if (!PathCombineA(szBuf,szBuf2,"system.regdb"))
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: PathCombine failed: %#x\n"),GetLastError()),-1);
			else
				m_strRegistry = szBuf;
		}
	}

#else

	#define OMEGA_REGISTRY_DIR "/var/lib/omegaonline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("mkdir failed")),-1);
	}
	m_strRegistry = OMEGA_REGISTRY_DIR "/system.regdb";

#endif

	// Create a new database
	ACE_NEW_RETURN(m_db,Db::Database(),-1);

	if (m_db->open(m_strRegistry) != 0)
		return -1;

	ACE_NEW_RETURN(m_registry,RegistryHive(m_db),-1);

	if (m_registry->open() != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("registry open() failed")),-1);

	return 0;
}

void Root::Manager::end_event_loop_i()
{
	// Stop accepting new clients
	m_client_acceptor.stop();

	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();
}

bool Root::Manager::can_route(ACE_CDR::ULong src_channel, ACE_CDR::ULong dest_channel)
{
	// Only route to or from the sandbox
	return (src_channel == m_sandbox_channel || dest_channel == m_sandbox_channel);
}

void Root::Manager::channel_closed(ACE_CDR::ULong channel)
{
	// Close the channel
	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::ULong,UserProcess>::iterator i=m_mapUserProcesses.find(channel);
		if (i != m_mapUserProcesses.end())
		{
			delete i->second.pSpawn;
			m_mapUserProcesses.erase(i);
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,"%N:%l: std::exception thrown %s\n",e.what()));
	}
}

int Root::Manager::process_client_connects()
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	const char* pipe_name = "OOServer";
#else
	const char* pipe_name = "/tmp/omegaonline/ooserverd";
#endif

	if (m_client_acceptor.start(this,pipe_name) != 0)
		return -1;

	return ACE_Reactor::instance()->run_reactor_event_loop();
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
int Root::Manager::on_accept(const ACE_Refcounted_Auto_Ptr<ACE_SPIPE_Stream,ACE_Null_Mutex>& pipe)
{
#else
int Root::Manager::on_accept(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe)
{
#endif

	user_id_type uid = 0;

	// Read the uid - we must read even for Windows
	if (pipe->recv(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("recv() failed")),-1);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (!ImpersonateNamedPipeClient(pipe->get_handle()))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: ImpersonateNamedPipeClient failed: %#x\n"),GetLastError()),-1);

	BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&uid);
	if (!RevertToSelf())
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: RevertToSelf failed: %#x\n"),GetLastError()));
		CloseHandle(uid);
		ACE_OS::exit(-1);
	}
	if (!bRes)
	{
		CloseHandle(uid);
		return -1;
	}
#endif

	ACE_CString strPipe;
	if (connect_client(uid,strPipe))
	{
		size_t uLen = strPipe.length()+1;
		pipe->send(&uLen,sizeof(uLen));
		pipe->send(strPipe.c_str(),uLen);
	}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	CloseHandle(uid);
#endif

	return 0;
}

ACE_CDR::ULong Root::Manager::spawn_user(user_id_type uid, ACE_CString& strPipe, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrRegistry)
{
	// Stash the sandbox flag because we adjust uid...
	bool bSandbox = (uid == static_cast<user_id_type>(0));

	// Alloc a new SpawnedProcess
	SpawnedProcess* pSpawn = 0;
	ACE_NEW_RETURN(pSpawn,SpawnedProcess,0);

	if (bSandbox && !SpawnedProcess::LogonSandboxUser(uid))
	{
		delete pSpawn;
		return 0;
	}

	ACE_CDR::ULong nChannelId = 0;

	MessagePipeAcceptor acceptor;
	ACE_CString strNewPipe = MessagePipe::unique_name("oor");
	if (acceptor.open(strNewPipe,uid) != 0)
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.open() failed")));
	else
	{
		// Spawn process
		if (pSpawn->Spawn(uid,strNewPipe,bSandbox))
		{
			// Init the registry, if necessary
			bool bOk = true;
			if (ptrRegistry.null())
			{
				bOk = false;

				ACE_CString strDb = pSpawn->GetRegistryHive();
				if (!strDb.empty())
				{
                    // Create a new database
                    ACE_Refcounted_Auto_Ptr<Db::Database,ACE_Null_Mutex> db;
                    ACE_NEW_NORETURN(db,Db::Database());
                    if (!db.null() && db->open(pSpawn->GetRegistryHive()) == 0)
                    {
                        ACE_NEW_NORETURN(ptrRegistry,RegistryHive(db));
                        bOk = (!ptrRegistry.null() && ptrRegistry->open()==0);
                    }
				}
			}

			if (bOk)
			{
				// Accept
		#ifdef OMEGA_DEBUG
				ACE_Time_Value wait(120);
		#else
				ACE_Time_Value wait(30);
		#endif
				ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex> pipe;
				if (acceptor.accept(pipe,&wait) == 0)
				{
					strPipe = bootstrap_user(pipe);
					if (!strPipe.empty())
					{
						// Create a new MessageConnection
						MessageConnection* pMC = 0;
						ACE_NEW_NORETURN(pMC,MessageConnection(this));
						if (!pMC)
							ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %m\n")));
						else if ((nChannelId = pMC->open(pipe,0,false)) != 0)
						{
							// Insert the data into various maps...
							try
							{
								ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

								UserProcess process = {strPipe, pSpawn, ptrRegistry };
								m_mapUserProcesses.insert(std::map<ACE_CDR::ULong,UserProcess>::value_type(nChannelId,process));
							}
							catch (std::exception& e)
							{
								ACE_ERROR((LM_ERROR,"%N:%l: std::exception thrown %s\n",e.what()));
								nChannelId = 0;
							}

							if (nChannelId)
							{
								if (!pMC->read())
								{
									bOk = false;
								}
								else if (pipe->send(&nChannelId,sizeof(nChannelId)) != sizeof(nChannelId))
								{
									ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("pipe.send() failed")));
									bOk = false;
								}

								if (!bOk)
								{
									try
									{
										ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

										m_mapUserProcesses.erase(nChannelId);
									}
									catch (std::exception& e)
									{
										ACE_ERROR((LM_ERROR,"%N:%l: std::exception thrown %s\n",e.what()));
									}
								}
							}
						}

						if (!nChannelId)
							delete pMC;
					}

					if (!nChannelId)
						pipe->close();
				}
			}
		}

		acceptor.close();
	}

	if (bSandbox)
		SpawnedProcess::CloseSandboxLogon(uid);

	if (!nChannelId)
		delete pSpawn;

	return nChannelId;
}

void Root::Manager::close_users()
{
	// Close all connections to user processes
	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::ULong,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
		{
			delete i->second.pSpawn;
		}
		m_mapUserProcesses.clear();
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,"%N:%l: std::exception thrown %s\n",e.what()));
	}
}

ACE_CString Root::Manager::bootstrap_user(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe)
{
	if (pipe->send(&m_sandbox_channel,sizeof(m_sandbox_channel)) != sizeof(m_sandbox_channel))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("pipe.send() failed")));
		return "";
	}

	size_t uLen = 0;
	if (pipe->recv(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("pipe.recv() failed")));
		return "";
	}

	char* buf;
	ACE_NEW_RETURN(buf,char[uLen],"");

	if (pipe->recv(buf,uLen) != static_cast<ssize_t>(uLen))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("pipe.recv() failed")));
		delete [] buf;
		return "";
	}

	ACE_CString strRet(buf);
	delete [] buf;

	return strRet;
}

bool Root::Manager::connect_client(user_id_type uid, ACE_CString& strPipe)
{
	try
	{
		ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrRegistry;

		// See if we have a process already
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			for (std::map<ACE_CDR::ULong,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
			{
				if (i->second.pSpawn->Compare(uid))
				{
					strPipe = i->second.strPipe;
					return true;
				}
				else if (i->second.pSpawn->IsSameUser(uid))
				{
					ptrRegistry = i->second.ptrRegistry;
				}
			}
		}

		return spawn_user(uid,strPipe,ptrRegistry) != 0;
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,"%N:%l: std::exception thrown %s\n",e.what()));
	}

	return false;
}

void Root::Manager::process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	RootOpCode_t op_code;
	request >> op_code;

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Bad request")));
		return;
	}

	ACE_OutputCDR response;
	switch (op_code)
	{
	case KeyExists:
		registry_key_exists(src_channel_id,request,response);
		break;

	case CreateKey:
		registry_create_key(src_channel_id,request,response);
		break;

	case DeleteKey:
		registry_delete_key(src_channel_id,request,response);
		break;

	case EnumSubKeys:
		registry_enum_subkeys(src_channel_id,request,response);
		break;

	case ValueType:
		registry_value_type(src_channel_id,request,response);
		break;

	case GetStringValue:
		registry_get_string_value(src_channel_id,request,response);
		break;

	case GetIntegerValue:
		registry_get_int_value(src_channel_id,request,response);
		break;

	case GetBinaryValue:
		registry_get_binary_value(src_channel_id,request,response);
		break;

	case SetStringValue:
		registry_set_string_value(src_channel_id,request,response);
		break;

	case SetIntegerValue:
		registry_set_int_value(src_channel_id,request,response);
		break;

	case SetBinaryValue:
		registry_set_binary_value(src_channel_id,request,response);
		break;

	case GetDescription:
		registry_get_description(src_channel_id,request,response);
		break;

	case GetValueDescription:
		registry_get_value_description(src_channel_id,request,response);
		break;

	case SetDescription:
		registry_set_description(src_channel_id,request,response);
		break;

	case SetValueDescription:
		registry_set_value_description(src_channel_id,request,response);
		break;

	case EnumValues:
		registry_enum_values(src_channel_id,request,response);
		break;

	case DeleteValue:
		registry_delete_value(src_channel_id,request,response);
		break;

	default:
		response.write_long(EINVAL);
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: Bad request op_code: %d\n"),op_code));
		break;
	}

	if (response.good_bit() && !(attribs & 1))
		send_response(seq_no,src_channel_id,src_thread_id,response.begin(),deadline,attribs);
}
