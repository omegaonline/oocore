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
	m_config_file(ACE_INVALID_HANDLE)
	//,m_bThreaded(false)
{
}

Root::Manager::~Manager()
{
	term();
}

bool Root::Manager::install(int argc, ACE_TCHAR* argv[])
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening registry")),false);

	if (!SpawnedProcess::InstallSandbox(argc,argv))
		return false;

	// Add the default All Users key
	ACE_Configuration_Section_Key res;
	ROOT_MANAGER::instance()->m_registry.open_section(ROOT_MANAGER::instance()->m_registry.root_section(),"All Users",1,res);

	return true;
}

bool Root::Manager::uninstall()
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening registry")),false);

	if (!SpawnedProcess::UninstallSandbox())
		return false;

	return true;
}

int Root::Manager::run(int argc, ACE_TCHAR* argv[])
{
	return ROOT_MANAGER::instance()->run_event_loop_i(argc,argv);
}

int Root::Manager::run_event_loop_i(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
	/*if (argc > 0 && ACE_OS::strcmp(argv[0],"threaded")==0)
        m_bThreaded = true;*/

	// Init
	int ret = init();
	if (ret != 0)
		return ret;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the proactor threads
	int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,proactor_worker_fn);
	if (pro_thrd_grp_id == -1)
		ret = -1;
	else
	{
		// Spawn off the request threads
		int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads-1,request_worker_fn,this);
		if (req_thrd_grp_id == -1)
			ret = -1;
		else
		{
			//ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer has started successfully.")));

			// Treat this thread as a request worker as well
			ret = (int)request_worker_fn(this);

			// Wait for all the request threads to finish
			ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
		}

		// Wait for all the proactor threads to finish
		ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
	}

	//ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer has stopped.")));

	return ret;
}

ACE_THR_FUNC_RETURN Root::Manager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

ACE_THR_FUNC_RETURN Root::Manager::request_worker_fn(void* pParam)
{
	return (ACE_THR_FUNC_RETURN)(static_cast<Manager*>(pParam)->pump_requests() ? 0 : -1);
}

ACE_CString Root::Manager::get_bootstrap_filename()
{
#define OMEGA_BOOTSTRAP_FILE "ooserver.bootstrap"

#if defined(ACE_WIN32)

	ACE_CString strFilename = "C:\\" OMEGA_BOOTSTRAP_FILE;

	char szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_CURRENT,szBuf);
	if SUCCEEDED(hr)
	{
		char szBuf2[MAX_PATH] = {0};
		if (PathCombineA(szBuf2,szBuf,"Omega Online"))
		{
			if (!PathFileExistsA(szBuf2) && ACE_OS::mkdir(szBuf2) != 0)
				return strFilename;

			if (PathCombineA(szBuf,szBuf2,OMEGA_BOOTSTRAP_FILE))
				strFilename = szBuf;
		}
	}

	return strFilename;

#else

	#define OMEGA_BOOTSTRAP_DIR "/var/lock/omegaonline"

	// Ignore the errors, they will reoccur when we try to opne the file
	ACE_OS::mkdir(OMEGA_BOOTSTRAP_DIR,S_IRWXU | S_IRWXG | S_IROTH);

	return ACE_CString(OMEGA_BOOTSTRAP_DIR "/" OMEGA_BOOTSTRAP_FILE);

#endif
}

int Root::Manager::init()
{
    // Open the Server lock file
	if (m_config_file != ACE_INVALID_HANDLE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);

	m_config_file = ACE_OS::open(get_bootstrap_filename().c_str(),O_RDONLY);
	if (m_config_file != ACE_INVALID_HANDLE)
	{
		pid_t pid;
		if (ACE_OS::read(m_config_file,&pid,sizeof(pid)) == sizeof(pid))
		{
			// Check if the process is still running...
			if (ACE::process_active(pid)==1)
			{
				// Already running on this machine... Fail
				ACE_OS::close(m_config_file);
				m_config_file = ACE_INVALID_HANDLE;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
			}
		}
		ACE_OS::close(m_config_file);
	}

	int flags = O_WRONLY | O_CREAT | O_TRUNC;
#if defined(ACE_WIN32)
	flags |= O_TEMPORARY;
#endif

	m_config_file = ACE_OS::open(get_bootstrap_filename().c_str(),flags);
	if (m_config_file == ACE_INVALID_HANDLE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::init - open() failed")),-1);

	// Write our pid instead
	pid_t pid = ACE_OS::getpid();
	if (ACE_OS::write(m_config_file,&pid,sizeof(pid)) != sizeof(pid))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::init - pid write() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}

	// Bind a tcp socket
	ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	if (ACE_Asynch_Acceptor<ClientConnection>::open(sa) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::init - open() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}

	// Get our port number
	int len = sa.get_size ();
	sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
	if (ACE_OS::getsockname(ACE_Asynch_Acceptor<ClientConnection>::get_handle(),addr,&len) == -1)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::init - Failed to discover local port")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}
	sa.set_type(addr->sa_family);
	sa.set_size(len);

	// Write it back...
	u_short uPort = sa.get_port_number();
	if (ACE_OS::write(m_config_file,&uPort,sizeof(uPort)) != sizeof(uPort))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::init - port write() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}

	// Open the root registry and create a new sandbox...
	if (init_registry() != 0 || !spawn_sandbox())
	{
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}

	return 0;
}

int Root::Manager::init_registry()
{
#define OMEGA_REGISTRY_FILE "system.regdb"

#if defined(ACE_WIN32)

	m_strRegistry = "C:\\" OMEGA_REGISTRY_FILE;

	char szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if SUCCEEDED(hr)
	{
		char szBuf2[MAX_PATH] = {0};
		if (PathCombineA(szBuf2,szBuf,"Omega Online"))
		{
			if (!PathFileExistsA(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					return ret;
			}

			if (PathCombineA(szBuf,szBuf2,OMEGA_REGISTRY_FILE))
				m_strRegistry = szBuf;
		}
	}

#else

	#define OMEGA_REGISTRY_DIR "/var/lib/omegaonline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}
	m_strRegistry = OMEGA_REGISTRY_DIR "/" OMEGA_REGISTRY_FILE;

#endif

	return m_registry.open(m_strRegistry.c_str());
}

void Root::Manager::end()
{
	ROOT_MANAGER::instance()->end_event_loop_i();
}

void Root::Manager::end_event_loop_i()
{
	/*try
	{
		// Stop accepting
		ACE_OS::shutdown(get_handle(),ACE_SHUTDOWN_BOTH);

		{
			ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			// Shutdown all client handles
			for (std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.begin();j!=m_mapReverseChannelIds.end();++j)
			{
				ACE_OS::shutdown(j->first,ACE_SHUTDOWN_WRITE);
			}
		}

		// Wait for everyone to close
		ACE_Time_Value wait(15);
		ACE_Countdown_Time timeout(&wait);
		while (wait != ACE_Time_Value::zero)
		{
			ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			if (m_mapReverseChannelIds.empty())
				break;

			timeout.update();
		}
	}
	catch (...)
	{}*/

	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

void Root::Manager::term()
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	// Empty the map
	try
	{
		for (std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
		{
			delete i->second.pSpawn;
		}
		m_mapUserProcesses.clear();
		m_mapUserIds.clear();
	}
	catch (...)
	{}

	// Close the config file...
	if (m_config_file != ACE_INVALID_HANDLE)
	{
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
	}
}

ACE_Configuration_Heap& Root::Manager::get_registry()
{
	return ROOT_MANAGER::instance()->m_registry;
}

bool Root::Manager::spawn_sandbox()
{
	ACE_CString strUserId;
	if (SpawnedProcess::GetSandboxUid(strUserId))
	{
		// Spawn the sandbox
		ACE_CString strSource;
		u_short uPort;
		if (!spawn_client(static_cast<uid_t>(-1),strUserId,uPort,strSource))
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Manager::spawn_sandbox() failed")));
			return false;
		}
	}
	else
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Sandbox failed to start. See previous error for cause.\n")),false);

	return true;
}

bool Root::Manager::spawn_client(uid_t uid, const ACE_CString& strUserId, u_short& uNewPort, ACE_CString& strSource)
{
	// Alloc a new SpawnedProcess
	SpawnedProcess* pSpawn;
	//if (!m_bThreaded)
		ACE_NEW_RETURN(pSpawn,SpawnedProcess,false);
	/*else
		ACE_NEW_RETURN(pSpawn,SpawnedThread,false);*/

	// Open an acceptor
	ACE_INET_Addr addr((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Acceptor acceptor;
	int ret = acceptor.open(addr,0,PF_INET,1);
	if (ret != 0)
	{
		strSource = "Root::Manager::spawn_client - acceptor.open";
	}
	else
	{
		// Get the port we are accepting on
		ret = acceptor.get_local_addr(addr);
		if (ret != 0)
		{
			strSource = "Root::Manager::spawn_client - acceptor.get_local_addr";
		}
		else
		{
			// Spawn the user process
			if (!pSpawn->Spawn(uid,addr.get_port_number(),strSource))
			{
				ACE_OS::last_error(EINVAL);
				ret = -1;
			}
			else
			{
				// Accept a socket
				ACE_SOCK_Stream stream;
				ACE_Time_Value wait(15);
				ret = acceptor.accept(stream,0,&wait);
				if (ret != 0)
				{
					strSource = "Root::Manager::spawn_client - acceptor.accept";
				}
				else
				{
					if (!bootstrap_client(stream,uid == static_cast<uid_t>(-1),strSource))
					{
						ret = -1;
					}
					else if (stream.recv(&uNewPort,sizeof(uNewPort)) != static_cast<ssize_t>(sizeof(uNewPort)))
					{
						ret = -1;
						strSource = "Root::Manager::spawn_client - stream.recv";
					}
					if (ret == 0 && uNewPort == 0)
					{
						ret = -1;
						strSource = "Root::Manager::spawn_client - client process exited early";
					}
					
					if (ret == 0)
					{
						// Create a new MessageConnection
						ACE_HANDLE handle = stream.get_handle();
						MessageConnection* pMC = 0;
						ACE_NEW_NORETURN(pMC,MessageConnection(this));
						if (!pMC)
						{
							ret = -1;
							strSource = "Root::Manager::spawn_client - new MessageConnection";
						}
						else if (pMC->attach(handle) == 0)
						{
							ret = -1;
							strSource = "Root::Manager::spawn_client - MessageConnection::attach";
						}
						else
						{
							// Clear the handle in the stream, pMC now owns it
							stream.set_handle(ACE_INVALID_HANDLE);

							// Insert the data into various maps...
							try
							{
								ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

								UserProcess process = {uNewPort, pSpawn};
								m_mapUserProcesses.insert(std::map<ACE_CString,UserProcess>::value_type(strUserId,process));
								m_mapUserIds.insert(std::map<ACE_HANDLE,ACE_CString>::value_type(handle,strUserId));
							}
							catch (...)
							{
								ret = -1;
								strSource = "Root::Manager::spawn_client - unhandled exception";
							}
						}

						if (ret != 0)
							delete pMC;
					}

					stream.close();
				}
			}
		}

		acceptor.close();
	}

	if (ret != 0)
		delete pSpawn;

	return (ret == 0);
}

bool Root::Manager::bootstrap_client(ACE_SOCK_STREAM& stream, bool bSandbox, ACE_CString& strSource)
{
	// This could be changed to a struct if we wanted...
	ACE_CDR::UShort sandbox_channel = bSandbox ? 0 : 1;

	if (stream.send(&sandbox_channel,sizeof(sandbox_channel)) != sizeof(sandbox_channel))
	{
		strSource = "Root::Manager::bootstrap_client - send";
		return false;
	}

	return true;
}

bool Root::Manager::connect_client(uid_t uid, u_short& uNewPort, ACE_CString& strSource)
{
	return ROOT_MANAGER::instance()->connect_client_i(uid,uNewPort,strSource);
}

bool Root::Manager::connect_client_i(uid_t uid, u_short& uNewPort, ACE_CString& strSource)
{
	ACE_CString strUserId;
	if (!SpawnedProcess::ResolveTokenToUid(uid,strUserId,strSource))
		return false;

	try
	{
		// See if we have a process already
		UserProcess process = {0,0};
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.find(strUserId);
			if (i!=m_mapUserProcesses.end())
			{
				process = i->second;
			}
		}

		// See if its still running...
		if (process.pSpawn)
		{
			if (!process.pSpawn->IsRunning())
			{
				ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

				delete process.pSpawn;
				m_mapUserProcesses.erase(strUserId);
			}
			else
			{
				uNewPort = process.uPort;
				return true;
			}
		}

		return spawn_client(uid,strUserId,uNewPort,strSource);
	}
	catch (std::exception&)
	{
		strSource = "Root::Manager::connect_client_i - std::exception";
	}

	return false;
}

bool Root::Manager::access_check(ACE_HANDLE handle, const char* pszObject, ACE_UINT32 mode, bool& bAllowed)
{
	try
	{
		SpawnedProcess* pSpawn;
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			// Find the user id
			std::map<ACE_HANDLE,ACE_CString>::iterator i = m_mapUserIds.find(handle);
			if (i == m_mapUserIds.end())
			{
				ACE_OS::last_error(EINVAL);
				return false;
			}

			// Find the process info associated with user id
			std::map<ACE_CString,UserProcess>::iterator j = m_mapUserProcesses.find(i->second);
			if (j == m_mapUserProcesses.end())
			{
				ACE_OS::last_error(EINVAL);
				return false;
			}

			pSpawn = j->second.pSpawn;
		}

		return pSpawn->CheckAccess(pszObject,mode,bAllowed);
	}
	catch (...)
	{
		ACE_OS::last_error(EINVAL);
		return false;
	}
}

void Root::Manager::process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort /*attribs*/)
{
	RootOpCode_t op_code;
	request >> op_code;

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("Root context: Root request %u from %u(%u)"),op_code,reply_channel_id,src_channel_id));

	if (!request.good_bit())
		return;

	ACE_OutputCDR response;

	switch (op_code)
	{
	case KeyExists:
		registry_key_exists(handle,request,response);
		break;

	case CreateKey:
		registry_create_key(handle,request,response);
		break;

	case DeleteKey:
		registry_delete_key(handle,request,response);
		break;

	case EnumSubKeys:
		registry_enum_subkeys(handle,request,response);
		break;

	case ValueType:
		registry_value_type(handle,request,response);
		break;

	case GetStringValue:
		registry_get_string_value(handle,request,response);
		break;

	case GetUInt32Value:
		registry_get_uint_value(handle,request,response);
		break;

	case GetBinaryValue:
		registry_get_binary_value(handle,request,response);
		break;

	case SetStringValue:
		registry_set_string_value(handle,request,response);
		break;

	case SetUInt32Value:
		registry_set_uint_value(handle,request,response);
		break;

	case SetBinaryValue:
		registry_set_binary_value(handle,request,response);
		break;

	case EnumValues:
		registry_enum_values(handle,request,response);
		break;

	case DeleteValue:
		registry_delete_value(handle,request,response);
		break;

	default:
		response.write_long(EINVAL);
		break;
	}

	if (response.good_bit())
		send_response(src_channel_id,src_thread_id,response.begin(),deadline,0);
}

bool Root::Manager::registry_open_section(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, bool bAccessCheck)
{
	ACE_CString strKey;
	if (!request.read_string(strKey))
		return false;

	if (bAccessCheck)
	{
		bool bAllowed = false;
		if (strKey.substr(0,9) == "All Users")
			bAllowed = true;
		else if (!access_check(handle,m_strRegistry.c_str(),O_RDWR,bAllowed))
			return false;

		if (!bAllowed)
		{
			ACE_OS::last_error(EACCES);
			return false;
		}
	}

	if (strKey.length()==0)
		key = m_registry.root_section();
	else
	{
		if (m_registry.open_section(m_registry.root_section(),ACE_TEXT_CHAR_TO_TCHAR(strKey).c_str(),0,key) != 0)
			return false;
	}

	return true;
}

bool Root::Manager::registry_open_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, ACE_CString& strValue, bool bAccessCheck)
{
	if (!registry_open_section(handle,request,key,bAccessCheck))
		return false;

	if (!request.read_string(strValue))
		return false;

	return true;
}

void Root::Manager::registry_key_exists(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CDR::Boolean bRes = false;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		if (!registry_open_section(handle,request,key))
		{
			if (ACE_OS::last_error() != ENOENT)
				err = ACE_OS::last_error();
		}
		else
		{
			bRes = true;
		}
	}

	response.write_long(err);
	if (err == 0)
		response.write_boolean(bRes);
}

void Root::Manager::registry_create_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CString strKey;
	if (!request.read_string(strKey))
		err = ACE_OS::last_error();
	else
	{
        bool bAllowed = false;
		if (!access_check(handle,m_strRegistry.c_str(),O_RDWR,bAllowed))
			err = ACE_OS::last_error();
		else if (!bAllowed)
			err = EACCES;
		else
		{
			ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

			ACE_Configuration_Section_Key key;
			if (m_registry.open_section(m_registry.root_section(),ACE_TEXT_CHAR_TO_TCHAR(strKey).c_str(),1,key) != 0)
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
}

void Root::Manager::registry_delete_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;

	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		if (!registry_open_section(handle,request,key,true))
			err = ACE_OS::last_error();
		else
		{
			ACE_CString strSubKey;
			if (!request.read_string(strSubKey))
				err = ACE_OS::last_error();
			else
			{
				if (strSubKey == "All Users")
					err = EACCES;
				else if (m_registry.remove_section(key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey).c_str(),1) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response.write_long(err);
}

void Root::Manager::registry_enum_subkeys(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	std::list<ACE_TString> listSections;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		if (!registry_open_section(handle,request,key))
			err = ACE_OS::last_error();
		else
		{
			try
			{
				for (int index=0;;++index)
				{
					ACE_TString strSubKey;
					int e = m_registry.enumerate_sections(key,index,strSubKey);
					if (e == 0)
						listSections.push_back(strSubKey);
					else
					{
						if (e != 1)
							err = ACE_OS::last_error();
						break;
					}
				}
			}
			catch (std::exception&)
			{
				err = EINVAL;
			}
		}
	}

	response.write_long(err);
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listSections.size());
			for (std::list<ACE_TString>::iterator i=listSections.begin();i!=listSections.end();++i)
			{
				response.write_string(ACE_TEXT_ALWAYS_CHAR(*i));
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_value_type(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CDR::Octet type = 0;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Heap::VALUETYPE vtype;
			if (m_registry.find_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),vtype) == 0)
				type = static_cast<ACE_CDR::Octet>(vtype);
			else
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
	if (err == 0)
		response.write_octet(type);
}

void Root::Manager::registry_get_string_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CString strText;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_TString strTextT;
			if (m_registry.get_string_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),strTextT) != 0)
				err = ACE_OS::last_error();
			else
				strText = ACE_TEXT_ALWAYS_CHAR(strTextT);
		}
	}

	response.write_long(err);
	if (err == 0)
		response.write_string(strText);
}

void Root::Manager::registry_get_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CDR::ULong val = 0;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue))
			err = ACE_OS::last_error();
		else
		{
			if (m_registry.get_integer_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),val) != 0)
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
	if (err == 0)
		response.write_ulong(val);
}

void Root::Manager::registry_get_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	ACE_CDR::ULong len = 0;
	void* data = 0;
	bool bReplyWithData = false;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue))
			err = ACE_OS::last_error();
		else
		{
			if (!request.read_ulong(len))
				err = ACE_OS::last_error();
			else
			{
				bReplyWithData = (len != 0);
				size_t dlen = 0;
                if (m_registry.get_binary_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),data,dlen) != 0)
					err = ACE_OS::last_error();
				else if (len != 0)
					len = std::min(len,static_cast<ACE_CDR::ULong>(dlen));
				else
					len = static_cast<ACE_CDR::ULong>(dlen);
			}
		}
	}

	response.write_long(err);
	if (err == 0)
	{
		response.write_ulong(len);
		if (bReplyWithData)
			response.write_octet_array(static_cast<const ACE_CDR::Octet*>(data),len);
	}

	delete [] static_cast<char*>(data);
}

void Root::Manager::registry_set_string_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;

	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue,true))
			err = ACE_OS::last_error();
		else
		{
			ACE_CString strText;
			if (!request.read_string(strText))
				err = ACE_OS::last_error();
			else if (m_registry.set_string_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),ACE_TEXT_CHAR_TO_TCHAR(strText)) != 0)
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
}

void Root::Manager::registry_set_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;

	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue,true))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::ULong iValue;
			if (!request.read_ulong(iValue))
				err = ACE_OS::last_error();
			else if (m_registry.set_integer_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),iValue) != 0)
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
}

void Root::Manager::registry_set_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;

	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue,true))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::ULong len;
			if (!request.read_ulong(len))
				err = ACE_OS::last_error();
			else
			{
				// TODO - This could be made quicker by aligning the read_ptr and not copying... but not today...
				ACE_CDR::Octet* data = 0;
				ACE_NEW_NORETURN(data,ACE_CDR::Octet[len]);
				if (!data)
					err = ENOMEM;
				else
				{
					if (!request.read_octet_array(data,len))
						err = ACE_OS::last_error();
					else if (m_registry.set_binary_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),data,len) != 0)
						err = ACE_OS::last_error();

					delete [] data;
				}
			}
		}
	}

	response.write_long(err);
}

void Root::Manager::registry_enum_values(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;
	std::list<ACE_TString> listValues;

	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		if (!registry_open_section(handle,request,key))
			err = ACE_OS::last_error();
		else
		{
			try
			{
				for (int index=0;;++index)
				{
					ACE_TString strSubKey;
					ACE_Configuration_Heap::VALUETYPE type;
					int e = m_registry.enumerate_values(key,index,strSubKey,type);
					if (e == 0)
						listValues.push_back(strSubKey);
					else
					{
						if (e != 1)
							err = ACE_OS::last_error();
						break;
					}
				}
			}
			catch (std::exception&)
			{
				err = EINVAL;
			}
		}
	}

	response.write_long(err);
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listValues.size());
			for (std::list<ACE_TString>::iterator i=listValues.begin();i!=listValues.end();++i)
			{
				response.write_string(ACE_TEXT_ALWAYS_CHAR(*i));
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_delete_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Long err = 0;

	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_registry_lock);

		ACE_Configuration_Section_Key key;
		ACE_CString strValue;
		if (!registry_open_value(handle,request,key,strValue,true))
			err = ACE_OS::last_error();
		else if (m_registry.remove_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str()) != 0)
			err = ACE_OS::last_error();
	}

	response.write_long(err);
}
