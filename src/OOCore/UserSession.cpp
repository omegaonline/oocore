///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "./UserSession.h"

using namespace Omega;
using namespace OTL;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)

int OOCore::UserSession::MessagePipe::connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait)
{
	ACE_Time_Value val(30);
	if (!wait)
		wait = &val;

	ACE_Countdown_Time countdown(wait);

	ACE_SPIPE_Connector connector;
	ACE_SPIPE_Addr addr;

	ACE_SPIPE_Stream up;
	addr.string_to_addr((strAddr + L"\\up").c_str());
	if (connector.connect(up,addr,wait,ACE_Addr::sap_any,0,O_WRONLY) != 0)
		return -1;

	countdown.update();

	ACE_SPIPE_Stream down;
	addr.string_to_addr((strAddr + L"\\down").c_str());
	if (connector.connect(down,addr,wait,ACE_Addr::sap_any,0,O_RDWR | FILE_FLAG_OVERLAPPED) != 0)
		return -1;

	pipe.m_hRead = down.get_handle();
	pipe.m_hWrite = up.get_handle();

	up.set_handle(ACE_INVALID_HANDLE);
	down.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void OOCore::UserSession::MessagePipe::close()
{
	ACE_HANDLE hRead = m_hRead;
	m_hRead = ACE_INVALID_HANDLE;
	ACE_HANDLE hWrite = m_hWrite;
	m_hWrite = ACE_INVALID_HANDLE;

	if (hRead != ACE_INVALID_HANDLE)
		ACE_OS::close(hRead);

	if (hWrite != ACE_INVALID_HANDLE)
	{
		ACE_OS::fsync(hWrite);
		ACE_OS::close(hWrite);
	}
}

ssize_t OOCore::UserSession::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value*, size_t* sent)
{
	return ACE::write_n(m_hWrite,mb,sent);
}

ssize_t OOCore::UserSession::MessagePipe::recv(void* buf, size_t len)
{
	ssize_t nRead = ACE_OS::read_n(m_hRead,buf,len);
	if (nRead == -1 && ACE_OS::last_error() == ERROR_MORE_DATA)
		nRead = static_cast<ssize_t>(len);

	return nRead;
}

#else // defined(ACE_HAS_WIN32_NAMED_PIPES)

int OOCore::UserSession::MessagePipe::connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait)
{
	ACE_UNIX_Addr addr(strAddr.c_str());

	ACE_SOCK_Stream stream;
	if (ACE_SOCK_Connector().connect(stream,addr,wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"connector.connect() failed"),-1);

	pipe.m_hSocket = stream.get_handle();
	stream.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void OOCore::UserSession::MessagePipe::close()
{
	ACE_HANDLE hSocket = m_hSocket;
	m_hSocket = ACE_INVALID_HANDLE;

	if (hSocket != ACE_INVALID_HANDLE)
		ACE_OS::close(hSocket);
}

ssize_t OOCore::UserSession::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value* timeout, size_t* sent)
{
	return ACE::send_n(m_hSocket,mb,timeout,sent);
}

ssize_t OOCore::UserSession::MessagePipe::recv(void* buf, size_t len)
{
	for (;;)
	{
		ACE_Time_Value wait(60);	// We use a timeout to force ACE to block!
		ssize_t nRead = ACE_OS::recv(m_hSocket,(char*)buf,len,&wait);
		if (nRead != -1 || ACE_OS::last_error() != ETIMEDOUT)
			return nRead;
	}
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)

OOCore::UserSession::UserSession() :
	m_thrd_grp_id(-1), m_channel_id(0), m_nIPSCookie(0)
{
}

OOCore::UserSession::~UserSession()
{
}

IException* OOCore::UserSession::init()
{
	if (!USER_SESSION::instance()->init_i())
	{
		ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateInstance();
		pE->m_strDesc = L"Failed to connect to server process, please check installation";
		pE->m_strSource = L"Omega::Initialize";

		//USER_SESSION::close();
        return pE;
	}

	IException* pE = USER_SESSION::instance()->bootstrap();
	if (pE)
		term();
	
	return pE;
}

bool OOCore::UserSession::init_i()
{
	ACE_WString strPipe;
	if (!discover_server_port(strPipe))
		return false;

    // Connect to the root
	ACE_Time_Value wait(5);
	if (MessagePipe::connect(m_stream,strPipe,&wait) != 0)
		return false;

	// Read our channel id
	if (m_stream.recv(&m_channel_id,sizeof(m_channel_id)) != static_cast<ssize_t>(sizeof(m_channel_id)))
	{
		m_stream.close();
		return false;
	}

	// Spawn off the proactor threads
	m_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(1,io_worker_fn,this);
	if (m_thrd_grp_id == -1)
	{
		m_stream.close();
		return false;
	}

	return true;
}

IException* OOCore::UserSession::bootstrap()
{
	try
	{
		// Create a new object manager for the user channel
		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(m_channel_id & 0xFF000000,Remoting::inter_process);

		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->CreateRemoteInstance(Remoting::OID_InterProcessService,OMEGA_UUIDOF(Remoting::IInterProcessService),0,pIPS);

		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Register locally...
		m_nIPSCookie = Activation::RegisterObject(Remoting::OID_InterProcessService,ptrIPS,Activation::InProcess,Activation::MultipleUse);
	}
	catch (IException* pE)
	{
		return pE;	
	}

	return 0;
}

bool OOCore::UserSession::launch_server()
{
#if defined(OMEGA_WIN32)
	ACE_NT_Service service(L"OOServer");
	ACE_Time_Value wait(30);
	if (service.start_svc(&wait) != 0)
		return false;

#else
	// Find what the server is called
	ACE_WString strExec = ACE_Ascii_To_Wide(ACE_OS::getenv("OOSERVER")).wchar_rep();
	if (strExec.empty())
		strExec = L"ooserver";

	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(strExec.c_str()) == -1)
		return false;

	// Set the creation flags
	u_long flags = 0;
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		return false;

#endif

	return true;
}

bool OOCore::UserSession::discover_server_port(ACE_WString& strPipe)
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	ACE_SPIPE_Connector connector;
	ACE_SPIPE_Stream peer;
	ACE_SPIPE_Addr addr;
	addr.string_to_addr(L"ooserver");
#else
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream peer;
	ACE_UNIX_Addr addr(L"/var/ooserver");
#endif

	ACE_Time_Value wait(10);
	if (connector.connect(peer,addr,&wait) != 0)
	{
		// Launch the server
		if (!launch_server())
			return false;

		ACE_Countdown_Time countdown(&wait);
		do
		{
			ACE_OS::sleep(1);

			// Try again
			countdown.update();
		} while (connector.connect(peer,addr,&wait) != 0 && wait != ACE_Time_Value::zero);

		if (wait == ACE_Time_Value::zero)
			return false;
	}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	// Send nothing
	HANDLE uid = 0;
#else
	// Send our uid
	uid_t uid = ACE_OS::getuid();
#endif
	if (peer.send(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
		return false;

	// Read the string length
	size_t uLen = 0;
	if (peer.recv(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)))
		return false;

	// Check for the integer overflow...
	if (uLen > (size_t)-1 / sizeof(wchar_t))
		return false;

	// Read the string
	wchar_t* buf;
	ACE_NEW_RETURN(buf,wchar_t[uLen],false);

	if (peer.recv(buf,uLen*sizeof(wchar_t)) != static_cast<ssize_t>(uLen*sizeof(wchar_t)))
	{
		delete [] buf;
		return false;
	}

	ACE_WString str = buf;
	delete [] buf;

	strPipe = str;
	return true;
}

void OOCore::UserSession::term()
{
	USER_SESSION::instance()->term_i();
}

void OOCore::UserSession::term_i()
{
	// Unregister InterProcessService
	if (m_nIPSCookie)
	{
		Activation::RevokeObject(m_nIPSCookie);
		m_nIPSCookie = 0;
	}

	// Shut down the socket...
	m_stream.close();

	// Wait for all the proactor threads to finish
	if (m_thrd_grp_id != -1)
		ACE_Thread_Manager::instance()->wait_grp(m_thrd_grp_id);
}

ACE_THR_FUNC_RETURN OOCore::UserSession::io_worker_fn(void* pParam)
{
	return (ACE_THR_FUNC_RETURN)static_cast<UserSession*>(pParam)->run_read_loop();
}

int OOCore::UserSession::run_read_loop()
{
	static const ssize_t s_initial_read = ACE_CDR::LONG_SIZE;
	char szBuffer[ACE_CDR::LONG_SIZE + ACE_CDR::MAX_ALIGNMENT];
	char* pBuffer = ACE_ptr_align_binary(szBuffer,ACE_CDR::MAX_ALIGNMENT);

	for (;;)
	{
		// Read the header
		ssize_t nRead = m_stream.recv(pBuffer,s_initial_read);
		if (nRead != s_initial_read)
		{
			int err = ACE_OS::last_error();
#if defined(OMEGA_WIN32)
			if (err == ERROR_BROKEN_PIPE)
#else
			if (err == ENOTSOCK)
#endif
			{
				err = 0;
			}
			m_stream.close();
			return err;
		}

		// Create a temp input CDR
		ACE_InputCDR header(pBuffer,nRead);

		// Read the length
		ACE_CDR::ULong nReadLen = 0;
		if (!header.read_ulong(nReadLen))
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}

		// Create a new message block
		ACE_Message_Block* mb = 0;
		ACE_NEW_NORETURN(mb,ACE_Message_Block(nReadLen));
		if (!mb)
		{
			int err = ACE_OS::last_error();
			m_stream.close();
			return err;
		}

		ACE_CDR::mb_align(mb);

		// Skip forwards over what we have already read
		mb->rd_ptr(s_initial_read);
		mb->wr_ptr(s_initial_read);

		// Subtract what we have already read
		nReadLen -= nRead;

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
		ACE_NEW_NORETURN(input,ACE_InputCDR(mb));

		// Done with our copy of mb
		mb->release();

		if (!input)
		{
			m_stream.close();
			return ENOMEM;
		}

		// Read the destination
		ACE_CDR::ULong dest_channel_id;
		(*input) >> dest_channel_id;

		// Read the deadline
		ACE_CDR::ULong req_dline_secs;
		(*input) >> req_dline_secs;
		ACE_CDR::ULong req_dline_usecs;
		(*input) >> req_dline_usecs;
		ACE_Time_Value deadline = ACE_Time_Value(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));

		// Did everything make sense?
		if (!input->good_bit() || (dest_channel_id & 0xFFFFF000) != m_channel_id)
		{
			int err = ACE_OS::last_error();
			delete input;
			m_stream.close();
			return err;
		}

		// Create a new Message struct
		Message* msg = 0;
		ACE_NEW_NORETURN(msg,Message);
		if (!msg)
		{
			delete input;
			m_stream.close();
			return ENOMEM;
		}

		msg->m_deadline = deadline;

		// Read the rest of the message
		(*input) >> msg->m_src_channel_id;
		(*input) >> msg->m_dest_thread_id;
		(*input) >> msg->m_src_thread_id;
		(*input) >> msg->m_attribs;

		// Did everything make sense?
		if (!input->good_bit())
		{
			int err = ACE_OS::last_error();
			delete msg;
			delete input;
			m_stream.close();
			return err;
		}

		// Now validate the apartment...
		ACE_CDR::UShort dest_apartment_id = static_cast<ACE_CDR::UShort>(dest_channel_id & ~0xFFFFF000);
		if (dest_apartment_id)
		{
			if (msg->m_dest_thread_id != 0 && msg->m_dest_thread_id != dest_apartment_id)
			{
				// Destination thread is not the destination apartment!
				delete msg;
				delete input;
				m_stream.close();
				return EACCES;
			}

			// Route to the correct thread
			msg->m_dest_thread_id = dest_apartment_id;
		}

		// Align
		input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

		// Read the payload specific data
		ACE_CDR::Octet byte_order;
		input->read_octet(byte_order);
		ACE_CDR::Octet version;
		input->read_octet(version);

		input->reset_byte_order(byte_order);
	
		(*input) >> msg->m_flags;

		// Did everything make sense?
		if (!input->good_bit() || version != 1)
		{
			int err = ACE_OS::last_error();
			delete msg;
			delete input;
			m_stream.close();
			return err;
		}

		// Align
		input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
		msg->m_pPayload = input;

		// Find the right queue to send it to...
		if (msg->m_dest_thread_id != 0)
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

			try
			{
				std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
				if (i == m_mapThreadContexts.end())
				{
					delete msg;
					delete input;
					m_stream.close();
					return EACCES;
				}				
					
				if (i->second->m_msg_queue->enqueue_tail(msg,&msg->m_deadline) == -1)
				{
					int err = ACE_OS::last_error();
					delete msg;
					delete input;
					m_stream.close();
					return err;
				}
			}
			catch (std::exception&)
			{
				delete msg;
				delete input;
				m_stream.close();
				return EINVAL;
			}
		}
		else if (m_default_msg_queue.enqueue_tail(msg,&msg->m_deadline) == -1)
		{
			int err = ACE_OS::last_error();
			delete msg;
			delete input;
			m_stream.close();
			return err;
		}
	}
}

void OOCore::UserSession::pump_requests(const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance();
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = m_default_msg_queue.dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return;

		if (msg->m_flags & Message::Request)
		{
			// Set deadline
			pContext->m_deadline = msg->m_deadline;
			if (deadline && *deadline < pContext->m_deadline)
				pContext->m_deadline = *deadline;

			try
			{
				// Set per channel thread id
				pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
			}
			catch (std::exception&)
			{
				// This shouldn't ever occur, but that means it will ;)
				delete msg->m_pPayload;
				delete msg;
				continue;
			}
			
			// Process the message...
			process_request(msg,pContext->m_deadline);

			// Clear the thread map
			pContext->m_mapChannelThreads.clear();
		}

		delete msg->m_pPayload;
		delete msg;
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

		if (msg->m_flags & Message::Request)
		{
			// Update deadline
			ACE_Time_Value old_deadline = pContext->m_deadline;
			pContext->m_deadline = (msg->m_deadline < *deadline ? msg->m_deadline : *deadline);

			// Set per channel thread id
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>::iterator i;
			try
			{
				i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
				if (i != pContext->m_mapChannelThreads.end())
					i = pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,0)).first;
			}
			catch (std::exception&)
			{
				// This shouldn't ever occur, but that means it will ;)
				pContext->m_deadline = old_deadline;
				delete msg->m_pPayload;
				delete msg;
				continue;
			}

			ACE_CDR::UShort old_thread_id = i->second;
			i->second = msg->m_src_thread_id;

			// Process the message...
			process_request(msg,pContext->m_deadline);

			// Restore old context
			pContext->m_deadline = old_deadline;
			i->second = old_thread_id;
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

	try
	{
		for (ACE_CDR::UShort i=1;;++i)
		{
			if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
			{
				m_mapThreadContexts.insert(std::map<ACE_CDR::UShort,const ThreadContext*>::value_type(i,pContext));
				return i;
			}
		}
	}
	catch (std::exception&)
	{
		return 0;
	}
}

void OOCore::UserSession::remove_thread_context(const OOCore::UserSession::ThreadContext* pContext)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

bool OOCore::UserSession::send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::ULong attribs)
{
	const ThreadContext* pContext = ThreadContext::instance();

	ACE_Time_Value deadline = pContext->m_deadline;
	ACE_Time_Value deadline2 = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
	if (deadline2 < deadline)
		deadline = deadline2;

	// Determine dest_thread_id
	ACE_CDR::UShort dest_thread_id = 0;
	try
	{
		std::map<ACE_CDR::ULong,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(pContext->m_thread_id);
		if (i != pContext->m_mapChannelThreads.end())
			dest_thread_id = i->second;
	}
	catch (std::exception&)
	{
		return false;
	}
	
	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(pContext,dest_channel_id,dest_thread_id,header,mb,deadline,true,attribs))
		return false;

	// Send to the handle
	ACE_Time_Value now = ACE_OS::gettimeofday();
	
	// Scope lock...
	{
        ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_send_lock,false);

		size_t sent = 0;
		ACE_Time_Value wait = deadline - now;

		if (m_stream.send(header.begin(),&wait,&sent) == -1)
			return false;

		if (sent != header.total_length())
			return false;
	}

	if (attribs & Remoting::asynchronous)
		return true;
	else
		// Wait for response...
		return wait_for_response(response,&deadline);
}

void OOCore::UserSession::send_response(ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb)
{
	const ThreadContext* pContext = ThreadContext::instance();
	ACE_Time_Value deadline = pContext->m_deadline;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(pContext,dest_channel_id,dest_thread_id,header,mb,deadline,false,0))
		return;

	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (deadline <= now)
		return;

	ACE_GUARD(ACE_Thread_Mutex,guard,m_send_lock);

	ACE_Time_Value wait = deadline - now;
	m_stream.send(header.begin(),&wait);
}

namespace OOCore
{
	bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point);
}

bool OOCore::ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
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

bool OOCore::UserSession::build_header(const ThreadContext* pContext, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort flags, ACE_CDR::ULong attribs)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	header << dest_channel_id;
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);

	void* TODO;	// Apartment stuff goes here

	header << m_channel_id;
	header << dest_thread_id;
	header << pContext->m_thread_id;
	header << attribs;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	 // version
	header << flags;

	if (!header.good_bit())
		return false;
	
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	return true;
}

void OOCore::UserSession::process_request(const UserSession::Message* pMsg, const ACE_Time_Value& deadline)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		Remoting::MarshalFlags_t mflags = Remoting::same;
		if ((pMsg->m_src_channel_id & 0xFF000000) == (m_channel_id & 0xFF000000))
			mflags = Remoting::inter_process;
		else if ((pMsg->m_src_channel_id & 0x80000000) == (m_channel_id & 0x80000000))
			mflags = Remoting::inter_user;
		else 
			mflags = Remoting::another_machine;

		ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(pMsg->m_src_channel_id,mflags);

		// Wrap up the request
		ObjectPtr<ObjectImpl<InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<InputCDR>::CreateInstancePtr();
		ptrRequest->init(*pMsg->m_pPayload);

		// Create a response if required
		ObjectPtr<ObjectImpl<OutputCDR> > ptrResponse;
		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
			ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
			ptrResponse->WriteByte(0);
		}

		// Check timeout
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			return;

		try
		{
			ptrOM->Invoke(ptrRequest,ptrResponse);
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
				ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ptrOM->MarshalInterface(ptrResponse,pInner->ActualIID(),pInner);
			}
		}

		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
		    ACE_Message_Block* mb = static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			send_response(pMsg->m_src_channel_id,pMsg->m_src_thread_id,mb);
            mb->release();
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (!(pMsg->m_attribs & Remoting::asynchronous))
		{
			ACE_OutputCDR error;

			// Error code 1 - Exception raw
			error.write_octet(1);
			error.write_string(string_t_to_utf8(pOuter->Description()));
			error.write_string(string_t_to_utf8(pOuter->Source()));

			send_response(pMsg->m_src_channel_id,pMsg->m_src_thread_id,error.begin());
		}
	}
}

ObjectPtr<Remoting::IObjectManager> OOCore::UserSession::create_object_manager(ACE_CDR::ULong src_channel_id, Remoting::MarshalFlags_t marshal_flags)
{
	try
	{
		// Lookup existing..
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

            std::map<ACE_CDR::ULong,OMInfo>::iterator i=m_mapOMs.find(src_channel_id);
			if (i != m_mapOMs.end())
			{
				if (i->second.m_marshal_flags == marshal_flags)
					return i->second.m_ptrOM;

				OOCORE_THROW_ERRNO(EINVAL);
			}
		}

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
		ptrChannel->init(src_channel_id);

		// Create a new OM
		OMInfo info;
		info.m_marshal_flags = marshal_flags;
		info.m_ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);

		// Associate it with the channel
		info.m_ptrOM->Connect(ptrChannel,marshal_flags);

		// And add to the map
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<ACE_CDR::ULong,OMInfo>::iterator,bool> p = m_mapOMs.insert(std::map<ACE_CDR::ULong,OMInfo>::value_type(src_channel_id,info));
		if (!p.second)
		{
			if (p.first->second.m_marshal_flags != info.m_marshal_flags)
				OOCORE_THROW_ERRNO(EINVAL);		

			info.m_ptrOM = p.first->second.m_ptrOM;
		}

		return info.m_ptrOM;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void OOCore::UserSession::handle_requests(uint32_t timeout)
{
	ACE_Time_Value wait(timeout/1000,(timeout % 1000) * 1000);
	wait += ACE_OS::gettimeofday();

	ACE_Time_Value* wait2 = &wait;
	if (timeout == (uint32_t)-1)
		wait2 = 0;

	USER_SESSION::instance()->pump_requests(wait2);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_HandleRequests,1,((in),const uint32_t&,timeout))
{
	OOCore::UserSession::handle_requests(timeout);
}
