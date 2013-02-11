///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

namespace Root
{
	class ClientConnection : public OOBase::RefCounted
	{
	public:
		ClientConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock);

		bool start();

	private:
		Manager*                            m_pManager;
		OOBase::RefPtr<OOBase::AsyncSocket> m_socket;

		pid_t  m_pid;
		uid_t  m_uid;

#if defined(HAVE_UNISTD_H)
		void on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err);
#endif

		void on_message(OOBase::CDRStream& stream, int err);
	};
}

Root::ClientConnection::ClientConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock),
		m_pid(0)
#if !defined(_WIN32)
		,m_uid(-1)
#endif
{
}

#if defined(_WIN32)

bool Root::Manager::start_client_acceptor(OOBase::AllocatorInstance& allocator)
{
	const int NUM_ACES = 3;
	EXPLICIT_ACCESSW ea[NUM_ACES] = { {0}, {0}, {0} };

	PSID pSID;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = {SECURITY_NT_AUTHORITY};
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
								  SECURITY_LOCAL_SYSTEM_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}

	OOBase::LocalPtr<void,OOBase::Win32::SIDDestructor> pSIDSystem(pSID);

	// Set full control for the creating process SID
	ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pSIDSystem;  // Don't use CREATOR/OWNER, it doesn't work with multiple pipe instances...

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::TempPtr<void> ptrSIDLogon(allocator);
	if (OOBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon) == ERROR_SUCCESS)
	{
		// Use logon sid instead...
		ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDLogon;  // Don't use CREATOR/OWNER, it doesn't work with multiple pipe instances...
	}

	// Create a SID for the EVERYONE group.
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = {SECURITY_WORLD_SID_AUTHORITY};
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
								  SECURITY_WORLD_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}
	OOBase::LocalPtr<void,OOBase::Win32::SIDDestructor> pSIDEveryone(pSID);

	// Set read/write access for EVERYONE
	ea[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_WRITE_DATA;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDEveryone;

	// Create a SID for the Network group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
								  SECURITY_NETWORK_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}
	OOBase::LocalPtr<void,OOBase::Win32::SIDDestructor> pSIDNetwork(pSID);

	// Deny all to NETWORK
	ea[2].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[2].grfAccessMode = DENY_ACCESS;
	ea[2].grfInheritance = NO_INHERITANCE;
	ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[2].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[2].Trustee.ptstrName = (LPWSTR)pSIDNetwork;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwErr)),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(m_sa);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

	const char* pipe_name = "OmegaOnline";
	int err = 0;
	m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,pipe_name,err,&m_sa);
	if (err)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err)),false);

	return true;
}

bool Root::ClientConnection::start()
{
	// Get the connected uid
	HANDLE hPipe = (HANDLE)m_socket->get_handle();

	if (!ImpersonateNamedPipeClient(hPipe))
		LOG_ERROR_RETURN(("ImpersonateNamedPipeClient failed: %s",OOBase::system_error_text()),false);

	BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&m_uid);
	int err = 0;
	if (!bRes)
		err = GetLastError();

	if (!RevertToSelf())
	{
		OOBase_CallCriticalFailure(GetLastError());
		abort();
	}

	if (!bRes)
		LOG_ERROR_RETURN(("OpenThreadToken failed: %s",OOBase::system_error_text(err)),false);
_WIN32_WINNT
	HMODULE hKernel32 = ::GetModuleHandleW(L"Kernel32.dll");
	if (hKernel32)
	{
		typedef BOOL (WINAPI *pfn_GetNamedPipeClientProcessId)(HANDLE Pipe, PULONG ClientProcessId);

		pfn_GetNamedPipeClientProcessId pfn = (pfn_GetNamedPipeClientProcessId)(GetProcAddress(hKernel32,"GetNamedPipeClientProcessId"));

		if (!(*pfn)(hPipe,&m_pid))
			LOG_ERROR_RETURN(("GetNamedPipeClientProcessId failed: %s",OOBase::system_error_text()),false);
	}

	addref();

	err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(128,m_socket,this,&ClientConnection::on_message);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from client: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

#elif defined(HAVE_UNISTD_H)

bool Root::ClientConnection::start()
{
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<Omega::uint16_t>(128,m_socket,this,&ClientConnection::on_message_posix,ctl_buffer);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from client: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Root::ClientConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from client pipe: %s",OOBase::system_error_text(err)));
	else
	{
		bool bHaveCreds = false;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg && !err;CMSG_NXTHDR(&msgh,msg))
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				// Close any file handles that arrive
				int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
				size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);

				for (size_t i=0;i<fd_count;++i)
					OOBase::POSIX::close(fds[i]);
			}
#if defined(SCM_CREDENTIALS)
			else if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_CREDENTIALS)
			{
				struct ucred* uc = reinterpret_cast<struct ucred*>(CMSG_DATA(msg));
				m_uid = uc->uid;
				m_pid = uc->pid;
				bHaveCreds = true;
			}
#elif defined(SCM_CREDS)
			else if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_CREDS)
			{
#if defined(HAVE_STRUCT_CMSGCRED)
				struct cmsgcred* cred = reinterpret_cast<struct cmsgcred*>(CMSG_DATA(msg));
				m_uid = cred->cmcred_uid;
				m_pid = cred->cmcred->pid;
				bHaveCreds = true;
#elif defined(HAVE_STRUCT_FCRED)
				struct fcred* cred = reinterpret_cast<struct fcred*>(CMSG_DATA(msg));
				m_uid = cred->fc_uid;
				m_pid = cred->fc_pid;
				bHaveCreds = true;
#elif defined(HAVE_STRUCT_SOCKCRED)
				struct sockcred* cred = reinterpret_cast<struct sockcred*>(CMSG_DATA(msg));
				m_uid = cred->sc_uid;
				m_pid = cred->sc_pid;
				bHaveCreds = true;
#else
#error Implement credential passing!
#endif
			}
#endif
			else
			{
				LOG_ERROR(("Client pipe control data has weird stuff in it"));
				err = EINVAL;
			}
		}

		if (!bHaveCreds)
		{
			LOG_ERROR(("Client pipe has no credentials"));
			err = EINVAL;
		}

		if (!err)
			on_message(stream,0);
	}

	release();
}

bool Root::Manager::start_client_acceptor(OOBase::AllocatorInstance&)
{
#if defined(__linux__)
	#define ROOT_NAME "\0/org/omegaonline"
#elif defined(P_tmpdir)
	#define ROOT_NAME P_tmpdir "/omegaonline"
#else
	#define ROOT_NAME "/tmp/omegaonline"
#endif

	m_sa.mode = 0666;
	m_sa.pass_credentials = true;

	int err = 0;
	m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,ROOT_NAME,err,&m_sa);
	if (err == EADDRINUSE)
	{
		::unlink(ROOT_NAME);
		m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,ROOT_NAME,err,&m_sa);
	}

	if (err)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: %s",OOBase::system_error_text(err)),false);

	return true;
}

#endif // HAVE_UNISTD_H

void Root::Manager::accept_client(void* pThis, OOBase::AsyncSocket* pSocket, int err)
{
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = pSocket;

	if (err)
		LOG_ERROR(("Client acceptor failed: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::RefPtr<Root::ClientConnection> ptrConn = new (std::nothrow) Root::ClientConnection(static_cast<Manager*>(pThis),ptrSocket);
		if (!ptrConn)
			LOG_ERROR(("Failed to allocate client connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)));
		else
			ptrConn->start();
	}
}

#include "../../include/Omega/OOCore_version.h"

void Root::ClientConnection::on_message(OOBase::CDRStream& stream, int err)
{
	// Check the versions are correct
	Omega::uint32_t version = 0;
	if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
		LOG_WARNING(("Unsupported version received: %u",version));
	else
	{
		OOBase::StackAllocator<256> allocator;
		OOBase::LocalString strSid(allocator);
		if (!stream.read_string(strSid))
			LOG_ERROR(("Failed to retrieve client session id: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
#if defined(_WIN32)
			// strSid is actually the PID of the child process
			if (!m_pid)
				m_pid = strtoul(strSid.c_str(),NULL,10);

			strSid.clear();
#endif
			m_pManager->find_user_process(this,m_uid,m_pid,strSid);
		}
	}
}


/*
			UserProcess user_process;
			if (m_pManager->get_user_process(uid,strSid,user_process))
			{
				if (!stream.write_string(user_process.m_strPipe))
					LOG_ERROR(("Failed to write to client: %s",OOBase::system_error_text(stream.last_error())));
				else
					ptrSocket->send(stream.buffer());
			} */

