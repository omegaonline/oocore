#include "OOServer.h"
#include "./UserSession.h"
#include "../OOCore/UserService.h"

int StartReactor()
{
	// Create the correct reactor impl
	ACE_Reactor_Impl* reactor_impl;
#ifdef ACE_WIN32
	ACE_NEW_RETURN(reactor_impl,ACE_WFMO_Reactor,-1);
#else
	ACE_NEW_RETURN(reactor_impl,ACE_TP_Reactor,-1);
#endif // !ACE_WIN32
	ACE_Auto_Ptr<ACE_Reactor_Impl> ptrReactor(reactor_impl);

	ACE_Reactor* reactor;
	ACE_NEW_RETURN(reactor,ACE_Reactor(reactor_impl,1),-1);
	ACE_Reactor::instance(reactor,1);
	
	ptrReactor.release();
	return 0;
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

	// Start the reactor
	int ret = 0;
	if ((ret = StartReactor()) == 0)
	{
		ACE_Connector<UserSession, ACE_SOCK_CONNECTOR>	connector;

		UserSession* pSession = 0;
		ACE_INET_Addr addr(uPort,ACE_LOCALHOST);
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
					u_short uPort = port_addr.get_port_number();
					if ((ret = pSession->peer().send(&uPort,sizeof(uPort))) < sizeof(uPort))
					{
						ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")));
					}
					else
					{
						ret = ACE_Reactor::instance()->run_reactor_event_loop();
					}
				}

				acceptor.close();
			}

			pSession->close();
		}
	}

#ifdef _DEBUG
	// Give us a chance to read the error message
	if (ret != 0)
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
