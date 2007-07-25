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

#if defined(ACE_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

Root::Manager::Manager() :
	m_config_file(ACE_INVALID_HANDLE)
{
}

Root::Manager::~Manager()
{
}

bool Root::Manager::install(int argc, wchar_t* argv[])
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening registry"),false);

	if (!SpawnedProcess::InstallSandbox(argc,argv))
		return false;

	// Add the default All Users key
	ACE_Configuration_Section_Key res;
	ROOT_MANAGER::instance()->m_registry.open_section(ROOT_MANAGER::instance()->m_registry.root_section(),L"All Users",1,res);

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

ACE_Configuration_Heap& Root::Manager::get_registry()
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
	if (threads < 2)
		threads = 2;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn,this);
	if (req_thrd_grp_id != -1)
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,proactor_worker_fn);
		if (pro_thrd_grp_id != -1)
		{
			if (init())
			{
				//ACE_DEBUG((LM_INFO,L"OOServer has started successfully."));

				// Now just process client requests
				ret = process_client_connects();
			}

			// Stop accepting clients
			ACE_Asynch_Acceptor<ClientConnection>::cancel();

			// Close the config file...
			ACE_OS::close(m_config_file);

			// Close the user processes
			close_users();

			// Stop the proactor
			ACE_Proactor::instance()->end_event_loop();

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop the MessageHandler
		stop();

		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	//ACE_DEBUG((LM_INFO,L"OOServer has stopped."));

	return ret;
}

ACE_THR_FUNC_RETURN Root::Manager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

ACE_THR_FUNC_RETURN Root::Manager::request_worker_fn(void* pParam)
{
	static_cast<Manager*>(pParam)->pump_requests();
	return 0;
}

bool Root::Manager::init()
{
	// Open the Server lock file
	if (m_config_file != ACE_INVALID_HANDLE)
		ACE_ERROR_RETURN((LM_ERROR,L"OOServer already running.\n"),false);

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
				ACE_ERROR_RETURN((LM_ERROR,L"OOServer already running.\n"),false);
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
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Root::Manager::init - open() failed"),false);

	// Write our pid instead
	pid_t pid = ACE_OS::getpid();
	if (ACE_OS::write(m_config_file,&pid,sizeof(pid)) != sizeof(pid))
	{
		ACE_ERROR((LM_ERROR,L"%p\n",L"Root::Manager::init - pid write() failed"));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return false;
	}

	// Bind a tcp socket
	ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	if (ACE_Asynch_Acceptor<ClientConnection>::open(sa) != 0)
	{
		ACE_ERROR((LM_ERROR,L"%p\n",L"Root::Manager::init - open() failed"));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return false;
	}

	// Get our port number
	int len = sa.get_size ();
	sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
	if (ACE_OS::getsockname(ACE_Asynch_Acceptor<ClientConnection>::get_handle(),addr,&len) == -1)
	{
		ACE_ERROR((LM_ERROR,L"%p\n",L"Root::Manager::init - Failed to discover local port"));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return false;
	}
	sa.set_type(addr->sa_family);
	sa.set_size(len);

	// Write it back...
	u_short uPort = sa.get_port_number();
	if (ACE_OS::write(m_config_file,&uPort,sizeof(uPort)) != sizeof(uPort))
	{
		ACE_ERROR((LM_ERROR,L"%p\n",L"Root::Manager::init - port write() failed"));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return false;
	}

	// Open the root registry and create a new sandbox...
	if (init_registry() != 0 || !spawn_sandbox())
	{
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return false;
	}

	return true;
}

ACE_WString Root::Manager::get_bootstrap_filename()
{
#define OMEGA_BOOTSTRAP_FILE L"ooserver.bootstrap"

#if defined(ACE_WIN32)

	ACE_WString strFilename = L"C:\\" OMEGA_BOOTSTRAP_FILE;

	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_CURRENT,szBuf);
	if SUCCEEDED(hr)
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (PathCombineW(szBuf2,szBuf,L"Omega Online"))
		{
			if (!PathFileExistsW(szBuf2) && ACE_OS::mkdir(szBuf2) != 0)
				return strFilename;

			if (PathCombineW(szBuf,szBuf2,OMEGA_BOOTSTRAP_FILE))
				strFilename = szBuf;
		}
	}

	return strFilename;

#else

	#define OMEGA_BOOTSTRAP_DIR L"/var/lock/omegaonline"

	// Ignore the errors, they will reoccur when we try to opne the file
	ACE_OS::mkdir(OMEGA_BOOTSTRAP_DIR,S_IRWXU | S_IRWXG | S_IROTH);

	return ACE_WString(OMEGA_BOOTSTRAP_DIR L"/" OMEGA_BOOTSTRAP_FILE);

#endif
}

int Root::Manager::init_registry()
{
#define OMEGA_REGISTRY_FILE L"system.regdb"

#if defined(ACE_WIN32)

	m_strRegistry = L"C:\\" OMEGA_REGISTRY_FILE;

	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if SUCCEEDED(hr)
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (PathCombineW(szBuf2,szBuf,L"Omega Online"))
		{
			if (!PathFileExistsW(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					return ret;
			}

			if (PathCombineW(szBuf,szBuf2,OMEGA_REGISTRY_FILE))
				m_strRegistry = szBuf;
		}
	}

#else

	#define OMEGA_REGISTRY_DIR L"/var/lib/omegaonline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}
	m_strRegistry = OMEGA_REGISTRY_DIR L"/" OMEGA_REGISTRY_FILE;

#endif

	return m_registry.open(m_strRegistry.c_str());
}

void Root::Manager::end_event_loop_i()
{
	m_queue_clients.deactivate();
}

void Root::ClientConnection::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	Root::Manager::connect_client(new_handle);
}

void Root::Manager::connect_client(ACE_HANDLE handle)
{
	ACE_HANDLE* ph = 0;
	ACE_NEW_NORETURN(ph,ACE_HANDLE(handle));
	if (!ph)
		return;

	ACE_Time_Value wait(1);
	if (ROOT_MANAGER::instance()->m_queue_clients.enqueue_tail(ph,&wait) == -1)
	{
		delete ph;
		ACE_OS::closesocket(handle);
	}
}

int Root::Manager::process_client_connects()
{
	int ret = 0;
	for (;;)
	{
		ACE_HANDLE* handle;
		ret = m_queue_clients.dequeue_head(handle);
		if (ret == -1)
		{
			if (ACE_OS::last_error() == ESHUTDOWN)
				ret = 0;
			break;
		}

		ACE_SOCK_Stream stream(*handle);
		delete handle;

		// Read the uid
		ACE_Time_Value wait(1);
		user_id_type uid;
		if (stream.recv(&uid,sizeof(uid),&wait) != static_cast<ssize_t>(sizeof(uid)))
			continue;

		u_short uPort = 0;
		ACE_WString strSource;
		if (!connect_client(uid,uPort,strSource))
		{
			int err = ACE_OS::last_error();
			uPort = 0;
			stream.send(&uPort,sizeof(uPort));
			stream.send(&err,sizeof(err));
			stream.send(strSource.c_str(),strSource.length());
		}
		else
            stream.send(&uPort,sizeof(uPort));
	}

	return ret;
}

bool Root::Manager::spawn_sandbox()
{
	ACE_CString strUserId;
	if (SpawnedProcess::GetSandboxUid(strUserId))
	{
		// Spawn the sandbox
		ACE_WString strSource;
		u_short uPort;
		if (!spawn_user(static_cast<user_id_type>(-1),strUserId,uPort,strSource))
		{
			ACE_ERROR((LM_ERROR,L"%p\n",L"Root::Manager::spawn_sandbox() failed"));
			return false;
		}
	}
	else
		ACE_ERROR_RETURN((LM_ERROR,L"Sandbox failed to start. See previous error for cause.\n"),false);

	return true;
}

bool Root::Manager::spawn_user(user_id_type uid, const ACE_CString& strUserId, u_short& uNewPort, ACE_WString& strSource)
{
	// Alloc a new SpawnedProcess
	SpawnedProcess* pSpawn;
	ACE_NEW_RETURN(pSpawn,SpawnedProcess,false);

	bool bSuccess = false;

	// Open an acceptor
	ACE_INET_Addr addr((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Acceptor acceptor;
	if (acceptor.open(addr,0,PF_INET,1) != 0)
		strSource = L"Root::Manager::spawn_user - acceptor.open";
	else
	{
		// Get the port we are accepting on
		if (acceptor.get_local_addr(addr) != 0)
			strSource = L"Root::Manager::spawn_user - acceptor.get_local_addr";
		else
		{
			// Spawn the user process
			if (pSpawn->Spawn(uid,addr.get_port_number(),strSource))
			{
				// Accept a socket
				ACE_SOCK_Stream stream;
				ACE_Time_Value wait(15);
				if (acceptor.accept(stream,0,&wait) != 0)
					strSource = L"Root::Manager::spawn_user - acceptor.accept";
				else
				{
					uNewPort = bootstrap_user(stream,uid == static_cast<user_id_type>(-1),strSource);
					if (uNewPort != 0)
					{
						// Create a new MessageConnection
						ACE_HANDLE handle = stream.get_handle();
						MessageConnection* pMC = 0;
						ACE_NEW_NORETURN(pMC,MessageConnection(this));
						if (!pMC)
							strSource = L"Root::Manager::spawn_user - new MessageConnection";
						else if (pMC->attach(handle) == 0)
							strSource = L"Root::Manager::spawn_user - MessageConnection::attach";
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

								bSuccess = true;
							}
							catch (...)
							{
								strSource = L"Root::Manager::spawn_user - unhandled exception";
							}
						}

						if (!bSuccess)
							delete pMC;
					}

					stream.close();
				}
			}
		}

		acceptor.close();
	}

	if (!bSuccess)
		delete pSpawn;

	return bSuccess;
}

void Root::Manager::close_users()
{
    // Tell all user processes to close
	ACE_OutputCDR request;
	request << static_cast<RootOpCode_t>(Root::End);
	if (request.good_bit())
	{
		try
		{
			// Iterate backwards
			for (std::map<ACE_HANDLE,ACE_CString>::reverse_iterator i=m_mapUserIds.rbegin();i!=m_mapUserIds.rend();++i)
			{
                ACE_CDR::UShort channel = get_handle_channel(i->first,0);

				ACE_InputCDR* response = 0;
				send_request(channel,request.begin(),response,1000,1);
			}
		}
		catch (...)
		{
		}
	}

	// Close all connections to user processes
	try
	{
		for (std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
		{
			delete i->second.pSpawn;
		}
	}
	catch (...)
	{}
}

u_short Root::Manager::bootstrap_user(ACE_SOCK_STREAM& stream, bool bSandbox, ACE_WString& strSource)
{
	// This could be changed to a struct if we wanted...
	ACE_CDR::UShort sandbox_channel = bSandbox ? 0 : 1;

	if (stream.send(&sandbox_channel,sizeof(sandbox_channel)) != sizeof(sandbox_channel))
	{
		strSource = L"Root::Manager::bootstrap_user - send";
		return 0;
	}

	u_short uPort = 0;
	ACE_Time_Value wait(15);
	if (stream.recv(&uPort,sizeof(uPort),&wait) != static_cast<ssize_t>(sizeof(uPort)))
	{
		strSource = L"Root::Manager::bootstrap_user - recv";
		return 0;
	}

	return uPort;
}

bool Root::Manager::connect_client(user_id_type uid, u_short& uNewPort, ACE_WString& strSource)
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

		return spawn_user(uid,strUserId,uNewPort,strSource);
	}
	catch (std::exception&)
	{
		strSource = L"Root::Manager::connect_client_i - std::exception";
	}

	return false;
}

bool Root::Manager::access_check(ACE_HANDLE handle, const wchar_t* pszObject, ACE_UINT32 mode, bool& bAllowed)
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

	//ACE_DEBUG((LM_DEBUG,L"Root context: Root request %u from %u(%u)",op_code,reply_channel_id,src_channel_id));

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

bool Root::Manager::registry_open_section(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, bool bAccessCheck)
{
	ACE_WString strKey;
	if (!read_wstring(request,strKey))
		return false;

	if (bAccessCheck)
	{
		bool bAllowed = false;
		if (strKey.substr(0,9) == L"All Users")
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
		if (m_registry.open_section(m_registry.root_section(),strKey.c_str(),0,key) != 0)
			return false;
	}

	return true;
}

bool Root::Manager::registry_open_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, ACE_WString& strValue, bool bAccessCheck)
{
	if (!registry_open_section(handle,request,key,bAccessCheck))
		return false;

	if (!read_wstring(request,strValue))
		return false;

	return true;
}

void Root::Manager::registry_key_exists(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::Boolean bRes = false;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
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
	}

	response << err;
	if (err == 0)
		response.write_boolean(bRes);
}

void Root::Manager::registry_create_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_WString strKey;
	if (!read_wstring(request,strKey))
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
			ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
			if (guard.locked () == 0)
				err = ACE_OS::last_error();
			else
			{
				ACE_Configuration_Section_Key key;
				if (m_registry.open_section(m_registry.root_section(),strKey.c_str(),1,key) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_delete_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(handle,request,key,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_WString strSubKey;
				if (!read_wstring(request,strSubKey))
					err = ACE_OS::last_error();
				else
				{
					if (strSubKey == L"All Users")
						err = EACCES;
					else if (m_registry.remove_section(key,strSubKey.c_str(),1) != 0)
						err = ACE_OS::last_error();
				}
			}
		}
	}

	response << err;
}

void Root::Manager::registry_enum_subkeys(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	std::list<ACE_WString> listSections;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(handle,request,key))
				err = ACE_OS::last_error();
			else
			{
				try
				{
					for (int index=0;;++index)
					{
						ACE_WString strSubKey;
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
	}

	response << err;
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listSections.size());
			for (std::list<ACE_WString>::iterator i=listSections.begin();i!=listSections.end();++i)
			{
				response.write_wstring(i->c_str());
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_value_type(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::Octet type = 0;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				ACE_Configuration_Heap::VALUETYPE vtype;
				if (m_registry.find_value(key,strValue.c_str(),vtype) == 0)
					type = static_cast<ACE_CDR::Octet>(vtype);
				else
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_octet(type);
}

void Root::Manager::registry_get_string_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_WString strText;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				if (m_registry.get_string_value(key,strValue.c_str(),strText) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_wstring(strText.c_str());
}

void Root::Manager::registry_get_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::ULong val = 0;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				if (m_registry.get_integer_value(key,strValue.c_str(),val) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_ulong(val);
}

void Root::Manager::registry_get_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::ULong len = 0;
	void* data = 0;
	bool bReplyWithData = false;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
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
					if (m_registry.get_binary_value(key,strValue.c_str(),data,dlen) != 0)
						err = ACE_OS::last_error();
					else if (len != 0)
						len = std::min(len,static_cast<ACE_CDR::ULong>(dlen));
					else
						len = static_cast<ACE_CDR::ULong>(dlen);
				}
			}
		}
	}

	response << err;
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
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_WString strText;
				if (!read_wstring(request,strText))
					err = ACE_OS::last_error();
				else if (m_registry.set_string_value(key,strValue.c_str(),strText) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_set_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_CDR::ULong iValue;
				if (!request.read_ulong(iValue))
					err = ACE_OS::last_error();
				else if (m_registry.set_integer_value(key,strValue.c_str(),iValue) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_set_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
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
						else if (m_registry.set_binary_value(key,strValue.c_str(),data,len) != 0)
							err = ACE_OS::last_error();

						delete [] data;
					}
				}
			}
		}
	}

	response << err;
}

void Root::Manager::registry_enum_values(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	std::list<ACE_WString> listValues;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(handle,request,key))
				err = ACE_OS::last_error();
			else
			{
				try
				{
					for (int index=0;;++index)
					{
						ACE_WString strValue;
						ACE_Configuration_Heap::VALUETYPE type;
						int e = m_registry.enumerate_values(key,index,strValue,type);
						if (e == 0)
							listValues.push_back(strValue);
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
	}

	response << err;
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listValues.size());
			for (std::list<ACE_WString>::iterator i=listValues.begin();i!=listValues.end();++i)
			{
				response.write_wstring(i->c_str());
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_delete_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(handle,request,key,strValue,true))
				err = ACE_OS::last_error();
			else if (m_registry.remove_value(key,strValue.c_str()) != 0)
				err = ACE_OS::last_error();
		}
	}

	response << err;
}
