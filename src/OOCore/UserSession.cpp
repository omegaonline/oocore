///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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
#include "./Activation.h"

using namespace Omega;
using namespace OTL;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)

int OOCore::UserSession::MessagePipe::connect(MessagePipe& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait)
{
	ACE_TString strPipe = ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str());

	ACE_Time_Value val(30);
	if (!wait)
		wait = &val;

	ACE_Countdown_Time countdown(wait);

	ACE_SPIPE_Connector connector;
	ACE_SPIPE_Addr addr;

	ACE_SPIPE_Stream down;
	addr.string_to_addr((strPipe + ACE_TEXT("\\down")).c_str());
	if (connector.connect(down,addr,wait,ACE_Addr::sap_any,0,O_RDWR | FILE_FLAG_OVERLAPPED) != 0)
		return -1;

	countdown.update();

	ACE_SPIPE_Stream up;
	addr.string_to_addr((strPipe + ACE_TEXT("\\up")).c_str());
	if (connector.connect(up,addr,wait,ACE_Addr::sap_any,0,O_WRONLY) != 0)
	{
		down.close();
		return -1;
	}

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
	if (nRead == -1 && ::GetLastError() == ERROR_MORE_DATA)
		nRead = static_cast<ssize_t>(len);

	return nRead;
}

#else // defined(ACE_HAS_WIN32_NAMED_PIPES)

int OOCore::UserSession::MessagePipe::connect(MessagePipe& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait)
{
	ACE_UNIX_Addr addr(ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str()));

	if (ACE_SOCK_Connector().connect(pipe.m_stream,addr,wait) != 0)
		return -1;

	return 0;
}

void OOCore::UserSession::MessagePipe::close()
{
	m_stream.close_writer();
	m_stream.close();
}

ssize_t OOCore::UserSession::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value* timeout, size_t* sent)
{
	return m_stream.send_n(mb,timeout,sent);
}

ssize_t OOCore::UserSession::MessagePipe::recv(void* buf, size_t len)
{
	return m_stream.recv(buf,len);
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)

OOCore::UserSession::UserSession() :
	m_thrd_grp_id(-1),
	m_channel_id(0),
	m_nIPSCookie(0),
	m_consumers(0)
{
}

OOCore::UserSession::~UserSession()
{
}

IException* OOCore::UserSession::init()
{
	if (!USER_SESSION::instance()->init_i())
		return ISystemException::Create(L"Failed to connect to server process, please check installation",L"Omega::Initialize");

	IException* pE = USER_SESSION::instance()->bootstrap();
	if (pE)
		term();

	return pE;
}

bool OOCore::UserSession::init_i()
{
	ACE_CString strPipe;
	if (!discover_server_port(strPipe))
		return false;

	// Connect to the root - we should loop here, it might take a while
	// for the accept socket to be created...
	ACE_Time_Value wait(10);
	if (MessagePipe::connect(m_stream,strPipe,&wait) != 0)
	{
		ACE_Countdown_Time countdown(&wait);
		do
		{
			ACE_OS::sleep(ACE_Time_Value(0,100000));

			// Try again
			countdown.update();
		} while (MessagePipe::connect(m_stream,strPipe,&wait) != 0 && wait != ACE_Time_Value::zero);

		if (wait == ACE_Time_Value::zero)
			return false;
	}

	// Read our channel id
	if (m_stream.recv(&m_channel_id,sizeof(m_channel_id)) != sizeof(m_channel_id))
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
		ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(m_channel_id & 0xFF000000,guid_t::Null());
		 
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->GetRemoteInstance(Remoting::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_UUIDOF(Remoting::IInterProcessService),pIPS);

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
	ACE_NT_Service service(ACE_TEXT("OOServer"));
	ACE_Time_Value wait(30);
	return (service.start_svc(&wait) == 0);

#else

    // No point trying to start ooserverd, because we don't want it setuid(0)
    // and it can't run if its not!
    return false;

#endif
}

bool OOCore::UserSession::discover_server_port(ACE_CString& strPipe)
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	ACE_SPIPE_Connector connector;
	ACE_SPIPE_Stream peer;
	ACE_SPIPE_Addr addr;
	addr.string_to_addr(ACE_TEXT("OOServer"));
#else
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream peer;
	ACE_UNIX_Addr addr(ACE_TEXT("/tmp/omegaonline/ooserverd"));
#endif

	ACE_Time_Value wait(20);
	if (connector.connect(peer,addr,&wait) != 0)
	{
		// Launch the server
		if (!launch_server())
			return false;

		ACE_Countdown_Time countdown(&wait);
		do
		{
			ACE_OS::sleep(ACE_Time_Value(0,100000));

			// Try again
			countdown.update();
		} while (connector.connect(peer,addr,&wait) != 0 && wait != ACE_Time_Value::zero);

		if (wait == ACE_Time_Value::zero)
			return false;
	}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	// Send nothing, but we must send...
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

	// Read the string
	char* buf;
	ACE_NEW_RETURN(buf,char[uLen],false);

	if (peer.recv(buf,uLen) != static_cast<ssize_t>(uLen))
	{
		delete [] buf;
		return false;
	}

	ACE_CString str(buf);
	delete [] buf;

	strPipe = str;
	return true;
}

void OOCore::UserSession::term()
{
	USER_SESSION::instance()->term_i();

	SERVICE_MANAGER::close();

	USER_SESSION::close();
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

	// Tell all worker threads that we are done with them...
	for (std::map<ACE_CDR::UShort,ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
	{
		i->second->m_thread_id = 0;
	}

	// Stop the message queue
	m_default_msg_queue.close();

	// Close all open OM's
	for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator j=m_mapChannels.begin();j!=m_mapChannels.end();++j)
	{
		j->second->disconnect();
	}
	m_mapChannels.clear();
}

ACE_THR_FUNC_RETURN OOCore::UserSession::io_worker_fn(void* pParam)
{
	return (ACE_THR_FUNC_RETURN)(static_cast<UserSession*>(pParam)->run_read_loop());
}

int OOCore::UserSession::run_read_loop()
{
	static const ssize_t s_initial_read = ACE_CDR::LONG_SIZE * 2;
	char szBuffer[ACE_CDR::LONG_SIZE*2 + ACE_CDR::MAX_ALIGNMENT];
	char* pBuffer = ACE_ptr_align_binary(szBuffer,ACE_CDR::MAX_ALIGNMENT);

	int err = 0;
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
			break;
		}

		// Create a temp input CDR
		ACE_InputCDR header(pBuffer,nRead);

		// Read the payload specific data
		ACE_CDR::Octet byte_order;
		header.read_octet(byte_order);
		ACE_CDR::Octet version;
		header.read_octet(version);

		// Set the read for the right endianess
		header.reset_byte_order(byte_order);

		// Read the length
		ACE_CDR::ULong nReadLen = 0;
		if (!header.read_ulong(nReadLen) || version != 1)
		{
			err = ACE_OS::last_error();
			break;
		}

		// Create a new message block
		ACE_Message_Block* mb = 0;
		ACE_NEW_NORETURN(mb,ACE_Message_Block(nReadLen));
		if (!mb)
		{
			err = ACE_OS::last_error();
			break;
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
			err = ACE_OS::last_error();
			mb->release();
			break;
		}
		mb->wr_ptr(nRead);

		// Create a new Message struct
		Message* msg = 0;
		ACE_NEW_NORETURN(msg,Message);
		if (!msg)
		{
			err = ENOMEM;
			mb->release();
			break;
		}

		ACE_InputCDR* pI = 0;
		ACE_NEW_NORETURN(pI,ACE_InputCDR(mb));
		
		// Done with mb now
		mb->release();

		if (!pI)
		{
			err = ENOMEM;
			delete msg;
			break;
		}

		msg->m_ptrPayload.reset(pI);

		// Reset the byte order
		msg->m_ptrPayload->reset_byte_order(byte_order);

		// Read the destination and source channels
		ACE_CDR::ULong dest_channel_id;
		(*msg->m_ptrPayload) >> dest_channel_id;
		(*msg->m_ptrPayload) >> msg->m_src_channel_id;

		// Read the deadline
		ACE_CDR::ULongLong req_dline_secs;
		(*msg->m_ptrPayload) >> req_dline_secs;
		ACE_CDR::Long req_dline_usecs;
		(*msg->m_ptrPayload) >> req_dline_usecs;
		msg->m_deadline = ACE_Time_Value(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));

		// Read the rest of the message
		(*msg->m_ptrPayload) >> msg->m_attribs;
		(*msg->m_ptrPayload) >> msg->m_dest_thread_id;
		(*msg->m_ptrPayload) >> msg->m_src_thread_id;

		// Did everything make sense?
		if (!msg->m_ptrPayload->good_bit() || (dest_channel_id & 0xFFFFF000) != m_channel_id)
		{
			err = ACE_OS::last_error();
			delete msg;
			break;
		}

		// Now validate the apartment...
		ACE_CDR::UShort dest_apartment_id = static_cast<ACE_CDR::UShort>(dest_channel_id & ~0xFFFFF000);
		if (dest_apartment_id)
		{
			if (msg->m_dest_thread_id != 0 && msg->m_dest_thread_id != dest_apartment_id)
			{
				// Destination thread is not the destination apartment!
				err = EACCES;
				delete msg;
				break;
			}

			// Route to the correct thread
			msg->m_dest_thread_id = dest_apartment_id;
		}

		(*msg->m_ptrPayload) >> msg->m_seq_no;
		(*msg->m_ptrPayload) >> msg->m_type;

		if (msg->m_ptrPayload->length() > 0)
		{
			// 6 Bytes of padding here
			msg->m_ptrPayload->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
		}

		// Did everything make sense?
		if (!msg->m_ptrPayload->good_bit())
		{
			err = ACE_OS::last_error();
			delete msg;
			break;
		}

		if ((msg->m_attribs & Message::system_message) && msg->m_type == Message::Request)
		{
			// Process system messages here... because the main message pump may not be running currently
			if ((msg->m_attribs & Message::system_message)==Message::channel_close)
			{
				ACE_CDR::ULong closed_channel_id;
				(*msg->m_ptrPayload) >> closed_channel_id;
				if (!msg->m_ptrPayload->good_bit())
					err = ACE_OS::last_error();
				else
				{
					process_channel_close(closed_channel_id);
					delete msg;
				}
			}
			else if ((msg->m_attribs & Message::system_message)==Message::channel_reflect)
			{
				// Send back the src_channel_id
				ACE_OutputCDR response;
				response.write_ulong(msg->m_src_channel_id);

				send_response(msg->m_seq_no,msg->m_src_channel_id,msg->m_src_thread_id,response.current(),msg->m_deadline,Message::synchronous | Message::channel_reflect);
				delete msg;
			}
			else
			{
				// What?!?
				err = EINVAL;
			}
		}
		else if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
			{
				err = ACE_OS::last_error();
				delete msg;
				break;
			}

			try
			{
				std::map<ACE_CDR::UShort,ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
				if (i == m_mapThreadContexts.end())
					err = EACCES;
				else
				{
					if (i->second->m_msg_queue->enqueue_tail(msg,msg->m_deadline==ACE_Time_Value::max_time ? 0 : &msg->m_deadline) == -1)
						err = ACE_OS::last_error();
					else if (i->second->m_usage == 0)
					{
						// This incoming request may not be processed for some time...
						void* TICKET_91;	// Alert!
					}
				}
			}
			catch (std::exception&)
			{
				err = EINVAL;
			}
		}
		else
		{
			// Cannot have a response to 0 thread!
			if (msg->m_type != Message::Request)
				err = EINVAL;
			else if (m_default_msg_queue.enqueue_tail(msg,msg->m_deadline==ACE_Time_Value::max_time ? 0 : &msg->m_deadline) == -1)
				err = ACE_OS::last_error();
			else if (m_consumers == 0)
			{
				// This incoming request may not be processed for some time...
				void* TICKET_91;	// Alert!
			}
		}

		if (err != 0)
		{
			delete msg;
			break;
		}
	}

	m_stream.close();

	return err;
}

int OOCore::UserSession::pump_requests(const ACE_Time_Value* deadline, bool bOnce)
{
	int ret = 0;

	// Increment the consumers...
	++m_consumers;

	ThreadContext* pContext = 0;
	pContext = ThreadContext::instance();
	if (!pContext)
        return -1;

	do
	{
		// Get the next message
		Message* msg;
		if (m_default_msg_queue.dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline)) == -1)
		{
			if (ACE_OS::last_error() == EWOULDBLOCK)
				ret = 1;
			else
				ret = -1;
			break;
		}

		// Set deadline
		ACE_Time_Value old_deadline = pContext->m_deadline;
		pContext->m_deadline = msg->m_deadline;

		// Dont confuse the wait deadline to the message processing deadline
		//if (deadline && *deadline < pContext->m_deadline)
		//	pContext->m_deadline = *deadline;

		try
		{
			// Set per channel thread id
			pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,msg->m_src_thread_id));
		}
		catch (std::exception&)
		{
			// This shouldn't ever occur, but that means it will ;)
			pContext->m_deadline = old_deadline;
			delete msg;
			continue;
		}

		// Process the message...
		process_request(msg,pContext->m_deadline);

		// Clear the thread map
		pContext->m_mapChannelThreads.clear();

		// Reset the deadline
		pContext->m_deadline = old_deadline;

		delete msg;

	} while (!bOnce);

	// Decrement the consumers...
	--m_consumers;

	return ret;
}

void OOCore::UserSession::process_channel_close(ACE_CDR::ULong closed_channel_id)
{
	// Close the corresponding Object Manager
	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();)
		{
			bool bErase = false;
			if (i->first == closed_channel_id)
			{
				// Close if its an exact match
				bErase = true;
			}
			else if ((i->first & 0xFFFFF000) == closed_channel_id)
			{
				// Close all apartment channels
				bErase = true;
			}
			else if (closed_channel_id == m_channel_id && ((i->first & 0xFFFFF000) != m_channel_id))
			{
				// If the user channel closes, close all upstream OMs
				bErase = true;
			}

			if (bErase)
			{
				i->second->disconnect();
				m_mapChannels.erase(i++);
			}
			else
				++i;
		}
	}
	catch (std::exception&)
	{}

	try
	{
		// Unblock all waiting threads
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::UShort,ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
		{
			if (i->second->m_usage > 0)
				i->second->m_msg_queue->pulse();
		}
	}
	catch (std::exception&)
	{}
}

bool OOCore::UserSession::wait_for_response(ACE_InputCDR*& response, ACE_CDR::ULong seq_no, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id)
{
	bool bRet = false;

	ThreadContext* pContext = 0;
	pContext = ThreadContext::instance();
	if (!pContext)
        return false;

	// Increment the usage count
	++pContext->m_usage;

	for (;;)
	{
		// Check if the channel we are waiting on is still valid...
		{
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
				break;

			std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(from_channel_id);
			if (i == m_mapChannels.end())
			{
				// Channel has gone!
				ACE_OS::last_error(ECONNRESET);
				break;
			}
		}

		// Get the next message
		Message* msg = 0;
		if (pContext->m_msg_queue->dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline)) == -1)
		{
			if (pContext->m_msg_queue->state() != ACE_Message_Queue_Base::PULSED)
				break;
		}
		else
		{
			if (msg->m_type == Message::Request)
			{
				// Update deadline
				ACE_Time_Value old_deadline = pContext->m_deadline;
				pContext->m_deadline = msg->m_deadline;
				if (deadline && *deadline < pContext->m_deadline)
					pContext->m_deadline = *deadline;

				// Set per channel thread id
				std::map<ACE_CDR::ULong,ACE_CDR::UShort>::iterator i;
				try
				{
					i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
					if (i == pContext->m_mapChannelThreads.end())
						i = pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::ULong,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,0)).first;
				}
				catch (std::exception&)
				{
					// This shouldn't ever occur, but that means it will ;)
					pContext->m_deadline = old_deadline;
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
			else if (msg->m_type == Message::Response && msg->m_seq_no == seq_no)
			{
				ACE_NEW_NORETURN(response,ACE_InputCDR(*msg->m_ptrPayload));
				if (response)
					bRet = true;

				delete msg;
				break;
			}

			delete msg;
		}
	}

	// Decrement the usage count
	--pContext->m_usage;

	return bRet;
}

OOCore::UserSession::ThreadContext* OOCore::UserSession::ThreadContext::instance()
{
	ThreadContext* pThis = ACE_TSS_Singleton<ThreadContext,ACE_Recursive_Thread_Mutex>::instance();
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
	m_bApartment(false),
	m_thread_id(0),
	m_msg_queue(0),
	m_usage(0),
	m_deadline(ACE_Time_Value::max_time),
	m_seq_no(0)
{
}

OOCore::UserSession::ThreadContext::~ThreadContext()
{
	if (m_thread_id)
		UserSession::USER_SESSION::instance()->remove_thread_context(m_thread_id);

	delete m_msg_queue;
}

// Accessors for ThreadContext
ACE_CDR::UShort OOCore::UserSession::insert_thread_context(OOCore::UserSession::ThreadContext* pContext)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	try
	{
		for (ACE_CDR::UShort i=1;i<=0xFFF;++i)
		{
			if (m_mapThreadContexts.find(i) == m_mapThreadContexts.end())
			{
				m_mapThreadContexts.insert(std::map<ACE_CDR::UShort,ThreadContext*>::value_type(i,pContext));
				return i;
			}
		}
	}
	catch (std::exception&)
	{}

	return 0;
}

void OOCore::UserSession::remove_thread_context(ACE_CDR::UShort thread_id)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(thread_id);
}

bool OOCore::UserSession::send_channel_close(ACE_CDR::ULong closed_channel_id)
{
	ACE_OutputCDR msg;
	msg << closed_channel_id;

	if (!msg.good_bit())
		return false;

	ACE_InputCDR* null;
	return send_request(m_channel_id & 0xFF000000,msg.begin(),null,0,Message::asynchronous | Message::channel_close);
}

bool OOCore::UserSession::send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::ULong timeout, ACE_CDR::ULong attribs)
{
	ACE_CDR::ULong src_channel_id = m_channel_id;
	ACE_CDR::UShort src_thread_id = 0;
	ACE_CDR::UShort dest_thread_id = 0;
	ACE_Time_Value deadline = ACE_Time_Value::max_time;

	ACE_CDR::ULong seq_no = 0;

	// Only use thread context if we are a synchronous call
	if (!(attribs & Message::asynchronous))
	{
		ThreadContext* pContext = ThreadContext::instance();

		// Determine dest_thread_id
		try
		{
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(dest_channel_id);
			if (i != pContext->m_mapChannelThreads.end())
				dest_thread_id = i->second;
		}
		catch (std::exception&)
		{
			return false;
		}

		src_thread_id = pContext->m_thread_id;
		deadline = pContext->m_deadline;

		// Set apartment
		if (pContext->m_bApartment)
			src_channel_id |= src_thread_id;

		while (!seq_no)
		{
			seq_no = ++pContext->m_seq_no;
		}
	}

	if (timeout > 0)
	{
		ACE_Time_Value deadline2 = ACE_OS::gettimeofday() + ACE_Time_Value(timeout / 1000,(timeout % 1000) * 1000);
		if (deadline2 < deadline)
			deadline = deadline2;
	}

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(seq_no,src_channel_id,src_thread_id,dest_channel_id,dest_thread_id,header,mb,deadline,Message::Request,attribs))
		return false;

	// Scope lock...
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_send_lock,false);

		// Send to the handle
		ACE_Time_Value wait = deadline;
		if (deadline != ACE_Time_Value::max_time)
		{
			ACE_Time_Value now = ACE_OS::gettimeofday();
			if (deadline <= now)
			{
				ACE_OS::last_error(ETIMEDOUT);
				return false;
			}

			wait = deadline - now;
		}

		size_t sent = 0;
		if (m_stream.send(header.begin(),wait != ACE_Time_Value::max_time ? &wait : 0,&sent) == -1)
			return false;

		if (sent != header.total_length())
			return false;
	}

	if (attribs & Remoting::Asynchronous)
		return true;
	else
		// Wait for response...
		return wait_for_response(response,seq_no,deadline != ACE_Time_Value::max_time ? &deadline : 0,dest_channel_id);
}

bool OOCore::UserSession::send_response(ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	const ThreadContext* pContext = ThreadContext::instance();
	
	// Set apartment
	ACE_CDR::ULong src_channel_id = m_channel_id;
	if (pContext->m_bApartment)
		src_channel_id |= pContext->m_thread_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(seq_no,src_channel_id,pContext->m_thread_id,dest_channel_id,dest_thread_id,header,mb,deadline,Message::Response,attribs))
		return false;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_send_lock,false);

	ACE_Time_Value wait = deadline;
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
		{
			ACE_OS::last_error(ETIMEDOUT);
			return false;
		}

		wait = deadline - now;
	}

	return (m_stream.send(header.begin(),wait != ACE_Time_Value::max_time ? &wait : 0) != -1);
}

namespace OOCore
{
	static bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point);
}

bool OOCore::ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
{
#if ACE_MAJOR_VERSION < 5 || (ACE_MAJOR_VERSION == 5 && (ACE_MINOR_VERSION < 5 || (ACE_MINOR_VERSION == 5 && ACE_BETA_VERSION == 0)))

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

bool OOCore::UserSession::build_header(ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort flags, ACE_CDR::ULong attribs)
{
	// Check the size
	if (mb && mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	 // version

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	header << dest_channel_id;
	header << src_channel_id;

	header.write_ulonglong(deadline.sec());
	header.write_long(deadline.usec());

	header << attribs;
	header << dest_thread_id;
	header << src_thread_id;
	header << seq_no;
	header << flags;

	if (!header.good_bit())
		return false;

	if (mb)
	{
		header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

		// Write the request stream
		header.write_octet_array_mb(mb);
		if (!header.good_bit())
			return false;
	}

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	return true;
}

Remoting::MarshalFlags_t OOCore::UserSession::classify_channel(ACE_CDR::ULong channel)
{
	Remoting::MarshalFlags_t mflags = Remoting::Same;
	if ((channel & 0xFF000000) == (m_channel_id & 0xFF000000))
		mflags = Remoting::InterProcess;
	else if ((channel & 0x80000000) == (m_channel_id & 0x80000000))
		mflags = Remoting::InterUser;
	else
		mflags = Remoting::RemoteMachine;

	return mflags;
}

void OOCore::UserSession::process_request(const UserSession::Message* pMsg, const ACE_Time_Value& deadline)
{
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(pMsg->m_src_channel_id,guid_t::Null());
		
		// Wrap up the request
		ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrEnvelope;
		ptrEnvelope = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
		ptrEnvelope->init(*pMsg->m_ptrPayload);

		// Unpack the payload
		IObject* pPayload = 0;
		ptrOM->UnmarshalInterface(L"payload",ptrEnvelope,OMEGA_UUIDOF(Remoting::IMessage),pPayload);
		ObjectPtr<Remoting::IMessage> ptrRequest;
		ptrRequest.Attach(static_cast<Remoting::IMessage*>(pPayload));

		// Check timeout
		uint32_t timeout = 0;
		if (deadline != ACE_Time_Value::max_time)
		{
			ACE_Time_Value now = ACE_OS::gettimeofday();
			if (deadline <= now)
			{
				ACE_OS::last_error(ETIMEDOUT);
				return;
			}
			timeout = (deadline - now).msec();
		}

		// Make the call
		ObjectPtr<Remoting::IMessage> ptrResult;
		ptrResult.Attach(ptrOM->Invoke(ptrRequest,timeout));

		if (!(pMsg->m_attribs & Remoting::Asynchronous))
		{
			// Wrap the response...
			ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
			ptrOM->MarshalInterface(L"payload",ptrResponse,OMEGA_UUIDOF(Remoting::IMessage),ptrResult);

			// Send it back...
			const ACE_Message_Block* mb = static_cast<const ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			if (!send_response(pMsg->m_seq_no,pMsg->m_src_channel_id,pMsg->m_src_thread_id,mb,deadline))
				ptrOM->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_UUIDOF(Remoting::IMessage),ptrResult);
		}
	}
	catch (IException* pOuter)
	{
		// Just drop the exception, and let it pass...
		pOuter->Release();
	}
}

ObjectPtr<Remoting::IObjectManager> OOCore::UserSession::get_channel_om(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = create_channel(src_channel_id,message_oid);
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());
	return ptrOM;
}

ObjectPtr<ObjectImpl<OOCore::Channel> > OOCore::UserSession::create_channel(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	try
	{
		// Lookup existing..
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(src_channel_id);
			if (i != m_mapChannels.end())
				return i->second;
		}

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
		ptrChannel->init(src_channel_id,classify_channel(src_channel_id),message_oid);

		// And add to the map
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator,bool> p = m_mapChannels.insert(std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::value_type(src_channel_id,ptrChannel));
		if (!p.second)
		{
			if (p.first->second->GetMarshalFlags() != ptrChannel->GetMarshalFlags())
				OMEGA_THROW(EINVAL);

			ptrChannel = p.first->second;
		}

		return ptrChannel;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

bool OOCore::UserSession::handle_request(uint32_t timeout)
{
	ACE_Time_Value wait(timeout/1000,(timeout % 1000) * 1000);

	ACE_Time_Value* wait2 = &wait;
	if (timeout == (uint32_t)0)
		wait2 = 0;
	else
		wait += ACE_OS::gettimeofday();

	int ret = USER_SESSION::instance()->pump_requests(wait2,true);
	if (ret == -1)
		OMEGA_THROW(ACE_OS::last_error());
	else
		return (ret == 0 ? false : true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,Omega_HandleRequest,1,((in),uint32_t,timeout))
{
	if (OOCore::HostedByOOServer())
		return OOCore::GetInterProcessService()->HandleRequest(timeout);
	else
		return OOCore::UserSession::handle_request(timeout);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Remoting::IChannelSink*,Remoting_OpenServerSink,2,((in),const Omega::guid_t&,message_oid,(in),Omega::Remoting::IChannelSink*,pSink))
{
	return OOCore::GetInterProcessService()->OpenServerSink(message_oid,pSink);
}
