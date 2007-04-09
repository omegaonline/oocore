/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#include "./RootManager.h"
#include "./SpawnedProcess.h"

#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Countdown_Time.h>

#include <OOCore/Preprocessor/base.h>

#include <list>

#include "./Protocol.h"

RootManager::RootManager() : 
	LocalAcceptor<ClientConnection>(),
	m_config_file(ACE_INVALID_HANDLE),
	m_uNextChannelId(1)
{
}

RootManager::~RootManager()
{
	term();
}

int RootManager::run_event_loop()
{
	return ROOT_MANAGER::instance()->run_event_loop_i();
}

int RootManager::run_event_loop_i()
{
	// Init
	int ret = init();
	if (ret != 0)
		return ret;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn);
	if (req_thrd_grp_id == -1)
		ret = -1;
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads-1,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ret = -1;
		else
		{
			ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer has started successfully.")));

			// Treat this thread as a worker as well
			ret = proactor_worker_fn(0);

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop handling requests
		RequestHandler<RequestBase>::stop();

		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer has stopped.")));

	return ret;
}

int RootManager::init()
{
    // Open the Server lock file
	if (m_config_file != ACE_INVALID_HANDLE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Already open.\n")),-1);

	m_config_file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_RDONLY);
	if (m_config_file != INVALID_HANDLE_VALUE)
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
		m_config_file = ACE_INVALID_HANDLE;
	}

	m_config_file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_WRONLY | O_CREAT | O_TRUNC | O_TEMPORARY);
	if (m_config_file == INVALID_HANDLE_VALUE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")),-1);

	// Write our pid instead
	pid_t pid = ACE_OS::getpid();
	if (ACE_OS::write(m_config_file,&pid,sizeof(pid)) != sizeof(pid))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}

	// Bind a tcp socket 
	ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	if (open(sa) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;
	}
	
	// Get our port number
	int len = sa.get_size ();
	sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
	if (ACE_OS::getsockname(this->get_handle(),addr,&len) == -1)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
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
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")));
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;	
	}

	// Open the root registry
	if (init_registry() != 0)
	{
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
		return -1;	
	}

	return 0;
}

int RootManager::init_registry()
{
#define OMEGA_REGISTRY_FILE "system.regdb"

#if defined(ACE_WIN32)

	m_strRegistry = "C:\\" OMEGA_REGISTRY_FILE;

	char szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathA(0,CSIDL_LOCAL_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if SUCCEEDED(hr)
	{
		char szBuf2[MAX_PATH] = {0};
		if (PathCombineA(szBuf2,szBuf,"OmegaOnline"))
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

	#define OMEGA_REGISTRY_DIR "/var/lib/OmegaOnline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}
	m_strRegistry = ACE_TEXT(OMEGA_REGISTRY_DIR "/" OMEGA_REGISTRY_FILE);

#endif

	return m_registry.open(m_strRegistry.c_str());
}

void RootManager::end_event_loop()
{
	ROOT_MANAGER::instance()->end_event_loop_i();
}

void RootManager::end_event_loop_i()
{
	try
	{
		// Stop accepting
		ACE_OS::shutdown(get_handle(),SD_BOTH);

		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);
	
			// Shutdown all client handles
			for (std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.begin();j!=m_mapReverseChannelIds.end();++j)
			{
				ACE_OS::shutdown(j->first,SD_SEND);
			}
		}

		// Wait for everyone to close
		ACE_Time_Value wait(15);
		ACE_Countdown_Time timeout(&wait);
		while (!timeout.stopped())
		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);
			if (m_mapReverseChannelIds.empty())
				break;

			timeout.update();
		}
	}
	catch (...)
	{}

	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

void RootManager::term()
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

	// Empty the map
	try
	{
		for (std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
		{
			if (i->second.pSpawn->Close() == ETIMEDOUT)
				i->second.pSpawn->Kill();

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

ACE_Configuration_Heap& RootManager::get_registry()
{
	return ROOT_MANAGER::instance()->m_registry;
}

int RootManager::spawn_sandbox()
{
	// Build a request manually
	Session::Request request;
	request.cbSize = sizeof(Session::Request);
	request.uid = static_cast<Session::TOKEN>(-1);
	
	// Build a response
	Session::Response response;
	response.cbSize = sizeof(response);
	response.bFailure = 0;

	ACE_CString strUserId;
	int ret = SpawnedProcess::GetSandboxUid(strUserId);
	if (ret != 0)
		return ret;

	// Spawn the sandbox
	spawn_client(request,response,strUserId);

	if (response.bFailure)
		return response.err;
	else
		return 0;
}

void RootManager::spawn_client(const Session::Request& request, Session::Response& response, const ACE_CString& strUserId)
{
	// Alloc a new SpawnedProcess
	SpawnedProcess* pSpawn;
	ACE_NEW_NORETURN(pSpawn,SpawnedProcess);
	if (!pSpawn)
	{
		response.bFailure = 1;
		response.err = E_OUTOFMEMORY;
		return;
	}

	// Open an acceptor
	ACE_INET_Addr addr((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Acceptor acceptor;
	int ret = acceptor.open(addr,0,PF_INET,1);
	if (ret != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")));
	}
	else
	{
		// Get the port we are accepting on
		if (acceptor.get_local_addr(addr)!=0)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("get_local_addr() failed")));
		}
		else
		{
			// Spawn the user process
			ret = pSpawn->Spawn(request.uid,addr.get_port_number());
			if (ret != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("spawn() failed")));
			}
			else
			{
				// Accept a socket
				ACE_SOCK_Stream stream;
				ACE_Time_Value wait(5);
				ret = acceptor.accept(stream,0,&wait);
				if (ret != 0)
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("accept() failed")));
				}
				else
				{
					// Read the port the user session is listening on...
					if (stream.recv(&response.uNewPort,sizeof(response.uNewPort),&wait) < sizeof(response.uNewPort))
					{
						ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("recv() failed")));
						ret = -1;
					}
					else
					{
						ret = bootstrap_client(stream,request.uid == static_cast<Session::TOKEN>(-1));
					}

					if (ret == 0)
					{
						// Create a new RootConnection
						RootConnection* pRC = 0;
						ACE_NEW_NORETURN(pRC,RootConnection(this,strUserId));
						if (!pRC)
						{
							ret = -1;
						}
						else if ((ret=pRC->open(stream.get_handle())) == 0)
						{
							// Insert the data into various maps...
							try
							{
								UserProcess process = {response.uNewPort, pSpawn};
								m_mapUserProcesses.insert(std::map<ACE_CString,UserProcess>::value_type(strUserId,process));
								m_mapUserIds.insert(std::map<ACE_HANDLE,ACE_CString>::value_type(stream.get_handle(),strUserId));

								// Create a new unique channel id
								ChannelPair channel = {stream.get_handle(), 0};
								ACE_CDR::UShort uChannelId = m_uNextChannelId++;
								while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
								{
									uChannelId = m_uNextChannelId++;
								}
								m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(uChannelId,channel));
								m_mapReverseChannelIds.insert(std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(stream.get_handle(),std::map<ACE_CDR::UShort,ACE_CDR::UShort>()));
								
								// Clear the handle in the stream, pRC now owns it
								stream.set_handle(ACE_INVALID_HANDLE);
							}
							catch (...)
							{
								delete pRC;
								ret = -1;
							}
						}
						else
						{
							delete pRC;
						}
					}

					stream.close();
				}

				if (ret != 0)
					pSpawn->Close();
			}
		}

		acceptor.close();
	}

	if (ret != 0)
	{
		delete pSpawn;
		response.bFailure = 1;
		response.err = ret;
	}
	else
	{
		response.bFailure = 0;
	}
}

int RootManager::bootstrap_client(ACE_SOCK_STREAM& stream, bool bSandbox)
{
	// This could be changed to a struct if we wanted...
	char sandbox = bSandbox ? 1 : 0;

	if (stream.send(&sandbox,sizeof(sandbox)) != sizeof(sandbox))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send() failed")),-1);
						
	return 0;
}

void RootManager::connect_client(const Session::Request& request, Session::Response& response)
{
	return ROOT_MANAGER::instance()->connect_client_i(request,response);
}

void RootManager::connect_client_i(const Session::Request& request, Session::Response& response)
{
	// Set the response size
	response.cbSize = sizeof(response);
	response.bFailure = 0;

	ACE_CString strUserId;
	int err = SpawnedProcess::ResolveTokenToUid(request.uid,strUserId);
	if (err != 0)
	{
		response.bFailure = 1;
		response.err = err;
		return;
	}

	// Lock the cs
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,
		response.bFailure = 1;
		response.err = ACE_OS::last_error();
		return;
	);

	// See if we have a process already
	try
	{
		// See if we have a sandbox yet...
		if (m_mapUserProcesses.empty())
		{
			// Create a new sandbox...
			response.err = spawn_sandbox();
			if (response.err != 0)
				response.bFailure = 1;
		}

		if (response.bFailure == 0)
		{
			std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.find(strUserId);
			if (i!=m_mapUserProcesses.end())
			{
				// See if its still running...
				if (!i->second.pSpawn->IsRunning())
				{
					// No, close it and remove from map
					i->second.pSpawn->Close();
					delete i->second.pSpawn;
					m_mapUserProcesses.erase(i);
					i = m_mapUserProcesses.end();
				}
			}

			if (i==m_mapUserProcesses.end())
			{
				// No we don't
				spawn_client(request,response,strUserId);
			}
			else
			{
				// Yes we do
				response.bFailure = 0;
				response.uNewPort = i->second.uPort;
			}
		}
	}
	catch (...)
	{
		response.bFailure = 1;
		response.err = EINVAL;
		return;
	}
}

ACE_THR_FUNC_RETURN RootManager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

void RootManager::root_connection_closed(const ACE_CString& strUserId, ACE_HANDLE handle)
{
	ACE_OS::closesocket(handle);

	ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<ACE_CString,UserProcess>::iterator i=m_mapUserProcesses.find(strUserId);
		if (i != m_mapUserProcesses.end())
		{
			i->second.pSpawn->Close();
			delete i->second.pSpawn;
			m_mapUserProcesses.erase(i);
		}

		m_mapUserIds.erase(handle);

		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(handle);
		if (j!=m_mapReverseChannelIds.end())
		{
			for (std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k=j->second.begin();k!=j->second.end();++k)
			{
				m_mapChannelIds.erase(k->second);
			}
			m_mapReverseChannelIds.erase(j);
		}
	}
	catch (...)
	{}
}

int RootManager::enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	RequestBase* req;
	ACE_NEW_RETURN(req,RequestBase(handle,input),-1);

	int ret = enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

ACE_THR_FUNC_RETURN RootManager::request_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ROOT_MANAGER::instance()->pump_requests();
}

void RootManager::forward_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Forward to the correct channel...
	ChannelPair dest_channel;
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("Invalid destination channel id.\n")));
			return;
		}
		dest_channel = i->second;
		
		// Find the local channel id that matches src_channel_id
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(request->handle());
		if (j==m_mapReverseChannelIds.end())
			return; 

		std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(src_channel_id);
		if (k == j->second.end())
		{
			ChannelPair channel = {request->handle(), src_channel_id};
			reply_channel_id = m_uNextChannelId++;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = m_uNextChannelId++;
			}
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(reply_channel_id,channel));

			k = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(src_channel_id,reply_channel_id)).first;
		}
		reply_channel_id = k->second;
	}
	catch (...)
	{
		return;
	}

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("Root context: Forwarding request from %u(%u) to %u(%u)"),reply_channel_id,src_channel_id,dest_channel_id,dest_channel.channel));

	if (trans_id == 0)
	{
		send_asynch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),request_deadline);
	}
	else
	{
		RequestBase* response;
		if (send_synch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),response,request_deadline) == 0)
		{
			send_response(request->handle(),src_channel_id,trans_id,response->input()->start(),request_deadline);
			delete response;
		}
	}
}

void RootManager::process_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Get the corresponding UserId
	/*ACE_CString strRealUserId(strUserId);
	if (strRealUserId.is_empty())
	{
		try
		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

			std::map<ACE_HANDLE,ACE_CString>::iterator i=m_mapUserIds.find(request->handle());
			if (i == m_mapUserIds.end())
				return;
			
			strRealUserId = i->second;
		}
		catch (...)
		{
			return;
		}
	}*/

	if (dest_channel_id == 0)
	{
		/*if (request has come from a network driver)
		{
			// Force on to sand box
			forward_request(request,1,src_channel,trans_id,request_deadline);
		}
		else*/ 
		{
			process_root_request(request,src_channel_id,trans_id,request_deadline);
		}
	}
	else
	{
		forward_request(request,dest_channel_id,src_channel_id,trans_id,request_deadline);
	}

	delete request;
}

int RootManager::access_check(ACE_HANDLE handle, const char* pszObject, ACE_UINT32 mode, bool& bAllowed)
{
	try
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,-1);

		// Find the user id
		std::map<ACE_HANDLE,ACE_CString>::iterator i = m_mapUserIds.find(handle);
		if (i == m_mapUserIds.end())
		{
			ACE_OS::last_error(EINVAL);
			return -1;
		}

		// Find the process info associated with user id
		std::map<ACE_CString,UserProcess>::iterator j = m_mapUserProcesses.find(i->second);
		if (j == m_mapUserProcesses.end())
		{
			ACE_OS::last_error(EINVAL);
			return -1;
		}

		return (j->second.pSpawn->CheckAccess(pszObject,mode,bAllowed) ? 0 : -1);
	}
	catch (...)
	{
		ACE_OS::last_error(EINVAL);
		return -1;
	}
}

void RootManager::process_root_request(RequestBase* request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

		// Find the local channel id that matches src_channel_id
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(request->handle());
		if (j==m_mapReverseChannelIds.end())
			return; 

		std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(src_channel_id);
		if (k == j->second.end())
		{
			ChannelPair channel = {request->handle(), src_channel_id};
			reply_channel_id = m_uNextChannelId++;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = m_uNextChannelId++;
			}
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(reply_channel_id,channel));

			k = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(src_channel_id,reply_channel_id)).first;
		}
		reply_channel_id = k->second;
	}
	catch (...)
	{
		return;
	}

	OOServer::RootOpCode_t op_code;
	(*request->input()) >> op_code;

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("Root context: Root request %u from %u(%u)"),op_code,reply_channel_id,src_channel_id));

	if (!request->input()->good_bit())
		return;

	ACE_OutputCDR response;
	
	switch (op_code)
	{
<<<<<<< .mine
	case OOServer::KeyExists:
		registry_key_exists(request,response);
=======
	case OOServer::Open:
		{
			ACE_CString strIn;
			request->input()->read_string(strIn);

			//int err = m_registry.open_section(m_registry.root_section(),ACE_TEXT_CHAR_TO_TCHAR(
			
			response.write_string(strIn);
			send_response(request->handle(),0,trans_id,response.begin(),request_deadline);
		}
>>>>>>> .r249
		break;

	case OOServer::CreateKey:
		registry_create_key(request,response);												
		break;

	case OOServer::DeleteKey:
		registry_delete_key(request,response);
		break;

	case OOServer::EnumSubKeys:
		registry_enum_subkeys(request,response);
		break;

	case OOServer::ValueType:
		registry_value_type(request,response);
		break;

	case OOServer::GetStringValue:
		registry_get_string_value(request,response);
		break;

	case OOServer::GetUInt32Value:
		registry_get_uint_value(request,response);
		break;

	case OOServer::SetStringValue:
		registry_set_string_value(request,response);
		break;

	case OOServer::SetUInt32Value:
		registry_set_uint_value(request,response);
		break;

	case OOServer::EnumValues:
		registry_enum_values(request,response);
		break;

	case OOServer::DeleteValue:
		registry_delete_value(request,response);
		break;

	default:
		response.write_long(EINVAL);
		break;
	}

	if (response.good_bit())
		send_response(request->handle(),0,trans_id,response.begin(),request_deadline);
}

bool RootManager::registry_open_section(RequestBase* request, ACE_Configuration_Section_Key& key, bool bAccessCheck)
{
	ACE_CString strKey;
	if (!request->input()->read_string(strKey))
		return false;

	if (bAccessCheck)
	{
		bool bAllowed = false;
		if (strKey.substr(0,9) == "All Users")
			bAllowed = true;
		else if (access_check(request->handle(),m_strRegistry.c_str(),O_RDWR,bAllowed) != 0)
			return false;
		
		if (!bAllowed)
		{
			ACE_OS::last_error(EACCES);
			return false;
		}
	}
	
	if (strKey.length()==0)
		key = m_registry.root_section();
	else if (m_registry.open_section(m_registry.root_section(),ACE_TEXT_CHAR_TO_TCHAR(strKey).c_str(),0,key) != 0)
		return false;
	
	return true;
}

bool RootManager::registry_open_value(RequestBase* request, ACE_Configuration_Section_Key& key, ACE_CString& strValue, bool bAccessCheck)
{
	if (!registry_open_section(request,key,bAccessCheck))
		return false;

	if (!request->input()->read_string(strValue))
		return false;
	
	return true;
}

void RootManager::registry_key_exists(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_CDR::Boolean bRes = false;

	ACE_Configuration_Section_Key key;
	if (!registry_open_section(request,key))
	{
		if (ACE_OS::last_error() != ENOENT)
			err = ACE_OS::last_error();
	}
	else
	{
		bRes = true;
	}

	response.write_long(err);
	if (err == 0)
		response.write_boolean(bRes);
}

void RootManager::registry_create_key(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_CString strKey;
	if (!request->input()->read_string(strKey))
		err = ACE_OS::last_error();
	else
	{
        bool bAllowed = false;
		if (access_check(request->handle(),m_strRegistry.c_str(),O_RDWR,bAllowed) != 0)
			err = ACE_OS::last_error();
		else if (!bAllowed)
			err = EACCES;
		else
		{
			ACE_Configuration_Section_Key key;
			if (m_registry.open_section(m_registry.root_section(),ACE_TEXT_CHAR_TO_TCHAR(strKey).c_str(),1,key) != 0)
				err = ACE_OS::last_error();
		}
	}

	response.write_long(err);
}

void RootManager::registry_delete_key(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_Configuration_Section_Key key;
	if (!registry_open_section(request,key,true))
		err = ACE_OS::last_error();
	else
	{
		ACE_CString strSubKey;

		if (!request->input()->read_string(strSubKey))
			err = ACE_OS::last_error();
		else
		{
			if (m_registry.remove_section(key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey).c_str(),1) != 0)
				err = ACE_OS::last_error();
		}
	}
	
	response.write_long(err);
}

void RootManager::registry_enum_subkeys(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	std::list<ACE_TString> listSections;
	ACE_Configuration_Section_Key key;
	if (!registry_open_section(request,key))
		err = ACE_OS::last_error();
	else
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
	
	response.write_long(err);
	if (err == 0)
	{
		response.write_ulonglong(listSections.size());
		for (std::list<ACE_TString>::iterator i=listSections.begin();i!=listSections.end();++i)
		{
			response.write_string(ACE_TEXT_ALWAYS_CHAR(*i));
		}
	}
}

void RootManager::registry_value_type(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_CDR::Octet type = 0;

	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue))
		err = ACE_OS::last_error();
	else
	{
		ACE_Configuration_Heap::VALUETYPE vtype;
		if (m_registry.find_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),vtype) == 0)
			type = static_cast<ACE_CDR::Octet>(vtype);
		else
			err = ACE_OS::last_error();
	}
		
	response.write_long(err);
	if (err == 0)
		response.write_octet(type);
}

void RootManager::registry_get_string_value(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_CString strText;

	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue))
		err = ACE_OS::last_error();
	else
	{
		ACE_TString strTextT;
		if (m_registry.get_string_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),strTextT) != 0)
			err = ACE_OS::last_error();
		else
			strText = ACE_TEXT_ALWAYS_CHAR(strTextT);
	}

	response.write_long(err);
	if (err == 0)
		response.write_string(strText);
}

void RootManager::registry_get_uint_value(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	ACE_CDR::ULong val = 0;

	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue))
		err = ACE_OS::last_error();
	else
	{
		u_int v = 0;
		if (m_registry.get_integer_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),val) != 0)
			err = ACE_OS::last_error();
		else
			val = v;
	}

	response.write_long(err);
	if (err == 0)
		response.write_ulong(val);
}

//virtual void GetBinaryValue(const string_t& name, uint32_t& cbLen, byte_t* pBuffer) = 0;

void RootManager::registry_set_string_value(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	
	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue,true))
		err = ACE_OS::last_error();
	else
	{
		ACE_CString strText;
		if (!request->input()->read_string(strText))
			err = ACE_OS::last_error();
		else if (m_registry.set_string_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),ACE_TEXT_CHAR_TO_TCHAR(strText)) != 0)
			err = ACE_OS::last_error();
	}

	response.write_long(err);
}

void RootManager::registry_set_uint_value(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	
	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue,true))
		err = ACE_OS::last_error();
	else
	{
		ACE_CDR::ULong iValue;
		if (!request->input()->read_ulong(iValue))
			err = ACE_OS::last_error();
		else if (m_registry.set_integer_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str(),iValue) != 0)
			err = ACE_OS::last_error();
	}

	response.write_long(err);
}

//virtual void SetBinaryValue(const string_t& name, uint32_t cbLen, const byte_t* val) = 0;

void RootManager::registry_enum_values(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;
	std::list<ACE_TString> listValues;	
	ACE_Configuration_Section_Key key;
	if (!registry_open_section(request,key))
		err = ACE_OS::last_error();
	else
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
	
	response.write_long(err);
	if (err == 0)
	{
		response.write_ulonglong(listValues.size());
		for (std::list<ACE_TString>::iterator i=listValues.begin();i!=listValues.end();++i)
		{
			response.write_string(ACE_TEXT_ALWAYS_CHAR(*i));
		}
	}
}

void RootManager::registry_delete_value(RequestBase* request, ACE_OutputCDR& response)
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_registry_lock);

	ACE_CDR::Long err = 0;

	ACE_Configuration_Section_Key key;
	ACE_CString strValue;
	if (!registry_open_value(request,key,strValue,true))
		err = ACE_OS::last_error();
	else if (m_registry.remove_value(key,ACE_TEXT_CHAR_TO_TCHAR(strValue).c_str()) != 0)
		err = ACE_OS::last_error();
	
	response.write_long(err);
}
