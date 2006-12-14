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

void UserManager::root_connection_closed(SpawnedProcess::USERID key)
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
	Request* req;
	ACE_NEW_RETURN(req,Request,-1);

	req->m_bRoot = true;
	req->m_handle = handle;
	req->m_input = input;

	int ret = m_msg_queue.enqueue_prio(req);
	if (ret <= 0)
		delete req;

	return ret;
}

ACE_THR_FUNC_RETURN UserManager::request_worker_fn(void*)
{
	return USER_MANAGER::instance()->process_requests();
}

ACE_THR_FUNC_RETURN UserManager::process_requests()
{
	ACE_InputCDR* response = 0;
	int ret = wait_for_response(0,ACE_INVALID_HANDLE,response);
	if (response)
		delete response;

	return (ACE_THR_FUNC_RETURN)ret;
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
	// Farm it off to our local StdObjectManager!!
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

int UserManager::wait_for_response(ACE_CDR::ULong trans_id, ACE_HANDLE handle, ACE_InputCDR*& response, ACE_Time_Value* deadline)
{
	for (;;)
	{
		// Get the next message
		Request* req;
		int nLeft = m_msg_queue.dequeue_prio(req,deadline);
		if (nLeft == -1)
			return -1;

		for (int i=0;i<2;++i)
		{
			// Read and set the byte order
			ACE_CDR::Octet byte_order;
			if (req->m_input->read_octet(byte_order))
			{
				req->m_input->reset_byte_order(byte_order);

				// Read the header
				ACE_CDR::ULong req_dline_secs;
				ACE_CDR::ULong req_dline_usecs;
				ACE_CDR::ULong msg_len;
				ACE_CDR::ULong request_trans_id;
				ACE_CDR::Boolean bIsRequest;

				(*req->m_input) >> msg_len;
				(*req->m_input) >> req_dline_secs;
				(*req->m_input) >> req_dline_usecs;
				(*req->m_input) >> request_trans_id;
				req->m_input->read_boolean(bIsRequest);

				if (req->m_input->good_bit())
				{
					// Check the timeout value...
					ACE_Time_Value request_deadline(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
					
					// See if we want to process it...
					if (bIsRequest)
					{
						// Process the message...
						if (req->m_bRoot)
							process_root_request(req->m_handle,*req->m_input,request_trans_id,&request_deadline);
						else
							process_request(req->m_handle,*req->m_input,request_trans_id,&request_deadline);
					}
					else if (!bIsRequest && 
							!expired_request(request_trans_id) && 
							request_trans_id == trans_id &&
							handle == req->m_handle)
					{
						// Its the response we have been waiting for...
						response = req->m_input;
						delete req;
						return 0;
					}
					else
					{
						// If there is still more in the queue
						Request* next_req = 0;
						if (nLeft > 0)
							m_msg_queue.dequeue_prio(next_req,deadline);
						
						// Put the original back in the queue, its not for us...
						int ret = m_msg_queue.enqueue_head(req,deadline);
						if (ret < 0)
						{
							if (next_req)
							{
								delete next_req->m_input;
								delete next_req;
							}
							delete req->m_input;
							delete req;
							return -1;
						}

						if (next_req)
						{
							req = next_req;
							continue;
						}
					}
				}
			}
			// If we get here then we have finished with req
			break;
		}

		// Done with this request
		delete req->m_input;
		delete req;
	}
}

int UserManager::send_synch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_InputCDR*& response, ACE_Time_Value* wait)
{
	// Set the deadline
	ACE_Time_Value deadline(ACE_OS::gettimeofday() + *wait);

	if (request.total_length() + header_padding > ACE_UINT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	// Write the header info
	ACE_OutputCDR header(1024);
	ACE_CDR::ULong request_trans_id = build_header(header,request,deadline);
	if (request_trans_id == -1)
		return -1;

	// TODO RICK! Need to work out what header_padding is...
	size_t request_len = request.total_length();
	size_t header_len = header.total_length();

	::DebugBreak();

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),wait,&sent);
	if (res == -1 || sent < header.total_length())
	{
		cancel_trans_id(request_trans_id);
		return -1;
	}
	
	// Wait for response...
	int ret = wait_for_response(request_trans_id,handle,response,&deadline);
	if (ret != 0)
		cancel_trans_id(request_trans_id);

	return ret;
}

int UserManager::send_asynch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_Time_Value* wait)
{
	// Set the deadline
	ACE_Time_Value deadline(ACE_OS::gettimeofday() + *wait);

	if (request.total_length() + header_padding > ACE_UINT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	// Write the header info
	ACE_OutputCDR header(1024);
	if (build_header(header,request,deadline) == -1)
		return -1;

	// TODO RICK! Need to work out what header_padding is...
	size_t request_len = request.total_length();
	size_t header_len = header.total_length();

	::DebugBreak();

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),wait,&sent);
	if (res == -1 || sent < header.total_length())
		return -1;
		
	return 0;
}

ACE_CDR::ULong UserManager::build_header(ACE_OutputCDR& header, const ACE_OutputCDR& request, const ACE_Time_Value& deadline)
{
	ACE_CDR::Octet byte_order = static_cast<ACE_CDR::Octet>(header.byte_order());
	header << byte_order;
	if (!header.good_bit())
		return static_cast<ACE_CDR::ULong>(-1);

	ACE_CDR::ULong request_trans_id = next_trans_id();
	ACE_CDR::Boolean bIsRequest = true;
	ACE_CDR::ULong msg_len = static_cast<ACE_CDR::ULong>(request.total_length() + header_padding);
	header << msg_len; 
	header << static_cast<const timeval*>(deadline)->tv_sec;
	header << static_cast<const timeval*>(deadline)->tv_usec;
	header << request_trans_id;
	header.write_boolean(bIsRequest);

	if (!header.good_bit())
		return static_cast<ACE_CDR::ULong>(-1);

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream	
	header.write_octet_array_mb(request.begin());
	if (!header.good_bit())
		return static_cast<ACE_CDR::ULong>(-1);

	return request_trans_id;
}
