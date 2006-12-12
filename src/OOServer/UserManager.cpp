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

UserManager::UserManager() : 
	LocalAcceptor<UserConnection>(),
	m_root_handle(ACE_INVALID_HANDLE)
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

		// Stop the message queue
		m_msg_queue.close();
		
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
						m_root_handle = stream.get_handle();
						ACE_Message_Block mb;
						pRC->open(m_root_handle,mb);
						
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

void UserManager::connection_closed(SpawnedProcess::USERID key)
{
	// We only have one connection to the root, and when it closes, we are finished!
	
	//cancel();

	//if (get_handle() != ACE_INVALID_HANDLE)
	//	ACE_OS::closesocket(get_handle());

	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

ACE_THR_FUNC_RETURN UserManager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

void UserManager::enque_request(ACE_Message_Block& mb, ACE_HANDLE handle)
{
	ACE_Message_Block* mb_new = mb.duplicate();
	if (mb_new)
	{
		// Swap the length for the handle value...
		RootProtocol::Header* pHeader = reinterpret_cast<RootProtocol::Header*>(mb_new->rd_ptr());

		// Set the handle value to ACE_INVALID_HANDLE, marking this as the root connection...
		pHeader->handle = handle;

		if (m_msg_queue.enqueue_prio(mb.duplicate()) != 0)
			mb_new->release();
	}
}

ACE_THR_FUNC_RETURN UserManager::request_worker_fn(void*)
{
	return USER_MANAGER::instance()->process_requests();
}

ACE_THR_FUNC_RETURN UserManager::process_requests()
{
	for (;;)
	{
		// Get the next message
		ACE_Message_Block* mb;
		int ret = m_msg_queue.dequeue_prio(mb);
		if (ret < 0)
			return (ACE_THR_FUNC_RETURN)ret;
	
		// Get the header, and move on the rd_ptr
		RootProtocol::Header* pHeader = reinterpret_cast<RootProtocol::Header*>(mb->rd_ptr());
		
		if (pHeader->handle == m_root_handle)
		{
			// We have a root connection message...
			mb->rd_ptr(sizeof(RootProtocol::Header));
			
			// Do something with mb...
			ret = -1;
			switch (pHeader->op)
			{
			default:
				void* TODO;
			}
		}
		else
		{
			// We have a user connection message...

		}

		if (ret != 0)
		{
			// Something fishy from downstream, close socket!
			ACE_OS::closesocket(pHeader->handle);
		}
		
		mb->release();
	}
}
