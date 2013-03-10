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

Root::ClientConnection::ClientConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock),
		m_pid(0)
#if !defined(_WIN32)
		,m_uid(-1)
#endif
{
}

pid_t Root::ClientConnection::get_pid() const
{
	return m_pid;
}

const uid_t& Root::ClientConnection::get_uid() const
{
	return m_uid;
}

const char* Root::ClientConnection::get_session_id() const
{
	return m_session_id.c_str();
}

Root::ClientConnection::AutoDrop::~AutoDrop()
{
	if (m_id)
		m_pManager->drop_client(m_id);
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
	OOBase::Win32::sec_descript_t sd;
	DWORD dwErr = sd.SetEntriesInAcl(NUM_ACES,ea);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwErr)),false);

	// Create a new security descriptor
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd.descriptor();

	const char* pipe_name = "OmegaOnline";
	int err = 0;
	m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,pipe_name,err,&sa);
	if (err)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err)),false);

	return true;
}

bool Root::ClientConnection::start()
{
	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(128,m_socket,this,&ClientConnection::on_message_win32);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from client: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Root::ClientConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (!err)
	{
		// Get the connected uid
		HANDLE hPipe = (HANDLE)m_socket->get_handle();

		if (!ImpersonateNamedPipeClient(hPipe))
		{
			err = GetLastError();
			LOG_ERROR(("ImpersonateNamedPipeClient failed: %s",OOBase::system_error_text(err)));
		}
		else
		{
			BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&m_uid);
			if (!bRes)
				err = GetLastError();

			if (!RevertToSelf())
				OOBase_CallCriticalFailure(GetLastError());
	
			if (!bRes)
				LOG_ERROR(("OpenThreadToken failed: %s",OOBase::system_error_text(err)));
			else
			{
				HMODULE hKernel32 = ::GetModuleHandleW(L"Kernel32.dll");
				if (hKernel32)
				{
					typedef BOOL (WINAPI *pfn_GetNamedPipeClientProcessId)(HANDLE Pipe, PULONG ClientProcessId);

					pfn_GetNamedPipeClientProcessId pfn = (pfn_GetNamedPipeClientProcessId)(GetProcAddress(hKernel32,"GetNamedPipeClientProcessId"));

					if (!(*pfn)(hPipe,&m_pid))
					{
						err = GetLastError();
						LOG_ERROR(("GetNamedPipeClientProcessId failed: %s",OOBase::system_error_text(err)));
					}
				}
			}
		}

		if (err)
			return;
	}

	return on_message(stream,err);
}

#elif defined(HAVE_UNISTD_H)

bool Root::ClientConnection::start()
{
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(128,sizeof(size_t));
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

		for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
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
			return on_message(stream,0);
	}

	release();
}

bool Root::ClientConnection::send_response(OOBase::POSIX::SmartFD& fd, pid_t pid)
{
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	struct msghdr msg = {0};
	msg.msg_control = ctl_buffer->wr_ptr();
	msg.msg_controllen = ctl_buffer->space();

	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	*(int*)CMSG_DATA(cmsg) = fd;
	ctl_buffer->wr_ptr(cmsg->cmsg_len);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	stream.write(pid);

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_socket->send_msg(this,&ClientConnection::on_done,stream.buffer(),ctl_buffer);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}

	fd.detach();
	return true;
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

	SECURITY_ATTRIBUTES sa;
	sa.mode = 0666;
	sa.pass_credentials = true;

	int err = 0;
	m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,ROOT_NAME,err,&sa);
	if (err == EADDRINUSE)
	{
		::unlink(ROOT_NAME);
		m_client_acceptor = m_proactor->accept(this,&Manager::accept_client,ROOT_NAME,err,&sa);
	}

	if (err)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: %s",OOBase::system_error_text(err)),false);

	return true;
}


void Root::ClientConnection::on_done(OOBase::Buffer* data_buffer, OOBase::Buffer* ctl_buffer, int err)
{
	// Make sure we close all file handles...
	ctl_buffer->mark_rd_ptr(0);

	struct msghdr msgh = {0};
	msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
	msgh.msg_controllen = ctl_buffer->length();

	for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
	{
		if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
		{
			int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
			size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);

			for (size_t i=0;i<fd_count;++i)
				OOBase::POSIX::close(fds[i]);
		}
	}

	if (err)
		LOG_WARNING(("Failed to send user process information to client process: %s",OOBase::system_error_text(err)));

	m_pManager->drop_client(m_pid);

	release();
}

#endif // HAVE_UNISTD_H

#include "../../include/Omega/OOCore_version.h"

void Root::ClientConnection::on_message(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from client pipe: %s",OOBase::system_error_text(err)));
	else
	{
		// Check the versions are correct
		Omega::uint32_t version = 0;
		if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
			LOG_WARNING(("Unsupported version received: %u",version));
		else
		{
			pid_t pid = 0;
			if (!stream.read_string(m_session_id) || !stream.read(pid))
				LOG_ERROR(("Failed to retrieve client session id: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				if (!m_pid)
					m_pid = pid;

				m_pManager->connect_client(this);
			}
		}
	}

	release();
}

bool Root::Manager::get_client(pid_t id, OOBase::RefPtr<ClientConnection>& ptrClient)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	return m_clients.find(id,ptrClient);
}


void Root::Manager::drop_client(pid_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_clients.remove(id);
}
