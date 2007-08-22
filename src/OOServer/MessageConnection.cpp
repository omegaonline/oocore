/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./MessageConnection.h"

ACE_WString Root::MessagePipe::unique_name(const ACE_WString& strPrefix)
{
	ACE_Time_Value t = ACE_OS::gettimeofday();

	wchar_t szBuf[32];
	ACE_OS::sprintf(szBuf,L"%lx%lx",t.sec(),t.usec());

	return strPrefix + szBuf;
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)

#include <aclapi.h>

Root::MessagePipe::MessagePipe() :
	m_hRead(ACE_INVALID_HANDLE), m_hWrite(ACE_INVALID_HANDLE)
{
}

int Root::MessagePipe::connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait)
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
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"connector.connect() failed"),-1);

	countdown.update();

	ACE_SPIPE_Stream down;
	addr.string_to_addr((strAddr + L"\\down").c_str());
	if (connector.connect(down,addr,wait,ACE_Addr::sap_any,0,O_RDWR | FILE_FLAG_OVERLAPPED) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"connector.connect() failed"),-1);

	pipe.m_hRead = down.get_handle();
	pipe.m_hWrite = up.get_handle();

	up.set_handle(ACE_INVALID_HANDLE);
	down.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void Root::MessagePipe::close()
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

ACE_HANDLE Root::MessagePipe::get_read_handle() const
{
	return m_hRead;
}

bool Root::MessagePipe::operator < (const MessagePipe& rhs) const
{
	return (m_hRead < rhs.m_hRead || !(rhs.m_hRead < m_hRead) && m_hWrite < rhs.m_hWrite);
}

ssize_t Root::MessagePipe::send(const void* buf, size_t len, size_t* sent)
{
	return ACE_OS::write_n(m_hWrite,buf,len,sent);
}

ssize_t Root::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value*, size_t* sent)
{
	return ACE::write_n(m_hWrite,mb,sent);
}

ssize_t Root::MessagePipe::recv(void* buf, size_t len)
{
	ssize_t nRead = ACE_OS::read_n(m_hRead,buf,len);
	if (nRead == -1 && ACE_OS::last_error() == ERROR_MORE_DATA)
		nRead = static_cast<ssize_t>(len);

	return nRead;
}

Root::MessagePipeAcceptor::MessagePipeAcceptor()
{
	m_sa.lpSecurityDescriptor = NULL;
	m_sa.nLength = 0;
	m_pACL = NULL;
}

Root::MessagePipeAcceptor::~MessagePipeAcceptor()
{
	LocalFree(m_pACL);
	LocalFree(m_sa.lpSecurityDescriptor);
}

bool Root::MessagePipeAcceptor::CreateSA(HANDLE hToken, PSECURITY_DESCRIPTOR& pSD, PACL& pACL)
{
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

	const int NUM_ACES  = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	PSID pSIDUsers = NULL;
	PSID pSIDToken = NULL;
	if (!hToken)
	{
		// Create a SID for the Users group.
		if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_USERS,
			0, 0, 0, 0, 0, 0,
			&pSIDUsers))
		{
			return false;
		}

		// Set read access for Users.
		ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (LPWSTR) pSIDUsers;
	}
	else
	{
		// Get the SID for the token's user
		DWORD dwLen = 0;
		GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
		if (dwLen == 0)
			return false;

		TOKEN_USER* pBuffer = static_cast<TOKEN_USER*>(ACE_OS::malloc(dwLen));
		if (!pBuffer)
			return false;
		
		if (!GetTokenInformation(hToken,TokenUser,pBuffer,dwLen,&dwLen))
		{
			ACE_OS::free(pBuffer);
			return false;
		}

		dwLen = GetLengthSid(pBuffer->User.Sid);
		pSIDToken = static_cast<PSID>(ACE_OS::malloc(dwLen));
		if (!pSIDToken)
		{
			ACE_OS::free(pBuffer);
			return false;
		}

		if (!CopySid(dwLen,pSIDToken,pBuffer->User.Sid))
		{
			ACE_OS::free(pSIDToken);
			ACE_OS::free(pBuffer);
			return false;
		}

		ACE_OS::free(pBuffer);

		// Set read access for Specific user.
		ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[0].Trustee.ptstrName = (LPWSTR)pSIDToken;
	}

	// Create a SID for the BUILTIN\Administrators group.
	PSID pSIDAdmin = NULL;

	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSIDAdmin))
	{
		if (pSIDUsers)
			FreeSid(pSIDUsers);
		if (pSIDToken)
			ACE_OS::free(pSIDToken);
		return false;
	}

	// Set full control for Administrators.
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR) pSIDAdmin;

	if (ERROR_SUCCESS != SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL))
	{
		FreeSid(pSIDAdmin);
		if (pSIDUsers)
			FreeSid(pSIDUsers);
		if (pSIDToken)
			ACE_OS::free(pSIDToken);
		return false;
	}

	FreeSid(pSIDAdmin);
	if (pSIDUsers)
		FreeSid(pSIDUsers);
	if (pSIDToken)
		ACE_OS::free(pSIDToken);

	// Create a new security descriptor
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL) 
	{ 
		LocalFree(pACL);
		pACL = NULL;
		return false;
	}

	// Initialize a security descriptor. 
	if (!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
	{
		LocalFree(pSD);
		LocalFree(pACL);
		pACL = NULL;
		return false;
	} 

	// Add the ACL to the SD
	if (!SetSecurityDescriptorDacl(pSD,TRUE,pACL,FALSE))
	{	
		LocalFree(pSD);
		LocalFree(pACL);
		pACL = NULL;
		return false;
	} 

	return true;//(ERROR_SUCCESS == dwRes);
}

int Root::MessagePipeAcceptor::open(const ACE_WString& strAddr, HANDLE hToken)
{
	if (m_sa.nLength == 0)
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = FALSE;
		
		if (!CreateSA(hToken,m_sa.lpSecurityDescriptor,m_pACL))
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] Failed to create security descriptor: %x\n",GetLastError()),-1);
	}

	ACE_SPIPE_Addr addr;
	addr.string_to_addr((strAddr + L"\\up").c_str());
	if (m_acceptor_up.open(addr,1,ACE_DEFAULT_FILE_PERMS,&m_sa) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),-1);
	
	addr.string_to_addr((strAddr + L"\\down").c_str());
	if (m_acceptor_down.open(addr,1,ACE_DEFAULT_FILE_PERMS,&m_sa) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),-1);
	
	return 0;
}

int Root::MessagePipeAcceptor::accept(MessagePipe& pipe, ACE_Time_Value* timeout)
{
	ACE_Time_Value val(30);
	if (!timeout)
		timeout = &val;

	ACE_Countdown_Time countdown(timeout);

	ACE_SPIPE_Stream up;
	if (m_acceptor_up.accept(up,0,timeout) != 0 && GetLastError() != ERROR_MORE_DATA)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);

	countdown.update();

	ACE_SPIPE_Stream down;
	if (m_acceptor_down.accept(down,0,timeout) != 0 && GetLastError() != ERROR_MORE_DATA)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);

	pipe.m_hRead = up.get_handle();
	pipe.m_hWrite = down.get_handle();

	up.set_handle(ACE_INVALID_HANDLE);
	down.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

ACE_HANDLE Root::MessagePipeAcceptor::get_handle()
{
	return m_acceptor_up.get_handle();
}

void Root::MessagePipeAcceptor::close()
{
	m_acceptor_up.close();
	m_acceptor_down.close();
}

#else // defined(ACE_HAS_WIN32_NAMED_PIPES)

Root::MessagePipe::MessagePipe() :
	m_hSocket(ACE_INVALID_HANDLE)
{
}

int Root::MessagePipe::connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait)
{
	ACE_UNIX_Addr addr(strAddr.c_str());

	ACE_SOCK_Stream stream;
	if (ACE_SOCK_Connector().connect(stream,addr,wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"connector.connect() failed"),-1);

	pipe.m_hSocket = stream.get_handle();
	stream.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void Root::MessagePipe::close()
{
	ACE_HANDLE hSocket = m_hSocket;
	m_hSocket = ACE_INVALID_HANDLE;

	if (hSocket != ACE_INVALID_HANDLE)
		ACE_OS::close(hSocket);
}

ACE_HANDLE Root::MessagePipe::get_read_handle() const
{
	return hSocket;
}

bool Root::MessagePipe::operator < (const MessagePipe& rhs) const
{
	return (hSocket < rhs.hSocket);
}

ssize_t Root::MessagePipe::send(const void* buf, size_t len, size_t* sent)
{
	return ACE_OS::send(m_hSocket,(const char*)buf,len,sent);
}

ssize_t Root::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value* timeout, size_t* sent)
{
	return ACE::send_n(m_hSocket,mb,timeout,sent);
}

ssize_t Root::MessagePipe::recv(void* buf, size_t len)
{
	return ACE_OS::recv(m_hSocket,(char*)buf,len);
}

Root::MessagePipeAcceptor::MessagePipeAcceptor()
{
}

Root::MessagePipeAcceptor::~MessagePipeAcceptor()
{
}

int Root::MessagePipeAcceptor::open(const ACE_WString& strAddr)
{
	ACE_UNIX_Addr addr(strAddr.c_str());
	if (m_acceptor.open(strAddr) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),-1);

	return 0;
}

int Root::MessagePipeAcceptor::accept(MessagePipe& pipe, ACE_Time_Value* timeout, uid_t uid)
{
	// If uid==0 - it means everyone!
	if (uid == 0)
		uid = -1;
	
	ACE_SOCK_Stream stream;
	if (m_acceptor.accept(stream,0,timeout) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);

	pipe.m_hSocket = stream.get_handle();

	stream.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

ACE_HANDLE Root::MessagePipeAcceptor::get_handle()
{
	return m_acceptor.get_handle();
}

void Root::MessagePipeAcceptor::close()
{
	m_acceptor.close();

	ACE_OS::unlink(m_acceptor.get_addr());
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)

ACE_CDR::UShort Root::MessageConnection::open(MessagePipe& pipe)
{
	m_pipe = pipe;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (m_reader.open(*this,pipe.get_read_handle()) != 0 && GetLastError() != ERROR_MORE_DATA)
#else
	if (m_reader.open(*this,pipe.get_read_handle()) != 0)
#endif
	{
	    ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"reader.open() failed"),0);
	}

	ACE_CDR::UShort uId = m_pHandler->register_channel(pipe);
	if (uId == 0)
		return 0;

	if (!read())
		return 0;

	return uId;
}

bool Root::MessageConnection::read()
{
	ACE_Message_Block* mb = 0;
	ACE_NEW_RETURN(mb,ACE_Message_Block(2048),false);

	// Align the message block for CDR
	ACE_CDR::mb_align(mb);

	// We read the header first
	m_read_len = 0;

	// Start an async read
	if (m_reader.read(*mb,s_initial_read) != 0)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"read() failed"));
		mb->release();
		return false;
	}

	return true;
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
void Root::MessageConnection::handle_read_file(const ACE_Asynch_Read_File::Result& result)
#else
void Root::MessageConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
#endif
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = false;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (result.success() || result.error() == ERROR_MORE_DATA)
#else
	if (result.success())
#endif
	{
		if (m_read_len == 0)
		{
			if (result.bytes_transferred() == s_initial_read)
			{
				// Create a temp input CDR
				ACE_InputCDR input(mb.data_block(),ACE_Message_Block::DONT_DELETE,static_cast<size_t>(mb.rd_ptr() - mb.base()),static_cast<size_t>(mb.wr_ptr() - mb.base()));
				input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

				// Read and set the byte order
				if (input.read_octet(m_byte_order) && input.read_octet(m_version))
				{
					input.reset_byte_order(m_byte_order);

					// Read the length
					input >> m_read_len;
					if (input.good_bit())
					{
						// Resize the message block
						if (mb.size(m_read_len) == 0)
						{
							// Subtract what we have already read
							m_read_len -= static_cast<ACE_CDR::ULong>(mb.length());

							mb.rd_ptr(s_initial_read);

							if (m_reader.read(mb,m_read_len) == 0)
								return;
							else
								ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"reader.read() failed"));
						}
					}
					else
						ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));
				}
				else
					ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));
			}
		}
		else
		{
			if (result.bytes_transferred() < m_read_len)
			{
				m_read_len -= result.bytes_transferred();
				if (m_reader.read(mb,m_read_len) == 0)
					return;
			}
			else if (result.bytes_transferred() == m_read_len)
			{
				// Create a new input CDR wrapping mb
				ACE_InputCDR* input = 0;
				ACE_NEW_NORETURN(input,ACE_InputCDR(&mb,m_byte_order));
				if (input)
				{
					// Read in the message info
					MessageHandler::Message* msg = 0;
					ACE_NEW_NORETURN(msg,MessageHandler::Message);
					if (msg)
					{
						msg->m_pipe = m_pipe;

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

						#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
							input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
						#endif

						if (input->good_bit())
						{
							// Push into the HandlerBase queue...
							if (m_pHandler->parse_message(msg))
							{
								// We want to keep msg and input alive
								msg = 0;
								input = 0;
							}

							// Start a new read
							bSuccess = read();
						}
						else
							ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Corrupt header\n"));

						delete msg;
					}
					delete input;
				}
			}
			else
				ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Over-read?\n"));
		}
	}

	mb.release();

	if (!bSuccess)
	{
		int err = ACE_OS::last_error();
		if (err != 0 && err != ENOTSOCK && err != ERROR_BROKEN_PIPE)
			ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"handle_read_*() failed"));

		m_pHandler->pipe_closed(m_pipe);
		delete this;
	}
}

Root::MessageConnection* Root::MessageHandler::make_handler()
{
	MessageConnection* handler = 0;
	ACE_NEW_RETURN(handler,MessageConnection(this),0);
	return handler;
}

Root::MessageHandler::MessageHandler() :
	m_uNextChannelId(0)
{
}

ACE_CDR::UShort Root::MessageHandler::register_channel(MessagePipe& pipe)
{
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	ACE_CDR::UShort uChannelId = 0;
	try
	{
		ACE_Thread_Mutex* pLock = 0;
		ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
		ChannelInfo channel(pipe,0,pLock);

		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}
		m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(uChannelId,channel));

		std::map<ACE_CDR::UShort,ACE_CDR::UShort> reverse_map;
		reverse_map.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(0,uChannelId));

		m_mapReverseChannelIds.insert(std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(pipe,reverse_map));
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
	}

	return uChannelId;
}

ACE_CDR::UShort Root::MessageHandler::add_routing(ACE_CDR::UShort dest_channel, ACE_CDR::UShort dest_route)
{
	// Add a new channel that routes dest_channel's dest_route correctly...

	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	ACE_CDR::UShort uChannelId = 0;
	try
	{
		// Find the handle for dest_channel
		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel);
		if (i==m_mapChannelIds.end())
			return 0;

		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(i->second.pipe);
		if (j==m_mapReverseChannelIds.end())
			return 0;

		ACE_Thread_Mutex* pLock = 0;
		ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
		ChannelInfo channel(i->second.pipe, dest_route, pLock);

		uChannelId = ++m_uNextChannelId;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = ++m_uNextChannelId;
		}

		std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(dest_channel,uChannelId));
		if (!p.second)
			uChannelId = p.first->second;
		else
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(uChannelId,channel));
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
	}

	return uChannelId;
}

ACE_CDR::UShort Root::MessageHandler::get_pipe_channel(const MessagePipe& pipe, ACE_CDR::UShort channel)
{
	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,0);

	try
	{
		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::const_iterator j=m_mapReverseChannelIds.find(pipe);
		if (j!=m_mapReverseChannelIds.end())
		{
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::const_iterator i=j->second.find(channel);
			if (i!=j->second.end())
				return i->second;
		}
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),0);
	}

	return 0;
}

Root::MessagePipe Root::MessageHandler::get_channel_pipe(ACE_CDR::UShort channel)
{
	static Root::MessagePipe ret;

	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,ret);

	try
	{
		std::map<ACE_CDR::UShort,ChannelInfo>::const_iterator i=m_mapChannelIds.find(channel);
		if (i != m_mapChannelIds.end())
			return i->second.pipe;
	}
	catch (...)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"));
	}

	return ret;
}

void Root::MessageHandler::pipe_closed(const MessagePipe& pipe)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(pipe);
		if (j!=m_mapReverseChannelIds.end())
		{
			for (std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k=j->second.begin();k!=j->second.end();++k)
			{
				m_mapChannelIds.erase(k->second);
			}
			m_mapReverseChannelIds.erase(j);
		}
	}
	catch (...)
	{}
}

bool Root::MessageHandler::parse_message(Message* msg)
{
	ChannelInfo dest_channel;

	// Update the source, so we can send it back the right way...
	try
	{
		bool bFound = false;
		ACE_CDR::UShort reply_channel_id = 0;
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			if (msg->m_dest_channel_id != 0)
			{
				std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(msg->m_dest_channel_id);
				if (i == m_mapChannelIds.end())
					return false;

				dest_channel = i->second;
			}

			// Find the local channel id that matches src_channel_id
			std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_pipe);
			if (j==m_mapReverseChannelIds.end())
				return false;

			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(msg->m_src_channel_id);
			if (k != j->second.end())
			{
				bFound = true;
				reply_channel_id = k->second;
			}
		}

		if (!bFound)
		{
			ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(msg->m_pipe);
			if (j==m_mapReverseChannelIds.end())
				return false;

			ACE_Thread_Mutex* pLock = 0;
			ACE_NEW_RETURN(pLock,ACE_Thread_Mutex,0);
			ChannelInfo channel(msg->m_pipe, msg->m_src_channel_id, pLock);

			reply_channel_id = ++m_uNextChannelId;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = ++m_uNextChannelId;
			}

			std::pair<std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator,bool> p = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,reply_channel_id));
			if (!p.second)
				reply_channel_id = p.first->second;
			else
				m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelInfo>::value_type(reply_channel_id,channel));
		}

		msg->m_src_channel_id = reply_channel_id;
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),false);
	}

	if (msg->m_dest_channel_id == 0)
	{
		if (msg->m_dest_thread_id != 0)
		{
			// Find the right queue to send it to...
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			std::map<ACE_CDR::UShort,const ThreadContext*>::const_iterator i=m_mapThreadContexts.find(msg->m_dest_thread_id);
			if (i == m_mapThreadContexts.end())
				return false;

			return (i->second->m_msg_queue->enqueue_tail(msg,&msg->m_deadline) != -1);
		}
		else
		{
			return (m_default_msg_queue.enqueue_tail(msg,&msg->m_deadline) != -1);
		}
	}
	else
	{
		// Forward it...
		msg->m_dest_channel_id = dest_channel.channel_id;

		ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
		if (build_header(header,*msg,msg->m_pPayload->start()))
		{
            // Send to the handle
			ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

			ACE_Time_Value wait = msg->m_deadline - ACE_OS::gettimeofday();
			dest_channel.pipe.send(header.begin(),&wait);
		}

		// We are done with the message...
		return false;
	}
}

Root::MessageHandler::ThreadContext* Root::MessageHandler::ThreadContext::instance(Root::MessageHandler* pHandler)
{
	ThreadContext* pThis = ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>::instance();
	if (pThis->m_thread_id == 0)
	{
		ACE_NEW_NORETURN(pThis->m_msg_queue,(ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>));
		pThis->m_thread_id = pHandler->insert_thread_context(pThis);
		pThis->m_pHandler = pHandler;
	}

	if (pThis->m_thread_id == 0)
		return 0;
	else
		return pThis;
}

Root::MessageHandler::ThreadContext::ThreadContext() :
	m_thread_id(0),
	m_msg_queue(0),
	m_deadline(ACE_Time_Value::max_time),
	m_pHandler(0)
{
}

Root::MessageHandler::ThreadContext::~ThreadContext()
{
	if (m_pHandler)
		m_pHandler->remove_thread_context(this);
	delete m_msg_queue;
}

// Accessors for ThreadContext
ACE_CDR::UShort Root::MessageHandler::insert_thread_context(const Root::MessageHandler::ThreadContext* pContext)
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

void Root::MessageHandler::remove_thread_context(const Root::MessageHandler::ThreadContext* pContext)
{
	ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapThreadContexts.erase(pContext->m_thread_id);
}

int Root::MessageHandler::MessageConnector::start(MessageHandler* pManager, const ACE_WString& strAddr)
{
	m_pParent = pManager;

	if (m_acceptor.open(strAddr,0) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),-1);

	if (ACE_Reactor::instance()->register_handler(this,m_acceptor.get_handle()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"register_handler() failed"),-1);

	return 0;
}

int Root::MessageHandler::MessageConnector::handle_signal(int, siginfo_t*, ucontext_t*)
{
	MessagePipe pipe;
	if (m_acceptor.accept(pipe) != 0 && GetLastError() != ERROR_MORE_DATA)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);

	Root::MessageConnection* pMC = 0;
	ACE_NEW_RETURN(pMC,Root::MessageConnection(m_pParent),-1);

	if (pMC->open(pipe) == 0)
	{
		delete pMC;
		return -1;
	}

	return 0;
}

void Root::MessageHandler::MessageConnector::stop()
{
	ACE_Reactor::instance()->remove_handler(m_acceptor.get_handle(),ACE_Event_Handler::ALL_EVENTS_MASK);

    m_acceptor.close();
}

int Root::MessageHandler::start(const ACE_WString& strName)
{
	return m_connector.start(this,strName);
}

void Root::MessageHandler::stop_accepting()
{
	m_connector.stop();
}

void Root::MessageHandler::stop()
{
	ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	for (std::map<ACE_CDR::UShort,const ThreadContext*>::iterator i=m_mapThreadContexts.begin();i!=m_mapThreadContexts.end();++i)
	{
		i->second->m_msg_queue->close();
	}

	m_default_msg_queue.close();
}

bool Root::MessageHandler::wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance(this);
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = pContext->m_msg_queue->dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return false;

		if (msg->m_bIsRequest)
		{
			// Update deadline
			ACE_Time_Value old_deadline = pContext->m_deadline;
			pContext->m_deadline = (msg->m_deadline < pContext->m_deadline ? msg->m_deadline : pContext->m_deadline);

			ACE_CDR::UShort old_thread_id = 0;
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator i=pContext->m_mapChannelThreads.find(msg->m_src_channel_id);
			if (i != pContext->m_mapChannelThreads.end())
				i = pContext->m_mapChannelThreads.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(msg->m_src_channel_id,0)).first;

			old_thread_id = i->second;
			i->second = msg->m_src_thread_id;

			// Process the message...
			process_request(msg->m_pipe,*msg->m_pPayload,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);

			// Restore old context
			pContext->m_deadline = old_deadline;
			i->second = old_thread_id;
		}
		else
		{
			response = msg->m_pPayload;
			msg->m_pPayload = 0;
			delete msg;
			return true;
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

void Root::MessageHandler::pump_requests(const ACE_Time_Value* deadline)
{
	ThreadContext* pContext = ThreadContext::instance(this);
	for (;;)
	{
		// Get the next message
		Message* msg;
		int ret = m_default_msg_queue.dequeue_head(msg,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return;

		if (msg->m_bIsRequest)
		{
			// Update deadline
			pContext->m_deadline = msg->m_deadline;
			if (deadline && *deadline < pContext->m_deadline)
				pContext->m_deadline = *deadline;

			// Process the message...
			process_request(msg->m_pipe,*msg->m_pPayload,msg->m_src_channel_id,msg->m_src_thread_id,pContext->m_deadline,msg->m_attribs);
		}

		delete msg->m_pPayload;
		delete msg;
	}
}

bool Root::MessageHandler::send_request(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_channel_id = dest_channel_id;
	msg.m_dest_thread_id = 0;
	std::map<ACE_CDR::UShort,ACE_CDR::UShort>::const_iterator i=pContext->m_mapChannelThreads.find(pContext->m_thread_id);
	if (i != pContext->m_mapChannelThreads.end())
		msg.m_dest_thread_id = i->second;

	msg.m_src_channel_id = 0;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_bIsRequest = true;
	msg.m_deadline = pContext->m_deadline;
	ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000);
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return false;

		dest_channel = i->second;
	}
	catch (...)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"),false);
	}

	// Remap the destination channel...
	msg.m_dest_channel_id = dest_channel.channel_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,msg,mb))
		return false;

	// Check the timeout
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (msg.m_deadline <= now)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,*dest_channel.lock,false);

		size_t sent = 0;
		ACE_Time_Value wait = msg.m_deadline - now;
		if (dest_channel.pipe.send(header.begin(),&wait,&sent) == -1)
			return false;

		if (sent != header.total_length())
			return false;
	}

	if (attribs & 1)
		return true;
	else
		// Wait for response...
		return wait_for_response(response,&msg.m_deadline);
}

void Root::MessageHandler::send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs)
{
	const ThreadContext* pContext = ThreadContext::instance(this);

	// Build a header
	Message msg;
	msg.m_dest_channel_id = dest_channel_id;
	msg.m_dest_thread_id = dest_thread_id;
	msg.m_src_channel_id = 0;
	msg.m_src_thread_id = pContext->m_thread_id;
	msg.m_attribs = attribs;
	msg.m_bIsRequest = false;
	msg.m_deadline = pContext->m_deadline;
	if (deadline < msg.m_deadline)
		msg.m_deadline = deadline;

	ChannelInfo dest_channel;
	try
	{
		ACE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::UShort,ChannelInfo>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return;

		dest_channel = i->second;
	}
	catch (...)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] exception thrown\n"));
		return;
	}

	// Remap the destination channel...
	msg.m_dest_channel_id = dest_channel.channel_id;

	// Write the header info
	ACE_OutputCDR header(ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(header,msg,mb))
		return;

	// Check the timeout
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (msg.m_deadline <= now)
		return;

	ACE_GUARD(ACE_Thread_Mutex,guard,*dest_channel.lock);

	ACE_Time_Value wait = msg.m_deadline - now;
	dest_channel.pipe.send(header.begin(),&wait);
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

bool Root::MessageHandler::build_header(ACE_OutputCDR& header, const Message& msg, const ACE_Message_Block* mb)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] Message too big!\n"),false);

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

	// We have room for 2 bytes here!

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write the message
	header.write_ushort(msg.m_dest_channel_id);
	header.write_ushort(msg.m_dest_thread_id);
	header.write_ushort(msg.m_src_channel_id);
	header.write_ushort(msg.m_src_thread_id);

	header.write_ulong(static_cast<const timeval*>(msg.m_deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(msg.m_deadline)->tv_usec);

	header.write_ushort(msg.m_attribs);

	header.write_boolean(msg.m_bIsRequest);

	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

#if !defined (ACE_CDR_IGNORE_ALIGNMENT)
	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
#endif

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR write() failed"),false);

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"CDR replace() failed"),false);

	return true;
}