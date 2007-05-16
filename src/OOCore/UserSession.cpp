#include "OOCore_precomp.h"

#include "./UserSession.h"
#include "./UserConnection.h"

using namespace Omega;
using namespace OTL;

// Forward declares used internally
namespace OOCore
{
	void SetServiceTable(Activation::IServiceTable* pNewTable);
	void SetRegistry(Registry::IRegistryKey* pRootKey);
}

OOCore::UserSession::UserSession() :
	m_user_handle(ACE_INVALID_HANDLE),
	m_next_trans_id(1)
{
}

OOCore::UserSession::~UserSession()
{
	if (m_user_handle != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(m_user_handle);
}

IException* OOCore::UserSession::init()
{
	string_t strSource;
	if (!USER_SESSION::instance()->init_i(strSource))
	{
		ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateInstance();
		pE->m_strDesc = ACE_OS::strerror(ACE_OS::last_error());
		pE->m_strSource = strSource;
        return pE;
	}

	IException* pE = USER_SESSION::instance()->bootstrap();
	if (pE)
		USER_SESSION::instance()->term_i();

	return pE;
}

bool OOCore::UserSession::init_i(string_t& strSource)
{
	u_short uPort;
	if (!get_port(uPort,strSource))
		return false;

    // Connect to the root
	ACE_SOCK_Connector connector;
	ACE_INET_Addr addr(uPort,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Stream stream;
	ACE_Time_Value wait(5);
	if (connector.connect(stream,addr,&wait) != 0)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Create a new UserConnection
	UserConnection*	pRC;
	ACE_NEW_RETURN(pRC,UserConnection(this),false);
	if (!pRC->open(stream.get_handle(),strSource))
	{
		delete pRC;
		return false;
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
		strSource = OMEGA_SOURCE_INFO;
		delete pRC;
		return false;
	}

	return true;
}

IException* OOCore::UserSession::bootstrap()
{
	try
	{
		// Create a new object manager for the 0 channel
		ObjectPtr<Remoting::IObjectManager> ptrOM = get_object_manager(0);

		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->CreateUnboundProxy(Remoting::OID_InterProcess,OMEGA_UUIDOF(Remoting::IInterProcessService),pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Set the service table
		ObjectPtr<Activation::IServiceTable> ptrSIP;
		ptrSIP.Attach(ptrIPS->GetServiceTable());
		SetServiceTable(ptrSIP);

		ObjectPtr<Registry::IRegistryKey> ptrRegistry;
		ptrRegistry.Attach(ptrIPS->GetRegistry());
		SetRegistry(ptrRegistry);
	}
	catch (Omega::IException* pE)
	{
		return pE;
	}

	return 0;
}

ACE_CString OOCore::UserSession::get_bootstrap_filename()
{
	#define OMEGA_BOOTSTRAP_FILE "ooserver.bootstrap"

	#if defined(OMEGA_WIN32)

		ACE_CString strFilename = "C:\\" OMEGA_BOOTSTRAP_FILE;

		char szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_CURRENT,szBuf);
		if SUCCEEDED(hr)
		{
			char szBuf2[MAX_PATH] = {0};
			if (PathCombineA(szBuf2,szBuf,"Omega Online"))
			{
				if (PathCombineA(szBuf,szBuf2,OMEGA_BOOTSTRAP_FILE))
					strFilename = szBuf;
			}
		}

		return strFilename;

	#else

		#define OMEGA_BOOTSTRAP_DIR "/var/lock/OmegaOnline"

		return ACE_CString(OMEGA_BOOTSTRAP_DIR "/" OMEGA_BOOTSTRAP_FILE));

	#endif
}

bool OOCore::UserSession::launch_server(string_t& strSource)
{
#if defined(OMEGA_WIN32)
	ACE_NT_Service service(ACE_TEXT("OOServer"));
	if (service.start_svc() != 0)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}
#else
	// Find what the server is called
	ACE_CString strExec = ACE_OS::getenv("OOSERVER");
	if (strExec.empty())
		strExec = "OOServer";

	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(strExec.c_str()) == -1)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Set the creation flags
	u_long flags = 0;
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}
#endif

	return true;
}

bool OOCore::UserSession::get_port(u_short& uPort, string_t& strSource)
{
	pid_t pid = ACE_INVALID_PID;

	// Open the Server key file
	ACE_HANDLE file = ACE_OS::open(get_bootstrap_filename().c_str(),O_RDONLY);
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
		if (!launch_server(strSource))
			return false;

        // Re-open file
		ACE_Time_Value wait(5);
		ACE_Countdown_Time timeout(&wait);
		while (wait != ACE_Time_Value::zero)
		{
			file = ACE_OS::open(get_bootstrap_filename().c_str(),O_RDONLY);
			if (file != INVALID_HANDLE_VALUE)
				break;

			timeout.update();
		}
		if (file == INVALID_HANDLE_VALUE)
		{
			// If we fail here, then the server has crashed...
			strSource = OMEGA_SOURCE_INFO;
			return false;
		}

		// Read pid again
		if (ACE_OS::read(file,&pid,sizeof(pid)) != sizeof(pid))
		{
			strSource = OMEGA_SOURCE_INFO;
			ACE_OS::close(file);
			return false;
		}
	}

	// Get the port number from the binding
	u_short uPort2;
	if (ACE_OS::read(file,&uPort2,sizeof(uPort2)) != sizeof(uPort2))
	{
		strSource = OMEGA_SOURCE_INFO;
		ACE_OS::close(file);
		return false;
	}
	ACE_OS::close(file);

	// Sort out addresses
	ACE_INET_Addr addr(uPort2,INADDR_LOOPBACK);

	// Connect to the OOServer main process...
	ACE_SOCK_Stream peer;
	if (ACE_SOCK_Connector().connect(peer,addr) == -1)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Send our uid or pid
	ACE_UINT16 cbSize = sizeof(ACE_UINT16) + sizeof(uid_t);
#if defined(OMEGA_WIN32)
	uid_t uid = ACE_OS::getpid();
#else
	uid_t uid = ACE_OS::getuid();
#endif
	if (peer.send(&cbSize,sizeof(cbSize)) != static_cast<ssize_t>(sizeof(cbSize)) ||
		peer.send(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Wait for the response to come back...
	ACE_Time_Value wait(5);
	ACE_UINT32 err = 0;
	if (peer.recv(&err,sizeof(err),&wait) != static_cast<ssize_t>(sizeof(err)))
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	if (err == 0)
	{
		if (peer.recv(&uPort,sizeof(uPort),&wait) != static_cast<ssize_t>(sizeof(uPort)))
		{
			strSource = OMEGA_SOURCE_INFO;
			return false;
		}

		return true;
	}
	else
	{
		ACE_UINT16 nCount;
		char szBuf[1024];
		if (peer.recv(&nCount,sizeof(nCount),&wait) != static_cast<ssize_t>(sizeof(nCount)))
			strSource = OMEGA_SOURCE_INFO;
		else
		{
			if (nCount >= sizeof(szBuf))
				nCount = sizeof(szBuf)-1;

			if (peer.recv(szBuf,nCount,&wait) != static_cast<ssize_t>(nCount))
				strSource = OMEGA_SOURCE_INFO;
			else
			{
				szBuf[nCount] = '\0';
				strSource = szBuf;
			}

			szBuf[1023] = '\0';
		}
		ACE_OS::last_error(err);
		return false;
	}
}

void OOCore::UserSession::term()
{
	USER_SESSION::instance()->term_i();

	USER_SESSION::close();
}

void OOCore::UserSession::term_i()
{
	// Shut down the socket...
	ACE_OS::shutdown(m_user_handle,SD_BOTH);

	// Wait for all the proactor threads to finish
	ACE_Thread_Manager::instance()->wait_grp(m_pro_thrd_grp_id);

	// Stop the message queue
	m_msg_queue.close();

	ACE_OS::sleep(1);
}

void OOCore::UserSession::connection_closed()
{
	// Stop the proactor, the socket is closed
	ACE_Proactor::instance()->proactor_end_event_loop();
}

ACE_THR_FUNC_RETURN OOCore::UserSession::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

bool OOCore::UserSession::enqueue_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	Request* req;
	ACE_NEW_RETURN(req,Request(handle,input),false);

	int ret = m_msg_queue.enqueue_prio(req);
	if (ret <= 0)
		delete req;

	return (ret > 0);
}

bool OOCore::UserSession::wait_for_response(ACE_CDR::ULong trans_id, Request*& response, const ACE_Time_Value* deadline)
{
	for (;;)
	{
		// Get the next message
		Request* req;
		int ret = m_msg_queue.dequeue_prio(req,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return false;

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
			ACE_CDR::ULong req_dline_secs;
			ACE_CDR::ULong req_dline_usecs;

			input >> msg_len;
			input >> request_trans_id;
			input >> dest_channel_id;
			input.read_boolean(bIsRequest);

			input >> req_dline_secs;
			input >> req_dline_usecs;
			ACE_Time_Value request_deadline(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
			
			ACE_CDR::UShort src_channel_id = 0;
			if (bIsRequest)
				input >> src_channel_id;
			
			if (input.good_bit() && request_deadline > ACE_OS::gettimeofday())
			{
				// See if we want to process it...
				if (bIsRequest)
				{
					// Rest of data is aligned on next boundary
					input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Skip the actual input to where we are at
					req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

					// Process the message...
					process_request(req,src_channel_id,request_trans_id,request_deadline);

					// process_request() is expected to delete req;
					req = 0;
				}
				else 
				{
					if (request_trans_id == trans_id)
					{
						// Its the request we have been waiting for...
						
						// Rest of data is aligned on next boundary
						input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

						// Skip the actual input to where we are at
						req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

						response = req;
						return true;
					}
					else
					{
						// Put the response back in the queue, its not for us...
						ret = m_msg_queue.enqueue_head(req,const_cast<ACE_Time_Value*>(deadline));
						if (ret < 0)
						{
							delete req;
							return false;
						}

						// Don't delete the request
						req = 0;
					}
				}
			}
		}

		// Done with this request
		delete req;
	}
}

bool OOCore::UserSession::pump_requests(const ACE_Time_Value* deadline)
{
	Request* response = 0;
	bool bRet = wait_for_response(0,response,deadline);
	if (response)
		delete response;
	return bRet;
}

bool OOCore::UserSession::send_synch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, Request*& response, const ACE_Time_Value& deadline)
{
	// Generate next transaction id
	long trans = 0;
	while (trans == 0)
	{
		trans = m_next_trans_id++;
	}

	ACE_CDR::ULong trans_id = static_cast<ACE_CDR::ULong>(trans);

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,trans_id,header,mb,deadline))
		return false;

	bool bRet = false;
	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait > ACE_Time_Value::zero)
	{
		// Send to the handle
		size_t sent = 0;
		ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
		if (res != -1 && sent == header.total_length())
		{
			// Wait for response...
			bRet = wait_for_response(trans_id,response,&deadline);
		}
	}
	else
	{
		ACE_OS::last_error(ETIMEDOUT);
	}

	return bRet;
}

bool OOCore::UserSession::send_asynch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,0,header,mb,deadline))
		return false;

	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return false;

	return true;
}

bool OOCore::UserSession::build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return false;

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

	if (!header.good_bit())
		return false;

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!header.replace(static_cast<ACE_CDR::Long>(header.total_length()),msg_len_point))
		return false;

	return true;
}

bool OOCore::UserSession::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return false;

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write out common header
	header.write_ulong(trans_id);
	header.write_ushort(dest_channel_id);
	header.write_boolean(false);	// Response

	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);

	if (!header.good_bit())
		return false;

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!header.replace(static_cast<ACE_CDR::Long>(header.total_length()),msg_len_point))
		return false;

	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(m_user_handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return false;

	return true;
}

void OOCore::UserSession::process_request(Request* request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline)
{
	// Init the error stream
	ACE_OutputCDR error;

	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM;
		ptrOM = get_object_manager(src_channel_id);

		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
		ptrRequest->init(*request->input());

		// Create a response if required
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse;
		if (trans_id != 0)
		{
			ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
			ptrResponse->WriteByte(0);
		}

		ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
		void* TODO; // TODO Setup the CallContext... Use a self-destructing class!

		// Convert deadline time to #msecs
		ACE_Time_Value wait = request_deadline - ACE_OS::gettimeofday();
		if (wait <= ACE_Time_Value::zero)
		{
			if (trans_id != 0)
			{
				// Error code 1 - Request timed out
				error.write_octet(1);
				if (!send_response(src_channel_id,trans_id,error.begin(),request_deadline))
					OOCORE_THROW_LASTERROR();
			}
			delete request;
			return;
		}

		ACE_UINT64 msecs = 0;
		static_cast<const ACE_Time_Value>(wait).msec(static_cast<ACE_UINT64&>(msecs));
		if (msecs > ACE_UINT32_MAX)
			msecs = ACE_UINT32_MAX;

		try
		{
			ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<Omega::uint32_t>(msecs));
		}
		catch (IException* pInner)
		{
			// Make sure we release the exception
			ObjectPtr<IException> ptrInner;
			ptrInner.Attach(pInner);

			// Reply with an exception if we can send replies...
			if (trans_id != 0)
			{
				// Dump the previous output and create a fresh output
				ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ObjectPtr<System::MetaInfo::IWireManager> ptrWM(ptrOM);
				System::MetaInfo::wire_write(ptrWM,ptrResponse,pInner,pInner->ActualIID());
			}
		}

		if (trans_id != 0)
		{
		    int err = 0;
		    ACE_Message_Block* mb = static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			if (!send_response(src_channel_id,trans_id,mb,request_deadline))
                err = ACE_OS::last_error();

            mb->release();

            if (err != 0)
				OOCORE_THROW_LASTERROR();
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (trans_id != 0)
		{
			// Error code 2 - Exception raw
			error.write_octet(2);
			string_t strDesc = pOuter->Description();
			error.write_string(static_cast<ACE_CDR::ULong>(strDesc.Length()),strDesc);
			string_t strSrc = pOuter->Source();
			error.write_string(static_cast<ACE_CDR::ULong>(strSrc.Length()),strSrc);

			send_response(src_channel_id,trans_id,error.begin(),request_deadline);
		}
	}

	delete request;
}

ObjectPtr<Remoting::IObjectManager> OOCore::UserSession::get_object_manager(ACE_CDR::UShort src_channel_id)
{
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	try
	{
		// Lookup existing..
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

            std::map<ACE_CDR::UShort,ObjectPtr<Remoting::IObjectManager> >::iterator i=m_mapOMs.find(src_channel_id);
			if (i != m_mapOMs.end())
				ptrOM = i->second;
		}

		if (!ptrOM)
		{
			// Create a new channel
			ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = ObjectImpl<OOCore::Channel>::CreateInstancePtr();
			ptrChannel->init(this,src_channel_id);

			// Create a new OM
			ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);

			// Associate it with the channel
			ptrOM->Connect(ptrChannel);

			// And add to the map
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::pair<std::map<ACE_CDR::UShort,ObjectPtr<Remoting::IObjectManager> >::iterator,bool> p = m_mapOMs.insert(std::map<ACE_CDR::UShort,ObjectPtr<Remoting::IObjectManager> >::value_type(src_channel_id,ptrOM));
			if (!p.second)
				ptrOM = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return ptrOM;
}
