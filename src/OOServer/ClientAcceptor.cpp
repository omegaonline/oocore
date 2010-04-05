///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#include "OOServer_Root.h"
#include "ClientAcceptor.h"
#include "RootManager.h"

#if defined(_WIN32)
#include <aclapi.h>
#endif

Root::ClientAcceptor::ClientAcceptor() :
	m_pManager(0),
	m_pSocket(0)
{
}

Root::ClientAcceptor::~ClientAcceptor()
{
	delete m_pSocket;
}

bool Root::ClientAcceptor::start(Manager* pManager)
{
#if defined(_WIN32)
	std::string pipe_name = "OOServer";
#elif defined(HAVE_UNISTD_H)
	std::string pipe_name = "/tmp/omegaonline/ooserverd";
#else
#error Fix me!
#endif

	assert(!m_pManager);
	m_pManager = pManager;

	if (!init_security(pipe_name))
		return false;

	int err = 0;
	m_pSocket = Proactor::instance()->accept_local(this,pipe_name,&err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name.c_str(),OOSvrBase::Logger::format_error(err).c_str()),false);

	return true;
}

void Root::ClientAcceptor::stop()
{
	if (m_pSocket)
		m_pSocket->close();
}

bool Root::ClientAcceptor::on_accept(OOBase::Socket* pSocket, int err)
{
	// Make sure we delete any socket passed to us
	OOBase::SmartPtr<OOBase::Socket> ptrSock = pSocket;

	if (err != 0)
		LOG_ERROR_RETURN(("Root::ClientAcceptor::on_accept received failure: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	// Read 4 bytes - This forces credential passing
	Omega::uint32_t v = 0;
	if (pSocket->recv(v) == 0)
	{
		OOBase::LocalSocket::uid_t uid = static_cast<OOBase::LocalSocket*>(pSocket)->get_uid();

		std::string strPipe = m_pManager->get_user_pipe(uid);
		if (!strPipe.empty())
		{
			Omega::uint32_t uLen = static_cast<Omega::uint32_t>(strPipe.length()+1);
			if (pSocket->send(uLen) == 0)
				pSocket->send(strPipe.c_str(),uLen);
		}

#if defined(_WIN32)
		CloseHandle(uid);
#endif
	}

	pSocket->close();
	return true;
}

bool Root::ClientAcceptor::init_security(const std::string& pipe_name)
{
#if defined(_WIN32)

	void* TODO; // Remove network service access from the pipe

	assert(!pipe_name.empty());

	// Get the current process' user SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrSIDProcess = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenUser));
	if (!ptrSIDProcess)
		LOG_ERROR_RETURN(("GetTokenInfo failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	const int NUM_ACES  = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES] = {0};

	// Set full control for the calling process SID
	ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[0].grfAccessMode = GRANT_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDProcess->User.Sid;

	// Create a SID for the BUILTIN\Users group.
	PSID pSID;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_USERS,
		0, 0, 0, 0, 0, 0,
		&pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	}
	OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor<void> > pSIDUsers(pSID);

	// Set read/write access
	ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[1].grfAccessMode = GRANT_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDUsers;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

#elif defined(HAVE_UNISTD_H)

	void* POSIX_TODO; // Set security on pipe_name

#else
#error Fix me!
#endif

	return true;
}
