#include "OOServer.h"
#include "./UserManager.h"

int UserMain(u_short uPort)
{
#if defined(ACE_WIN32)

	//::DebugBreak();
	printf("SPAWNED!!!");

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

UserManager::UserManager() : LocalAcceptor<UserConnection>()
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
			// Treat this thread as a worker as well
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
					ACE_NEW_NORETURN(pRC,RootConnection(this,SpawnedProcess::USERID()));
					if (!pRC)
						ret = -1;
					else
					{
						ACE_Message_Block mb;
						pRC->open(stream.get_handle(),mb);
						
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
	
}

void UserManager::root_connection_closed(const SpawnedProcess::USERID& /*key*/, ACE_HANDLE /*handle*/)
{
	// We close when the root connection closes
	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

ACE_THR_FUNC_RETURN UserManager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

int UserManager::enque_root_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	UserRequest* req;
	ACE_NEW_RETURN(req,UserRequest(handle,input),-1);

	req->m_bRoot = true;
	
	int ret = enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

ACE_THR_FUNC_RETURN UserManager::request_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)USER_MANAGER::instance()->pump_requests();
}

void UserManager::process_request(UserRequest* request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	if (request->m_bRoot)
		process_root_request(request->handle(),*request->input(),trans_id,request_deadline);
	else
		process_request(request->handle(),*request->input(),trans_id,request_deadline);

	delete request;
}

void UserManager::process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// The root service really only notifies us of things...
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
