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

#include <OOCore/Preprocessor/base.h>

RootManager::RootManager() : 
	LocalAcceptor<ClientConnection>(),
	m_config_file(ACE_INVALID_HANDLE),
	m_uNextChannelId(2)
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
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
			}
		}
		ACE_OS::close(m_config_file);
	}

	m_config_file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_WRONLY | O_CREAT | O_TRUNC | O_TEMPORARY);
	if (m_config_file == INVALID_HANDLE_VALUE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")),-1);

	// Write our pid instead
	pid_t pid = ACE_OS::getpid();
	if (ACE_OS::write(m_config_file,&pid,sizeof(pid)) != sizeof(pid))
	{
		ACE_OS::close(m_config_file);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")),-1);
	}

	// Bind a tcp socket 
	ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	if (open(sa) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")));
		ACE_OS::close(m_config_file);
		return -1;
	}
	
	// Get our port number
	int len = sa.get_size ();
	sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
	if (ACE_OS::getsockname(this->get_handle(),addr,&len) == -1)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
		ACE_OS::close(m_config_file);
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
		return -1;	
	}

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening for client connections on port %u.\n"),uPort));
	
	return 0;
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
		ACE_OS::closesocket(handle());

		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);
	
			// Shutdown all client handles
			for (std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.begin();j!=m_mapReverseChannelIds.end();++j)
			{
				ACE_OS::shutdown(j->first,SD_SEND);
			}
		}

        // Give everyone a chance to shut down
		for (;;)
		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);
			if (m_mapReverseChannelIds.empty())
				break;
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
			i->second.pSpawn->Close();
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
								Channel channel = {stream.get_handle(), 0};
								ACE_CDR::UShort uChannelId = m_uNextChannelId++;
								while (uChannelId < 2 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
								{
									uChannelId = m_uNextChannelId++;
								}
								m_mapChannelIds.insert(std::map<ACE_CDR::UShort,Channel>::value_type(uChannelId,channel));
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

void RootManager::connect_client(const Session::Request& request, Session::Response& response)
{
	return ROOT_MANAGER::instance()->connect_client_i(request,response);
}

void RootManager::connect_client_i(const Session::Request& request, Session::Response& response)
{
	// Set the response size
	response.cbSize = sizeof(response);

	ACE_CString strUserId;
	int err = SpawnedProcess::ResolveTokenToUid(request.uid,strUserId);
	if (err != 0)
	{
		response.bFailure = 1;
		response.err = err;
		return;
	}

	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,
		response.bFailure = 1;
		response.err = ACE_OS::last_error();
		return;
	);

	// See if we have a process already
	try
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

void RootManager::forward_request(RequestBase* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Forward to the correct channel...
	Channel dest_channel;
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::UShort,Channel>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
		{
			if (dest_channel_id != 1)
				return;
			
			void* TODO; // Spawn sandbox!
		}
		dest_channel = i->second;
		
		// Find the local channel id that matches src_channel_id
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(request->handle());
		if (j==m_mapReverseChannelIds.end())
			return; 

		std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(src_channel_id);
		if (k == j->second.end())
		{
			Channel channel = {request->handle(), src_channel_id};
			reply_channel_id = m_uNextChannelId++;
			while (reply_channel_id<2 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = m_uNextChannelId++;
			}
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,Channel>::value_type(reply_channel_id,channel));

			k = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(src_channel_id,reply_channel_id)).first;
		}
		reply_channel_id = k->second;
	}
	catch (...)
	{
		return;
	}

	if (trans_id == 0)
	{
		send_asynch(dest_channel.handle,strUserId,dest_channel.channel,reply_channel_id,request->input()->start(),request_deadline);
	}
	else
	{
		RequestBase* response;
		if (send_synch(dest_channel.handle,strUserId,dest_channel.channel,reply_channel_id,request->input()->start(),response,request_deadline) == 0)
		{
			send_response(request->handle(),src_channel_id,trans_id,response->input()->start(),request_deadline);
			delete response;
		}
	}
}

void RootManager::process_request(RequestBase* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Get the corresponding UserId
	ACE_CString strRealUserId(strUserId);
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
	}

	if (dest_channel_id == 0)
	{
		/*if (request has come from a network driver)
		{
			// Force on to sand box
			forward_request(request,strUserId,1,src_channel,trans_id,request_deadline);
		}
		else*/ 
		{
			process_root_request(request,trans_id,request_deadline);
		}
	}
	else
	{
		forward_request(request,strRealUserId,dest_channel_id,src_channel_id,trans_id,request_deadline);
	}

	delete request;
}

void RootManager::process_root_request(RequestBase* request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ACE_CDR::ULong op_code;
	(*request->input()) >> op_code;
	if (!request->input()->good_bit())
		return;

	ACE_OutputCDR response;

	switch (op_code)
	{
	case 1:
		{
			ACE_CString strIn;
			request->input()->read_string(strIn);
			response.write_string(strIn);
			send_response(request->handle(),0,trans_id,response.begin(),request_deadline);
		}
		break;

	default:
		;
	}
}
