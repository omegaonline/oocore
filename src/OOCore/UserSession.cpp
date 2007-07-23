#include "OOCore_precomp.h"

#include "./UserSession.h"

using namespace Omega;
using namespace OTL;

// Forward declares used internally
namespace OOCore
{
	void SetServiceTable(Activation::IServiceTable* pNewTable);
	void SetRegistry(Registry::IRegistryKey* pRootKey);
}

OOCore::UserSession::UserSession() :
	m_thrd_grp_id(-1)
{
}

OOCore::UserSession::~UserSession()
{
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
	if (!discover_server_port(uPort,strSource))
		return false;

    // Connect to the root
	ACE_SOCK_Connector connector;
	ACE_INET_Addr addr(uPort,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_Time_Value wait(5);
	if (connector.connect(m_stream,addr,&wait) != 0)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Spawn off the proactor threads
	m_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(1,io_worker_fn,this);
	if (m_thrd_grp_id == -1)
	{
		m_stream.close();
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	return true;
}

IException* OOCore::UserSession::bootstrap()
{
	try
	{
		// Create a new object manager for the 0 channel
		OMInfo omi = get_object_manager(0);

		// Create a proxy to the server interface
		IObject* pIPS = 0;
		omi.m_ptrOM->CreateUnboundProxy(Remoting::OID_InterProcess,OMEGA_UUIDOF(Remoting::IInterProcessService),pIPS);
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

ACE_WString OOCore::UserSession::get_bootstrap_filename()
{
	#define OMEGA_BOOTSTRAP_FILE L"ooserver.bootstrap"

	#if defined(OMEGA_WIN32)

		ACE_WString strFilename = L"C:\\" OMEGA_BOOTSTRAP_FILE;

		wchar_t szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_CURRENT,szBuf);
		if SUCCEEDED(hr)
		{
			wchar_t szBuf2[MAX_PATH] = {0};
			if (PathCombineW(szBuf2,szBuf,L"Omega Online"))
			{
				if (PathCombineW(szBuf,szBuf2,OMEGA_BOOTSTRAP_FILE))
					strFilename = szBuf;
			}
		}

		return strFilename;

	#else

		#define OMEGA_BOOTSTRAP_DIR L"/var/lock/omegaonline"

		return ACE_WString(OMEGA_BOOTSTRAP_DIR L"/" OMEGA_BOOTSTRAP_FILE);

	#endif
}

bool OOCore::UserSession::launch_server(string_t& strSource)
{
#if defined(OMEGA_WIN32)
	ACE_NT_Service service(L"OOServer");
	if (service.start_svc() != 0)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}
#else
	// Find what the server is called
	void* TODO;

	ACE_WString strExec = ACE_OS::getenv(L"OOSERVER");
	if (strExec.length() == 0)
		strExec = L"OOServer";

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

bool OOCore::UserSession::discover_server_port(u_short& uPort, string_t& strSource)
{
	pid_t pid = ACE_INVALID_PID;

	// Open the Server key file
	ACE_HANDLE file = ACE_OS::open(get_bootstrap_filename().c_str(),O_RDONLY);
	if (file != ACE_INVALID_HANDLE)
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
			if (file != ACE_INVALID_HANDLE)
				break;

			timeout.update();
		}
		if (file == ACE_INVALID_HANDLE)
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

	// Connect to the OOServer root process...
	ACE_SOCK_Stream peer;
	ACE_Time_Value wait(5);
	ACE_Countdown_Time timeout(&wait);
	int conn_err = 0;
	while (wait != ACE_Time_Value::zero)
	{
		conn_err = ACE_SOCK_Connector().connect(peer,addr,&wait);
		if (conn_err == 0)
			break;

		timeout.update();
	}
	if (conn_err == -1)
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Send our uid or pid
#if defined(OMEGA_WIN32)
	pid_t uid = ACE_OS::getpid();
#else
	uid_t uid = ACE_OS::getuid();
#endif
	if (peer.send(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	// Read the port
	wait = ACE_Time_Value(30);
	if (peer.recv(&uPort,sizeof(uPort),&wait) != static_cast<ssize_t>(sizeof(uPort)))
	{
		strSource = OMEGA_SOURCE_INFO;
		return false;
	}

	if (uPort == 0)
	{
		// Error!
		strSource = OMEGA_SOURCE_INFO;
		int err = 0;
		if (peer.recv(&err,sizeof(err)) == static_cast<ssize_t>(sizeof(err)))
		{
			strSource.Clear();

			char szBuf[1024];
			ssize_t r;
			while ((r=peer.recv(szBuf,1024)) > 0)
			{
				strSource += ACE_CString(szBuf,static_cast<size_t>(r)).c_str();
			}
		}
		ACE_OS::last_error(err);
		return false;
	}

	return true;
}

void OOCore::UserSession::term()
{
	USER_SESSION::instance()->term_i();

	USER_SESSION::close();
}

void OOCore::UserSession::term_i()
{
	// Shut down the socket...
	m_stream.close();

	// Wait for all the proactor threads to finish
	if (m_thrd_grp_id != -1)
		ACE_Thread_Manager::instance()->wait_grp(m_thrd_grp_id);

	// Stop the message queue

	SetRegistry(0);
}

ACE_THR_FUNC_RETURN OOCore::UserSession::io_worker_fn(void* pParam)
{
	return (ACE_THR_FUNC_RETURN)static_cast<UserSession*>(pParam)->run_read_loop();
}

int OOCore::UserSession::run_read_loop()
{
	static const ssize_t	s_initial_read = 8;
	char szBuffer[20];

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	char* pBuffer = ACE_ptr_align_binary(szBuffer,ACE_CDR::MAX_ALIGNMENT);
#else
	char* pBuffer = szBuffer;
#endif

	for (;;)
	{
		// Read the header
		ACE_Time_Value wait(60);	// We use a timeout to force ACE to block!
		ssize_t nRead = m_stream.recv(pBuffer,s_initial_read,&wait);
		if (nRead == -1 && ACE_OS::last_error() == ETIMEDOUT)
			continue;

		if (nRead != s_initial_read)
		{
			int err = ACE_OS::last_error();
			if (err == ENOTSOCK)
				err = 0;
			m_stream.close();
			return err;
		}

		// Create a temp input CDR
		ACE_InputCDR header(pBuffer,nRead);

		// Read and set the byte order
		ACE_CDR::Octet byte_order;
		ACE_CDR::Octet version;
		if (!header.read_octet(byte_order) || !header.read_octet(version))
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}
		header.reset_byte_order(byte_order);

		// Read the length
		ACE_CDR::ULong nReadLen = 0;
		if (!header.read_ulong(nReadLen))
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}

		// Subtract what we have already read
		nReadLen -= nRead;

		// Create a new message block
		ACE_Message_Block* mb = 0;
		ACE_NEW_NORETURN(mb,ACE_Message_Block(nReadLen));
		if (!mb)
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}
#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
		ACE_CDR::mb_align(mb);
#endif

		// Issue another read for the rest of the data
		nRead = m_stream.recv(mb->wr_ptr(),nReadLen);
		if (nRead != static_cast<ssize_t>(nReadLen))
		{
			int err = ACE_OS::last_error();
			mb->release();
			m_stream.close();
			return err;
		}
		mb->wr_ptr(nRead);

		// Create a new input CDR wrapping mb
		ACE_InputCDR* input = 0;
		size_t rd_ptr = static_cast<size_t>(mb->rd_ptr() - mb->base());
		size_t wr_ptr = static_cast<size_t>(mb->wr_ptr() - mb->base());
		ACE_NEW_NORETURN(input,ACE_InputCDR(mb->data_block()->duplicate(),0,rd_ptr,wr_ptr,static_cast<int>(byte_order)));

		// Done with our copy of mb
		mb->release();

		if (!input)
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}

		// Read in the message info
		Message* msg = 0;
		ACE_NEW_NORETURN(msg,Message);
		if (!msg)
		{
			int err = ACE_OS::last_error();
			delete input;
			m_stream.close();
			return err;
		}

		// Read the message
		(*input) >> msg->m_dest_channel_id;
		(*input) >> msg->m_dest_thread_id;
		(*input) >> msg->m_src_channel_id;
		(*input) >> msg->m_src_thread_id;

		ACE_CDR::ULong req_dline_secs;
		ACE_CDR::ULong req_dline_usecs;
		(*input) >> req_dline_secs;
		(*input) >> req_dline_usecs;
		msg->m_deadline = ACE_Time_Value(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
		(*input) >> msg->m_attribs;
		input->read_boolean(msg->m_bIsRequest);
		msg->m_pPayload = input;

		if (!input->good_bit())
		{
			int err = ACE_OS::last_error();
			delete msg;
			delete input;
			m_stream.close();
			return err;
		}

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
		input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

		// Find the right queue to send it to...
		{
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
			{
				int err = ACE_OS::last_error();
				delete msg;
				delete input;
				m_stream.close();
				return err;
			}

			std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.end();
			if (msg->m_dest_thread_id != 0)
			{
				i=m_mapThreadContexts.find(msg->m_dest_thread_id);
			}
			else
			{
				for (int k=0;k<2;++k)
				{
					size_t max = (size_t)-1;
					for (std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator j=m_mapThreadContexts.begin();j!=m_mapThreadContexts.end();++j)
					{
						size_t c = j->second->m_msg_queue->message_count();
						if (c < max)
						{
							if (k==1 || j->second->m_bWaitingOnZero)
							{
								i = j;
								max = c;
							}
						}
					}

					if (i!=m_mapThreadContexts.end())
						break;
				}
			}

			if (i == m_mapThreadContexts.end() ||
				i->second->m_msg_queue->enqueue_tail(msg,&msg->m_deadline) == -1)
			{
				delete msg;
				delete input;
				continue;
			}
		}
	}
}

bool OOCore::UserSession::wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance();
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = pContext->m_msg_queue->dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return false;

		if (msg->m_bIsRequest)
		{
			// Find and/or create the object manager associated with src_channel_id
			OMInfo oim = get_object_manager(msg->m_src_channel_id);

			ACE_CDR::UShort old_thread_id = oim.m_ptrChannel->set_thread_id(msg->m_src_thread_id);
			ACE_Time_Value old_deadline = pContext->m_deadline;
			pContext->m_deadline = (msg->m_deadline < pContext->m_deadline ? msg->m_deadline : pContext->m_deadline);

			// Process the message...
			process_request(oim,msg,pContext->m_deadline);

			// Restore old context
			pContext->m_deadline = old_deadline;
			old_thread_id = oim.m_ptrChannel->set_thread_id(old_thread_id);
		}
		else
		{
			response = msg->m_pPayload;
			delete msg;
			return true;
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

OOCore::UserSession::ThreadContext* OOCore::UserSession::ThreadContext::instance()
{
	ThreadContext* pThis = ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>::instance();
	if (pThis->m_thread_id == 0)
	{
		ACE_NEW_NORETURN(pThis->m_msg_queue,(ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>));
		pThis->m_thread_id = UserSession::USER_SESSION::instance()->insert_thread_context(pThis);
	}

	if (pThis->m_thread_id == 0)
		return 0;
	else
		return pThis;
}

OOCore::UserSession::ThreadContext::ThreadContext() :
	m_thread_id(0),
	m_msg_queue(0),
	m_bWaitingOnZero(false),
	m_deadline(ACE_Time_Value::max_time)
{
}

OOCore::UserSession::ThreadContext::~ThreadContext()
{
	UserSession::USER_SESSION::instance()->remove_thread_context(this);
	delete m_msg_queue;
}

// Accessors for ThreadContext
ACE_CDR::UShort OOCore::UserSession::insert_thread_context(const OOCore::UserSession::ThreadContext* pContext)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	for (ACE_CDR::UShort i=1;;++i)
	{
		if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
		{
			m_mapThreadContexts.insert(std::map<ACE_CDR::UShort,const ThreadContext*>::value_type(i,pContext));
			return i;
		}
	}
}

void OOCore::UserSession::remove_thread_context(const OOCore::UserSession::ThreadContext* pContext)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

bool OOCore::UserSession::pump_requests(const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance();
	bool bWas = pContext->m_bWaitingOnZero;
	pContext->m_bWaitingOnZero = true;

	ACE_InputCDR* response = 0;
	bool bRet = wait_for_response(response,deadline);
	if (!bRet)
	{
		if (ACE_OS::last_error() == ESHUTDOWN)
			bRet = true;
	}
	if (response)
		delete response;

	pContext->m_bWaitingOnZero = bWas;

	return bRet;
}

bool OOCore::UserSession::send_request(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs)
{
	ACE_Time_Value deadline = ThreadContext::instance()->m_deadline;
	ACE_Time_Value deadline2 = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
	if (deadline2 < deadline)
		deadline = deadline2;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,dest_thread_id,header,mb,deadline,true,attribs))
		return false;

	// Send to the handle
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (deadline <= now)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}
	ACE_Time_Value wait = deadline - now;
	bool bRet = false;
	size_t sent = 0;
	ssize_t res = m_stream.send_n(header.begin(),&wait,&sent);
	if (res != -1 && sent == header.total_length())
	{
		if (attribs & Remoting::asynchronous)
			bRet = true;
		else
			// Wait for response...
			bRet = wait_for_response(response,&deadline);
	}

	return bRet;
}

bool OOCore::UserSession::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb)
{
	ACE_Time_Value deadline = ThreadContext::instance()->m_deadline;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,dest_thread_id,header,mb,deadline,false,0))
		return false;

	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (deadline <= now)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	ACE_Time_Value wait = deadline - now;
	size_t sent = 0;
	ssize_t res = m_stream.send_n(header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return false;

	return true;
}

static bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
{
#if ACE_MAJOR_VERSION <= 5 && ACE_MINOR_VERSION <= 5 && ACE_BETA_VERSION == 0

    ACE_CDR::Long len = static_cast<ACE_CDR::Long>(stream.total_length());

#if !defined (ACE_ENABLE_SWAP_ON_WRITE)
    *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
#else
    if (!stream.do_byte_swap())
        *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
    else
        ACE_CDR::swap_4(reinterpret_cast<const char*>(len),msg_len_point);
#endif

    return true;
#else
    return stream.replace(static_cast<ACE_CDR::Long>(stream.total_length()),msg_len_point);
#endif
}

bool OOCore::UserSession::build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, bool bIsRequest, ACE_CDR::UShort attribs)
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

	// We have room for 2 bytes here!

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write the message
	header.write_ushort(dest_channel_id);
	header.write_ushort(dest_thread_id);
	header.write_ushort(0);	// src_channel_id
	header.write_ushort(ThreadContext::instance()->m_thread_id);

	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);

	header.write_ushort(attribs);

	header.write_boolean(bIsRequest);

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
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	return true;
}

void OOCore::UserSession::process_request(OMInfo& oim, const UserSession::Message* pMsg, const ACE_Time_Value& deadline)
{
	// Init the error stream
	ACE_OutputCDR error;

	try
	{
		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
		ptrRequest->init(*pMsg->m_pPayload);

		// Create a response if required
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse;
		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
			ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
			ptrResponse->WriteByte(0);
		}

		ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
		void* TODO; // TODO Setup the CallContext... Use a self-destructing class!

		// Check timeout
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
		{
			if (!(pMsg->m_attribs & Remoting::asynchronous))
			{
				// Error code 1 - Request timed out
				error.write_octet(1);
				send_response(pMsg->m_src_channel_id,pMsg->m_src_thread_id,error.begin());
			}
			return;
		}

		// Convert deadline time to #msecs
		ACE_Time_Value wait = deadline - now;
		ACE_UINT64 msecs = 0;
		static_cast<const ACE_Time_Value>(wait).msec(static_cast<ACE_UINT64&>(msecs));
		if (msecs > ACE_UINT32_MAX)
			msecs = ACE_UINT32_MAX;

		try
		{
			oim.m_ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<Omega::uint32_t>(msecs));
		}
		catch (IException* pInner)
		{
			// Make sure we release the exception
			ObjectPtr<IException> ptrInner;
			ptrInner.Attach(pInner);

			// Reply with an exception if we can send replies...
			if (!(pMsg->m_attribs & Remoting::asynchronous))
			{
				// Dump the previous output and create a fresh output
				ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ObjectPtr<System::MetaInfo::IWireManager> ptrWM(oim.m_ptrOM);
				System::MetaInfo::wire_write(ptrWM,ptrResponse,pInner,pInner->ActualIID());
			}
		}

		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
		    int err = 0;
		    ACE_Message_Block* mb = static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			if (!send_response(pMsg->m_src_channel_id,pMsg->m_src_thread_id,mb))
                err = ACE_OS::last_error();

            mb->release();

            if (err != 0)
				OOCORE_THROW_ERRNO(err);
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
			// Error code 2 - Exception raw
			error.write_octet(2);
			ACE_CString strDesc = string_t_to_utf8(pOuter->Description());
			error.write_string(strDesc);
			ACE_CString strSrc = string_t_to_utf8(pOuter->Source());
			error.write_string(strSrc);

			send_response(pMsg->m_src_channel_id,pMsg->m_src_thread_id,error.begin());
		}
	}
}

OOCore::UserSession::OMInfo OOCore::UserSession::get_object_manager(ACE_CDR::UShort src_channel_id)
{
	OMInfo info;
	try
	{
		// Lookup existing..
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

            std::map<ACE_CDR::UShort,OMInfo>::iterator i=m_mapOMs.find(src_channel_id);
			if (i != m_mapOMs.end())
				info = i->second;
		}

		if (!info.m_ptrOM)
		{
			// Create a new channel
			info.m_ptrChannel = ObjectImpl<OOCore::Channel>::CreateInstancePtr();
			info.m_ptrChannel->init(this,src_channel_id);

			// Create a new OM
			info.m_ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);

			// Associate it with the channel
			info.m_ptrOM->Connect(info.m_ptrChannel);

			// And add to the map
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::pair<std::map<ACE_CDR::UShort,OMInfo>::iterator,bool> p = m_mapOMs.insert(std::map<ACE_CDR::UShort,OMInfo>::value_type(src_channel_id,info));
			if (!p.second)
				info = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return info;
}
