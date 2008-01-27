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

bool Root::Manager::install(int argc, wchar_t* argv[])
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening registry"),false);

	// Add the default keys
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive = ROOT_MANAGER::instance()->get_registry();
	ptrHive->create_key(L"All Users",false);
	ptrHive->create_key(L"Local User",false);
	ptrHive->create_key(L"Objects",false);
	ptrHive->create_key(L"Objects\\OIDs",false);
	ptrHive->create_key(L"Server",false);
	ptrHive->create_key(L"Server\\Sandbox",false);

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
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening registry"),false);

	if (!SpawnedProcess::UninstallSandbox())
		return false;

	return true;
}

int Root::Manager::run(int argc, wchar_t* argv[])
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

int Root::Manager::run_event_loop_i(int /*argc*/, wchar_t* /*argv*/[])
{
	int ret = -1;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 1)
		threads = 1;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,request_worker_fn,this);
	if (req_thrd_grp_id == -1)
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"spawn() failed"));
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads+1,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"spawn() failed"));
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
	if (init_registry() != 0)
		return false;

	// Setup the handler
	set_channel(0x80000000,0x80000000,0x7F000000,0x40000000);	

	// Spawn the sandbox
	ACE_WString strPipe;
	m_sandbox_channel = spawn_user(static_cast<user_id_type>(0),strPipe,0);
	if (!m_sandbox_channel)
		return false;

	void* TODO; // Accept the network channel here, and assign it to id 0x40000000

	return true;
}

int Root::Manager::init_registry()
{
	ACE_NEW_RETURN(m_registry,RegistryHive(),-1);

#if defined(ACE_WIN32)

	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if FAILED(hr)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] SHGetFolderPath failed: %#x\n",GetLastError()),-1);
	else
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (!PathCombineW(szBuf2,szBuf,L"Omega Online"))
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] PathCombine failed: %#x\n",GetLastError()),-1);
		else
		{
			if (!PathFileExistsW(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"mkdir failed"),-1);
			}

			if (!PathCombineW(szBuf,szBuf2,L"system.regdb"))
				ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] PathCombine failed: %#x\n",GetLastError()),-1);
			else
				m_strRegistry = szBuf;
		}
	}

#else

	#define OMEGA_REGISTRY_DIR L"/var/lib/omegaonline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"mkdir failed"),-1);
	}
	m_strRegistry = OMEGA_REGISTRY_DIR L"/system.regdb";

#endif

	if (m_registry->open(m_strRegistry.c_str()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"registry open() failed"),-1);

	return 0;
}

void Root::Manager::end_event_loop_i()
{
	// Stop accepting new clients
	m_client_acceptor.stop();

	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();
}

void Root::Manager::channel_closed(ACE_CDR::ULong channel)
{
	if (channel == m_sandbox_channel || classify_channel(channel)==4)
	{
		// Propogate the message... 
		void* TODO;
	}
}

int Root::Manager::process_client_connects()
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	const wchar_t* pipe_name = L"ooserver";
#else
	const wchar_t* pipe_name = L"/var/ooserver";
#endif

	if (m_client_acceptor.start(this,pipe_name) != 0)
		return -1;

	return ACE_Reactor::instance()->run_reactor_event_loop();
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
int Root::Manager::on_accept(ACE_SPIPE_Stream& pipe)
{
#else
int Root::Manager::on_accept(MessagePipe& pipe)
{
#endif

	user_id_type uid = 0;

	// Read the uid - we must read even for Windows
	if (pipe.recv(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"recv() failed"),-1);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (!ImpersonateNamedPipeClient(pipe.get_handle()))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] ImpersonateNamedPipeClient failed: %#x\n",GetLastError()),-1);

	BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&uid);
	if (!RevertToSelf())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] RevertToSelf failed: %#x\n",GetLastError()));
		CloseHandle(uid);
		ACE_OS::exit(-1);
	}
	if (!bRes)
	{
		CloseHandle(uid);
		return -1;
	}
#endif

	ACE_WString strPipe;
	if (connect_client(uid,strPipe))
	{
		size_t uLen = strPipe.length()+1;
		pipe.send(&uLen,sizeof(uLen));
		pipe.send(strPipe.c_str(),uLen*sizeof(wchar_t));
	}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	CloseHandle(uid);
#endif

	pipe.close();
	return 0;
}

ACE_CDR::ULong Root::Manager::spawn_user(user_id_type uid, ACE_WString& strPipe, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrRegistry)
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
	ACE_WString strNewPipe = MessagePipe::unique_name(L"oor");
	if (acceptor.open(strNewPipe.c_str(),uid) != 0)
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"));
	else 
	{
		// Spawn process
		if (pSpawn->Spawn(uid,strNewPipe,bSandbox))
		{
			// Init the registry, if necessary
			bool bOk = true;
			if (ptrRegistry.null())
			{
				ACE_NEW_NORETURN(ptrRegistry,RegistryHive);
				bOk = (!ptrRegistry.null() && ptrRegistry->open(pSpawn->GetRegistryHive())==0);
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
							ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %m\n"));
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
								ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] std::exception thrown %C\n",e.what()));
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
									ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"));
									bOk = false;
								}
								
								if (!bOk)
								{
									try
									{
										ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

										m_mapUserProcesses.erase(nChannelId);
									}
									catch (...)
									{	
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
	catch (...)
	{}
}

ACE_WString Root::Manager::bootstrap_user(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe)
{
	if (pipe->send(&m_sandbox_channel,sizeof(m_sandbox_channel)) != sizeof(m_sandbox_channel))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"));
		return L"";
	}

	size_t uLen = 0;
	if (pipe->recv(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.recv() failed"));
		return L"";
	}

	// Check for the integer overflow...
	if (uLen > (size_t)-1 / sizeof(wchar_t))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Overflow on buffer size"));
		return L"";
	}

	wchar_t* buf;
	ACE_NEW_RETURN(buf,wchar_t[uLen],L"");

	if (pipe->recv(buf,uLen*sizeof(wchar_t)) != static_cast<ssize_t>(uLen*sizeof(wchar_t)))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.recv() failed"));
		delete [] buf;
		return L"";
	}

	ACE_WString strRet = buf;
	delete [] buf;

	return strRet;
}

bool Root::Manager::connect_client(user_id_type uid, ACE_WString& strPipe)
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
	catch (std::exception&)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Unhandled exception\n"));
	}

	return false;
}

void Root::Manager::process_request(ACE_InputCDR& request, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	RootOpCode_t op_code;
	request >> op_code;

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Bad request"));
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

	case GetUInt32Value:
		registry_get_uint_value(src_channel_id,request,response);
		break;

	case GetBinaryValue:
		registry_get_binary_value(src_channel_id,request,response);
		break;

	case SetStringValue:
		registry_set_string_value(src_channel_id,request,response);
		break;

	case SetUInt32Value:
		registry_set_uint_value(src_channel_id,request,response);
		break;

	case SetBinaryValue:
		registry_set_binary_value(src_channel_id,request,response);
		break;

	case EnumValues:
		registry_enum_values(src_channel_id,request,response);
		break;

	case DeleteValue:
		registry_delete_value(src_channel_id,request,response);
		break;

	default:
		response.write_long(EINVAL);
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Bad request op_code\n"));
		break;
	}

	if (response.good_bit() && !(attribs & 1))
		send_response(src_channel_id,src_thread_id,response.begin(),deadline,attribs);
}

// Annoyingly this is missing from ACE...  A direct lift and translate of the ACE_CString version
static ACE_CDR::Boolean read_wstring(ACE_InputCDR& stream, ACE_WString& x)
{
	ACE_CDR::WChar *data = 0;
	if (stream.read_wstring(data))
	{
		x = data;
		delete [] data;
		return true;
	}

	x = L"";
	return stream.good_bit();
}

int Root::Manager::registry_open_hive(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex>& ptrHive, ACE_WString& strKey, bool bForWrite)
{
	// Read strKey
	if (!read_wstring(request,strKey))
		return ACE_OS::last_error();

	bool bCurrent = false;
	if (!strKey.empty())
	{
		if (strKey[0] != L'\\')
			return EINVAL;

		// Parse strKey
		if (strKey.substr(0,12) == L"\\Local User\\")
		{
			strKey = strKey.substr(12);
			bCurrent = true;
		}
		else if (strKey == L"\\Local User")
		{
			strKey.clear();
			bCurrent = true;
		}
		else
		{
			// Drop leading '\'
			strKey = strKey.substr(1);
		}
	}
	
	bool bAccessCheck = (!bCurrent && bForWrite);
	if (bAccessCheck)
	{
		if (strKey.substr(0,10) == L"\\All Users\\" || strKey == L"\\All Users")
			bAccessCheck = false;
	}
	
	if (bAccessCheck || bCurrent)
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

		// Find the process info
		std::map<ACE_CDR::ULong,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
		if (i == m_mapUserProcesses.end())
			return EINVAL;

		if (bAccessCheck)
		{
			// Check access
			bool bAllowed = false;
			if (!i->second.pSpawn->CheckAccess(m_strRegistry.c_str(),O_RDWR,bAllowed))
				return ACE_OS::last_error();
			else if (!bAllowed)
				return EACCES;
		}
		else
		{
			// Get the registry hive
			ptrHive = i->second.ptrRegistry;
		}
	}

	if (!bCurrent)
	{
		// Return the system hive
		ptrHive = m_registry;
	}

	return 0;
}

void Root::Manager::registry_key_exists(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Boolean bRes = false;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,false);
	if (err == 0)
	{
		err = ptrHive->key_exists(strKey);
		if (err == 0)
			bRes = true;
		else if (err == ENOENT)
			err = 0;
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_boolean(bRes);
}

void Root::Manager::registry_create_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_CDR::Boolean bFailIfThere = 0;
		if (!request.read_boolean(bFailIfThere))
			err = ACE_OS::last_error();
		else
			err = ptrHive->create_key(strKey,bFailIfThere);
	}

	response << err;
}

void Root::Manager::registry_delete_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
    ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_WString strSubKey;
		if (!read_wstring(request,strSubKey))
			err = ACE_OS::last_error();
		else
		{
			// Check for reserved keys under the system hive
			if (ptrHive == m_registry)
			{
				static const wchar_t* reserved[] =
				{
					L"\\",
					L"\\All Users",
					L"\\Objects",
					L"\\Objects\\OIDs",
					L"\\Server",
					L"\\Server\\Sandbox",
					0
				};

				ACE_WString strCheck = strKey + L"\\" + strSubKey;
				for (size_t i=0;reserved[i] != 0;++i)
				{
					if (strCheck == reserved[i])
					{
						err = EACCES;
						break;
					}
				}
			}

			if (err == 0)
				err = ptrHive->delete_key(strKey,strSubKey);
		}
	}
	
	response << err;
}

void Root::Manager::registry_enum_subkeys(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
		ptrHive->enum_subkeys(strKey,response);
	
	if (err != 0)
		response << err;
}

void Root::Manager::registry_value_type(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Octet type = 0;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,false);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_value_type(strKey,strValue,type);
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_octet(type);
}

void Root::Manager::registry_get_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_WString val;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,false);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_string_value(strKey,strValue,val);
	}

	response << err;
	if (err == 0)
		response.write_wstring(val.c_str());		
}

void Root::Manager::registry_get_uint_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::ULong val = 0;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,false);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_integer_value(strKey,strValue,val);
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_ulong(val);
}

void Root::Manager::registry_get_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,false);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::ULong cbLen = 0;
			if (!request.read_ulong(cbLen))
				err = ACE_OS::last_error();
			else
				ptrHive->get_binary_value(strKey,strValue,cbLen,response);
		}
	}

	if (err != 0)
		response << err;
}

void Root::Manager::registry_set_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_WString val;
			if (!read_wstring(request,val))
				err = ACE_OS::last_error();
			else
				err = ptrHive->set_string_value(strKey,strValue,val.c_str());
		}
	}

	response << err;
}

void Root::Manager::registry_set_uint_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::ULong val;
			if (!request.read_ulong(val))
				err = ACE_OS::last_error();
			else
				err = ptrHive->set_integer_value(strKey,strValue,val);
		}
	}

	response << err;
}

void Root::Manager::registry_set_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->set_binary_value(strKey,strValue,request);
	}

	response << err;
}

void Root::Manager::registry_enum_values(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
		ptrHive->enum_values(strKey,response);
	
	if (err != 0)
		response << err;
}

void Root::Manager::registry_delete_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_WString strKey;
	int err = registry_open_hive(channel_id,request,ptrHive,strKey,true);
	if (err == 0)
	{
		ACE_WString strValue;
		if (!read_wstring(request,strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->delete_value(strKey,strValue);
	}

	response << err;
}
