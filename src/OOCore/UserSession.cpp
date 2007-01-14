#include "OOCore_precomp.h"

#include "./UserSession.h"
#include "./UserConnection.h"
#include "./Session.h"

using namespace Omega;
using namespace OTL;

UserSession::UserSession() : 
	m_user_handle(ACE_INVALID_HANDLE),
	m_next_trans_id(1)
{
}

UserSession::~UserSession()
{
	if (m_user_handle != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(m_user_handle);
}

IException* UserSession::init()
{
	int ret = USER_SESSION::instance()->init_i();
	if (ret != 0)
	{
		ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateObject();
		pE->m_strDesc = ACE_OS::strerror(ret);
        return pE;
	}

	return USER_SESSION::instance()->bootstrap();
}

int UserSession::init_i()
{
	u_short uPort;
	if (get_port(uPort) != 0)
		return -1;

	ACE_SOCK_Connector connector;
	ACE_INET_Addr addr(uPort,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Stream stream;
	ACE_Time_Value wait(5);

	// Connect to the root
	if (connector.connect(stream,addr,&wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("connect() failed")),-1);
		
	// Create a new UserConnection
	UserConnection*	pRC;
	ACE_NEW_RETURN(pRC,UserConnection(this),-1);
	if (pRC->open(stream.get_handle()) != 0)
	{
		delete pRC;
		return -1;
	}
	
	// Stash the user handle
	m_user_handle = stream.get_handle();

	// Clear the handle in the stream, pRC now owns it
	stream.set_handle(ACE_INVALID_HANDLE);

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the proactor threads
	int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,proactor_worker_fn);
	if (pro_thrd_grp_id == -1)
	{
		delete pRC;
		return -1;
	}

	return 0;
}

IException* UserSession::bootstrap()
{
	try
	{
		// Create a new object manager for the 0 channel
		ObjectPtr<Remoting::IObjectManager> ptrOM = get_object_manager(0);

		// Create a proxy to the server interface
		IObject* pSIP = 0;
		ptrOM->CreateStaticProxy(OOServer::OID_InterProcess,OOServer::IID_IStaticInterProcess,pSIP);
		ObjectPtr<OOServer::IStaticInterProcess> ptrSIP;
		ptrSIP.Attach(static_cast<OOServer::IStaticInterProcess*>(pSIP));

		// Attach to the server interface
		m_ptrServer.Attach(ptrSIP->Init());
	}
	catch (Omega::IException* pE)
	{
		return pE;
	}

	return 0;
}

int UserSession::get_port(u_short& uPort)
{
	pid_t pid = ACE_INVALID_PID;

	// Open the Server key file
	ACE_HANDLE file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_RDONLY);
	if (file != INVALID_HANDLE_VALUE)
	{
		if (ACE_OS::read(file,&pid,sizeof(pid)) == sizeof(pid))
		{
            // Check if the process is still running...
			if (ACE::process_active(pid)!=1)
			{
				pid = ACE_INVALID_PID;
			}
		}
	}
		
	if (pid == ACE_INVALID_PID)
	{
		if (file != ACE_INVALID_HANDLE)
			ACE_OS::close(file);

		// Launch the server
		
		// Find what the server is called
		ACE_TString strExec;
		if (strExec.empty())
			strExec = ACE_OS::getenv("OOSERVER");
		if (strExec.empty())
			strExec = "OOServer";

		// Set the process options
		ACE_Process_Options options;
		options.avoid_zombies(0);
		options.handle_inheritence(0);
		if (options.command_line(strExec.c_str()) == -1)
			return -1;

		// Set the creation flags
		u_long flags = 0;
#if defined (ACE_WIN32) && defined(_DEBUG)
		flags |= CREATE_NEW_CONSOLE;
#endif
		options.creation_flags(flags);

		// Spawn the process
		ACE_Process process;
		if (process.spawn(options)==ACE_INVALID_PID)
			return -1;

		// Wait 1 second for the process to launch, if it takes more than 10 seconds its probably okay
		ACE_exitcode exitcode = 0;
		if (process.wait(ACE_Time_Value(10),&exitcode) != 0)
			return -1;

		// Re-open file
		file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_RDONLY);
		if (file == INVALID_HANDLE_VALUE)
		{
			process.kill();
			return -1;
		}
	
		// Read pid again
		if (ACE_OS::read(file,&pid,sizeof(pid)) != sizeof(pid))
		{
			process.kill();
			ACE_OS::close(file);
			return -1;
		}

		// Check the pids match
		if (pid != process.getpid())
		{
			process.kill();
			ACE_OS::close(file);
			return -1;
		}
	}

	// Get the port number from the binding
	u_short uPort2;
	if (ACE_OS::read(file,&uPort2,sizeof(uPort2)) != sizeof(uPort2))
	{
		ACE_OS::close(file);
		return -1;
	}
	ACE_OS::close(file);
	
	// Sort out addresses
	ACE_INET_Addr addr(uPort2,INADDR_LOOPBACK);

	// Connect to the OOServer main process...
	ACE_SOCK_Stream peer;
	if (ACE_SOCK_Connector().connect(peer,addr) == -1)
		return -1;

	// Send our uid or pid
	Session::Request request = {0};
	request.cbSize = sizeof(request);
#ifdef OMEGA_WIN32
	request.uid = ACE_OS::getpid();
#else
	request.uid = ACE_OS::getuid();
#endif
	if (peer.send(&request,request.cbSize) != request.cbSize)
		return -1;

	// Wait for the response to come back...
	ACE_Time_Value wait(5);
	Session::Response response = {0};
	if (peer.recv(&response.cbSize,sizeof(response.cbSize),&wait) < sizeof(response.cbSize))
		return -1;

	// Check the response is valid
	if (response.cbSize < sizeof(response))
	{
		ACE_OS::last_error(EINVAL);
		return -1;
	}

	// Recv the rest...
	if (peer.recv(&response.bFailure,response.cbSize - sizeof(response.cbSize)) < static_cast<ssize_t>(response.cbSize - sizeof(response.cbSize)))
		return -1;

	// Check failure code
	if (response.bFailure)
	{
		ACE_OS::last_error(response.err);
		return -1;
	}

	uPort = response.uNewPort;
	return 0;
}

void UserSession::term()
{
	USER_SESSION::instance()->term_i();

	USER_SESSION::close();
}

void UserSession::term_i()
{
	// Shut down the socket...
	ACE_OS::shutdown(m_user_handle,SD_BOTH);

	// Wait for all the proactor threads to finish
	ACE_Thread_Manager::instance()->wait_grp(m_pro_thrd_grp_id);

	// Stop the message queue
	m_msg_queue.close();
}

void UserSession::connection_closed()
{
	// Stop the proactor, the socket is closed
	ACE_Proactor::instance()->proactor_end_event_loop();
}

ACE_THR_FUNC_RETURN UserSession::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

int UserSession::enqueue_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	Request* req;
	ACE_NEW_RETURN(req,Request(handle,input),-1);

	int ret = m_msg_queue.enqueue_prio(req);
	if (ret <= 0)
		delete req;

	return ret;
}

int UserSession::wait_for_response(ACE_CDR::ULong trans_id, Request*& response, ACE_Time_Value* deadline)
{
	for (;;)
	{
		// Get the next message
		Request* req;
		int ret = m_msg_queue.dequeue_prio(req,deadline);
		if (ret == -1)
			return -1;
		
		// Stash the current rd_ptr()
		char* rd_ptr_start = req->input()->rd_ptr();

		// Clone the input
		ACE_InputCDR input(*req->input());

		// Read and set the byte order
		ACE_CDR::Octet byte_order;
		ACE_CDR::Octet version = 0;
		if (input.read_octet(byte_order) && input.read_octet(version) && version == 1)
		{
			input.reset_byte_order(byte_order);

			// Read the header
			ACE_CDR::ULong msg_len;
			ACE_CDR::ULong request_trans_id;
			ACE_CDR::Boolean bIsRequest = false;
			ACE_CDR::UShort dest_channel_id;
			
			input >> msg_len;
			input >> request_trans_id;
			input >> dest_channel_id;
			input.read_boolean(bIsRequest);
			
			ACE_CDR::ULong req_dline_secs = 0;
			ACE_CDR::ULong req_dline_usecs = 0;
			ACE_CDR::UShort src_channel_id = 0;
			
			if (bIsRequest)
			{
				// Read the request data
				input >> req_dline_secs;
				input >> req_dline_usecs;
				input >> src_channel_id;
			}

			if (input.good_bit())
			{
				// See if we want to process it...
				if (bIsRequest)
				{
					// Check the timeout value...
					ACE_Time_Value request_deadline(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
			
					// Rest of data is aligned on next boundary
					input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Skip the actual input to where we are at
					req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

					// Process the message...
					process_request(req,src_channel_id,request_trans_id,&request_deadline);

					// process_request() is expected to delete req;
					req = 0;
				}
				else if (valid_transaction(request_trans_id) && request_trans_id == trans_id)
				{
					// Its the request we have been waiting for...

					// Rest of data is aligned on next boundary
					input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Skip the actual input to where we are at
					req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

					response = req;
					return 0;
				}
				else
				{
					// Put the response back in the queue, its not for us...
					ret = m_msg_queue.enqueue_head(req,deadline);
					if (ret < 0)
					{
						delete req;
						return -1;
					}

					// Don't delete the request
					req = 0;
				}
			}
		}
		
		// Done with this request
		delete req;
	}
}

int UserSession::pump_requests(ACE_Time_Value* deadline)
{
	Request* response = 0;
	int ret = wait_for_response(0,response,deadline);
	if (response)
		delete response;
	return ret;
}

int UserSession::send_synch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, Request*& response, ACE_Time_Value* deadline)
{
	// Generate next transaction id
	long trans = m_next_trans_id++;
	if (trans == 0)
		trans = m_next_trans_id++;
	ACE_CDR::ULong trans_id = static_cast<ACE_CDR::ULong>(trans);

	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (build_header(dest_channel_id,trans_id,header,mb,*deadline) != 0)
		return -1;
	
	// Add to pending trans set
	try
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,guard,m_lock,-1);
		m_setPendingTrans.insert(trans_id);
	}
	catch (...)
	{
		ACE_OS::last_error(EINVAL);
		return -1;
	}
	
	int ret = 0;
	ACE_Time_Value wait = *deadline - ACE_OS::gettimeofday();
	if (wait > ACE_Time_Value::zero)
	{
		// Send to the handle
		ret = -1;
		size_t sent = 0;
		ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
		if (res != -1 && sent == header.total_length())
		{
			// Wait for response...
			ret = wait_for_response(trans_id,response,deadline);
		}
	}
	else
	{
		ACE_OS::last_error(ETIMEDOUT);
		ret = -1;
	}

	// Remove from pending trans set
	try
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,guard,m_lock,-1);
		m_setPendingTrans.erase(trans_id);
	}
	catch (...)
	{}
	
	return ret;
}

int UserSession::send_asynch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, ACE_Time_Value* deadline)
{
	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (build_header(dest_channel_id,0,header,mb,*deadline) == -1)
		return -1;

	ACE_Time_Value wait = *deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return -1;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return -1;
		
	return 0;
}

int UserSession::build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return -1;

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write the common header
	header.write_ulong(trans_id);
	header.write_ushort(dest_channel_id);
	header.write_boolean(true);	// Is request
	
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);
	header.write_ushort(0); // src_channel_id
	header.write_string(0,""); //  UserId

	if (!header.good_bit())
		return -1;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream	
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return -1;

	// Update the total length
	if (!header.replace(static_cast<ACE_CDR::Long>(header.total_length()),msg_len_point))
		return -1;	

	return 0;
}

int UserSession::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* mb, ACE_Time_Value* deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return -1;

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write out common header
	header.write_ulong(trans_id);
	header.write_ushort(dest_channel_id);
	header.write_boolean(false);	// Response
	
	if (!header.good_bit())
		return -1;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream	
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return -1;

	// Update the total length
	if (!header.replace(static_cast<ACE_CDR::Long>(header.total_length()),msg_len_point))
		return -1;	

	ACE_Time_Value wait = *deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return -1;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return -1;
		
	return 0;
}

bool UserSession::valid_transaction(ACE_CDR::ULong trans_id)
{
	try
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,guard,m_lock,false);
		return (m_setPendingTrans.find(trans_id) != m_setPendingTrans.end());
	}
	catch (...)
	{
		return false;
	}
}

void UserSession::process_request(Request* request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = get_object_manager(src_channel_id);
		
		// Convert deadline time to #msecs
		ACE_Time_Value wait(*request_deadline - ACE_OS::gettimeofday());
		if (wait <= ACE_Time_Value::zero)
			OOCORE_THROW_ERRNO(ETIMEDOUT);
		ACE_UINT64 msecs = 0;
		static_cast<const ACE_Time_Value>(wait).msec(msecs);
		if (msecs > ACE_UINT32_MAX)
			msecs = ACE_UINT32_MAX;

		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrRequest = ObjectImpl<OOCore::InputCDR>::CreateObjectPtr();
		ptrRequest->init(*request->input());

		// Create a response if required
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse;
		if (trans_id != 0)
			ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateObjectPtr();

		ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
		try		
		{
			void* TODO; // Setup the CallContext...

			ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<uint32_t>(msecs));
		}
		catch (...)
		{
			void* TODO; // Restore CallContext

			throw;
		}
		void* TODO; // Restore CallContext
	}
	catch (IException* pE)
	{
		void* TODO; // Report the error

		pE->Release();
	}
	
	delete request;
}

ObjectPtr<Remoting::IObjectManager> UserSession::get_object_manager(ACE_CDR::UShort src_channel_id)
{
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

		std::map<ACE_CDR::UShort,ObjectPtr<Remoting::IObjectManager> >::iterator i=m_mapOMs.find(src_channel_id);
		if (i == m_mapOMs.end())
		{
			// Create a new channel
			ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateObjectPtr();
			ptrChannel->init(this,src_channel_id);

			// Create a new OM
			ptrOM = ObjectPtr<Remoting::IObjectManager>(OID_StdObjectManager,Activation::InProcess);

			// Associate it with the channel
			ptrOM->Connect(ptrChannel);

			// And add to the map
			m_mapOMs.insert(std::map<ACE_CDR::UShort,ObjectPtr<Remoting::IObjectManager> >::value_type(src_channel_id,ptrOM));
		}
		else
		{
			ptrOM = i->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return ptrOM;
}
