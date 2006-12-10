#include "OOServer.h"
#include "./UserSession.h"
#include "../OOCore/UserService.h"

static ACE_THR_FUNC_RETURN worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

static int ConnectToRoot(u_short uPort)
{
	ACE_Connector<UserSession, ACE_SOCK_CONNECTOR>	connector;

	UserSession* pSession = 0;
	ACE_INET_Addr addr(uPort,ACE_LOCALHOST);
	int ret = 0;
	if ((ret = connector.connect(pSession,addr)) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("connect() failed")));
	}
	else
	{
		ACE_INET_Addr port_addr((u_short)0);
		Session::Acceptor<OTL::ObjectImpl<UserService>, ACE_SOCK_Acceptor> acceptor;
		if ((ret = acceptor.open(port_addr)) == -1)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("acceptor::open() failed")));
		}
		else
		{
			if ((ret = acceptor.acceptor().get_local_addr(port_addr))==-1)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
			}
			else
			{
				uPort = port_addr.get_port_number();
				if (pSession->peer().send(&uPort,sizeof(uPort)) < sizeof(uPort))
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")));
					ret = -1;
				}
			}

			acceptor.close();
		}

		pSession->close();
	}

	return ret;
}

int UserMain(u_short uPort)
{
	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	printf("SPAWNED!!!");

#if defined(ACE_WIN32)

	//::DebugBreak();

#endif

	int ret = ConnectToRoot(uPort);
	if (ret == 0)
	{
		// Spawn off the engine threads
		int thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads-1,worker_fn);
		if (thrd_grp_id == -1)
			ret = -1;

		if (ret==0)
		{
			// Treat this thread as a worker as well
			ret = worker_fn(0);

			// Wait for all the threads to finish
			ACE_Thread_Manager::instance()->wait_grp(thrd_grp_id);
		}
	}
	
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

int UserSession::open(void* p)
{
	int ret = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::open(p);
	if (ret != 0)
		return ret;


	return 0;
}

int UserSession::handle_input(ACE_HANDLE)
{
	// Recv the request size
/*	UserManager::Request request = {0};
	if (peer().recv(&request.cbSize, sizeof(request.cbSize)) < sizeof(request.cbSize))
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connection closed\n")));
		return -1;
	}

	// Check the request size
	if (request.cbSize < sizeof(request))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid request\n")),-1);

	// Read the reest of the request
	if (peer().recv(&request.uid, request.cbSize - sizeof(request.cbSize)) < static_cast<ssize_t>(request.cbSize - sizeof(request.cbSize)))
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connection closed\n")));
		return -1;
	}
	
	// Ask the client manager for a response...
	UserManager::Response response = {0};
	m_pManager->connect_client(request,response);
	
	// Try to send the response
	ssize_t send_cnt = peer().send(&response,response.cbSize);
	if (send_cnt == static_cast<ssize_t>(response.cbSize))
		return -1; // All done

	// Check for errors
	if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")),-1);

	// If we are blocking, queue up the rest of the response, and send later...
	if (send_cnt == -1)
		send_cnt = 0;
	size_t remaining = static_cast<size_t>(response.cbSize - send_cnt);

	ACE_Message_Block* mb = 0;
	ACE_NEW_RETURN(mb,ACE_Message_Block(&(reinterpret_cast<const char*>(&response)[send_cnt]),remaining),-1);
	
	bool output_off = msg_queue()->is_empty();
	if (putq(mb) == -1)
	{
		mb->release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p; discarding data\n"),ACE_TEXT("putq() failed")),-1);
	}

	if (output_off)
		return reactor()->register_handler(this,ACE_Event_Handler::WRITE_MASK);
	else
		return -1; // All done*/

	return 0;
}

int UserSession::handle_output(ACE_HANDLE)
{
	ACE_Message_Block* mb = 0;
	ACE_Time_Value nowait(ACE_OS::gettimeofday());
	while (getq(mb, &nowait) == 0)
	{
		ssize_t send_cnt = peer().send(mb->rd_ptr(),mb->length());
		if (send_cnt == -1)
			ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) %p\n"),ACE_TEXT("send")));
		else
			mb->rd_ptr(static_cast<size_t>(send_cnt));

		if (mb->length() > 0)
		{
			ungetq(mb);
			break;
		}
		mb->release();
	}

	return (msg_queue()->is_empty() ? -1 : 0);
}

int UserSession::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
	return ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::handle_close(fd,mask);
}
