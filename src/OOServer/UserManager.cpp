#include "OOServer.h"

#include "./UserManager.h"

int UserMain(u_short uPort)
{
#if defined(ACE_WIN32)

	//::DebugBreak();
	printf("SPAWNED!!!\n\n");

#endif

	int ret = UserManager::run_event_loop(uPort);
	
#ifdef _DEBUG
	// Give us a chance to read the error message
	if (ret < 0)
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("\nTerminating OOServer user process: exitcode = %d, error = %m.\n\n" \
									 "OOServer will now wait for 10 seconds so you can read this message...\n"),ret));
		ACE_OS::sleep(10);
	}
#endif

	return ret;
}

UserManager::UserManager() : 
	LocalAcceptor<UserConnection>(),
	m_root_handle(ACE_INVALID_HANDLE),
	m_uNextChannelId(1)
{
}

UserManager::~UserManager()
{
	term();
}

int UserManager::run_event_loop(u_short uPort)
{
	return USER_MANAGER::instance()->run_event_loop_i(uPort);
}

int UserManager::run_event_loop_i(u_short uPort)
{
	int ret = init(uPort);
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
			// Run out bootstrap functions
			ret = boostrap();
			if (ret != 0)
				ACE_OS::shutdown(m_root_handle,SD_BOTH);
			else
				ret = proactor_worker_fn(0);

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop handling requests
		RequestHandler<UserRequest>::stop();
		
		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	return ret;
}

int UserManager::init(u_short uPort)
{
	ACE_SOCK_Connector connector;
	ACE_INET_Addr addr(uPort,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Stream stream;
	ACE_Time_Value wait(5);

	// Connect to the root
	int ret = connector.connect(stream,addr,&wait);
	if (ret != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("connect() failed")));
	}
	else
	{
		// Bind a tcp socket 
		ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
		if ((ret = open(sa)) != 0)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("acceptor::open() failed")));
		}
		else
		{
			// Get our port number
			int len = sa.get_size ();
			sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
			if ((ret = ACE_OS::getsockname(this->get_handle(),addr,&len)) == -1)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
			}
			else
			{
				sa.set_type(addr->sa_family);
				sa.set_size(len);

				uPort = sa.get_port_number();
				if (stream.send(&uPort,sizeof(uPort),&wait) < sizeof(uPort))
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")));
					ret = -1;
				}
				else
				{
					// Create a new RootConnection
					RootConnection*	pRC;
					ACE_NEW_NORETURN(pRC,RootConnection(this,ACE_CString()));
					if (!pRC)
					{
						ret = -1;
					}
					else if ((ret=pRC->open(stream.get_handle())) == 0)
					{
						// Stash the root handle
						m_root_handle = stream.get_handle();

						// Clear the handle in the stream, pRC now owns it
						stream.set_handle(ACE_INVALID_HANDLE);
					}
				}
			}
		}

		stream.close();
	}

	return ret;
}

void UserManager::term()
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

	if (m_root_handle != ACE_INVALID_HANDLE)
	{
		ACE_OS::closesocket(m_root_handle);
		m_root_handle = ACE_INVALID_HANDLE;
	}
}

int UserManager::boostrap()
{
	// Send a test message
	ACE_OutputCDR request;
	request.write_ulong(1);
	request.write_string("Hello!");

	ACE_Time_Value wait(60);
	UserRequest* response;	
	int ret = send_synch(m_root_handle,0,request,response,&wait);
	if (ret == 0)
	{
		ACE_CString strResponse;
		if (response->input()->read_string(strResponse))
		{
			printf("Response: %s\n",strResponse.c_str());
		}
	}

	return ret;
}

void UserManager::root_connection_closed(const ACE_CString& /*key*/, ACE_HANDLE /*handle*/)
{
	try
	{
		// Stop accepting
		ACE_OS::closesocket(handle());

		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);
			// Shutdown all client handles
			for (std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.begin();j!=m_mapReverseChannelIds.end();)
			{
				if (j->first == m_root_handle)
				{
					m_mapReverseChannelIds.erase(j++);
				}
				else
				{
					ACE_OS::shutdown(j->first,SD_SEND);
					++j;
				}
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

	// We close when the root connection closes
	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

ACE_THR_FUNC_RETURN UserManager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

int UserManager::validate_connection(const ACE_Asynch_Accept::Result& result, const ACE_INET_Addr& remote, const ACE_INET_Addr& local)
{
	// Check we can accept it...
	if (LocalAcceptor<UserConnection>::validate_connection(result,remote,local) != 0)
		return -1;

	try
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,-1);

		Channel channel = {result.accept_handle(), 0};
		ACE_CDR::UShort uChannelId = m_uNextChannelId++;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = m_uNextChannelId++;
		}
		m_mapChannelIds.insert(std::map<ACE_CDR::UShort,Channel>::value_type(uChannelId,channel));
		m_mapReverseChannelIds.insert(std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(result.accept_handle(),std::map<ACE_CDR::UShort,ACE_CDR::UShort>()));
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}

int UserManager::enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	UserRequest* req;
	ACE_NEW_RETURN(req,UserRequest(handle,input),-1);

	req->m_bRoot = true;
	
	int ret = enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

int UserManager::enqueue_user_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	UserRequest* req;
	ACE_NEW_RETURN(req,UserRequest(handle,input),-1);

	req->m_bRoot = false;
	
	int ret = USER_MANAGER::instance()->enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

void UserManager::user_connection_closed(ACE_HANDLE handle)
{
	USER_MANAGER::instance()->user_connection_closed_i(handle);
}

void UserManager::user_connection_closed_i(ACE_HANDLE handle)
{
	ACE_OS::closesocket(handle);

	ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

	try
	{
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

ACE_THR_FUNC_RETURN UserManager::request_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)USER_MANAGER::instance()->pump_requests();
}

void UserManager::process_request(UserRequest* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	if (dest_channel_id == 0)
	{
		if (request->m_bRoot)
			process_root_request(request->handle(),*request->input(),trans_id,request_deadline);
		else 
			process_request(request->handle(),*request->input(),trans_id,request_deadline);
	}
	else
	{
		// Forward to the correct channel...
		forward_request(request,strUserId,dest_channel_id,src_channel_id,trans_id,request_deadline);
	}

	delete request;
}

void UserManager::process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ACE_CDR::ULong op_code;
	request >> op_code;
	if (!request.good_bit())
		return;

	switch (op_code)
	{
	default:
		;
	}
}

void UserManager::process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Create an IChannel and farm it off to our local StdObjectManager!!
	try
	{
        // Ooooh Omega::functionality!
	}
	catch (Omega::IException* pE)
	{
	}
	catch (...)
	{
	}
}

void UserManager::forward_request(UserRequest* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	Channel dest_channel;
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

		// Find the destination channel
		std::map<ACE_CDR::UShort,Channel>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return;
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
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
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
		RequestHandler<UserRequest>::send_asynch(dest_channel.handle,strUserId,dest_channel.channel,reply_channel_id,request->input()->start(),request_deadline);
	}
	else
	{
		UserRequest* response;
		if (RequestHandler<UserRequest>::send_synch(dest_channel.handle,strUserId,dest_channel.channel,reply_channel_id,request->input()->start(),response,request_deadline) == 0)
		{
			send_response(request->handle(),src_channel_id,trans_id,response->input()->start(),request_deadline);
			delete response;
		}
	}
}

int UserManager::send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_OutputCDR& request, ACE_Time_Value* wait)
{
	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	return RequestHandler<UserRequest>::send_asynch(handle,"",dest_channel_id,0,request.begin(),&deadline);
}

int UserManager::send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_OutputCDR& request, UserRequest*& response, ACE_Time_Value* wait)
{
	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	return RequestHandler<UserRequest>::send_synch(handle,"",dest_channel_id,0,request.begin(),response,&deadline);
}
