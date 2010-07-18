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

#include "OOServer_User.h"
#include "UserAcceptor.h"
#include "UserManager.h"

#if defined(_WIN32)
#include <aclapi.h>
#include <sddl.h>
#endif

User::Acceptor::Acceptor() :
		m_pManager(0),
		m_pSocket(0)
{
}

User::Acceptor::~Acceptor()
{
	delete m_pSocket;
}

std::string User::Acceptor::unique_name()
{
	// Create a new unique pipe
	std::ostringstream ssPipe;
	ssPipe.imbue(std::locale::classic());
	ssPipe.setf(std::ios_base::hex,std::ios_base::basefield);

#if defined(_WIN32)
	ssPipe << "OOU";

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),"");

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),"");

	char* pszSid;
	if (ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
	{
		ssPipe << pszSid;
		LocalFree(pszSid);
	}
#elif defined(HAVE_UNISTD_H)

	ssPipe << "/tmp/oo-" << getuid() << "-" << getpid();

#else
#error Fix me!
#endif

	// Add the current time...
	ssPipe << "-" << OOBase::gettimeofday().tv_usec();

	return ssPipe.str();
}

bool User::Acceptor::start(Manager* pManager, const std::string& pipe_name)
{
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

void User::Acceptor::stop()
{
	if (m_pSocket)
		m_pSocket->close();
}

bool User::Acceptor::on_accept(OOBase::Socket* pSocket, int err)
{
	// Make sure we delete any socket passed to us
	OOBase::SmartPtr<OOBase::Socket> ptrSock = pSocket;
	
	if (err != 0)
		LOG_ERROR_RETURN(("User::Acceptor::on_accept: accept failure: %s",OOSvrBase::Logger::format_error(err).c_str()),false);

	err = pSocket->close_on_exec();
	if (err != 0)
	{
		LOG_WARNING(("User::Acceptor::on_accept: close_on_exec failure: %s",OOSvrBase::Logger::format_error(err).c_str()));
		pSocket->close();
		return true;
	}

	// Read 4 bytes - This forces credential passing
	Omega::uint32_t version = 0;
	err = pSocket->recv(version);
	if (err != 0)
	{
		LOG_WARNING(("User::Acceptor::on_accept: receive failure: %s",OOSvrBase::Logger::format_error(err).c_str()));
		pSocket->close();
		return true;
	}

	// Check the versions are correct
	if (version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
	{
		LOG_WARNING(("User::Acceptor::on_accept: version received too early: %u",version));
		pSocket->close();
		return true;
	}

	// Check to see if the connection came from a process with our uid
	OOSvrBase::AsyncLocalSocket::uid_t uid = static_cast<OOBase::LocalSocket*>(pSocket)->get_uid();
	bool bOk = false;

#if defined(_WIN32)

	// Make sure the handle is closed
	OOBase::Win32::SmartHandle hUidToken(uid);

	// Get our Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
	{
		err = GetLastError();
		pSocket->close();
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage(err).c_str()),true);
	}

	// Check the SIDs and priviledges are the same...
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(uid,TokenGroupsAndPrivileges));
	if (!pStats1)
	{
		err = GetLastError();
		pSocket->close();
		LOG_ERROR_RETURN(("OOSvrBase::Win32::GetTokenInfo failed: %s",OOBase::Win32::FormatMessage(err).c_str()),true);
	}

	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenGroupsAndPrivileges));
	if (!pStats2)
	{
		err = GetLastError();
		pSocket->close();
		LOG_ERROR_RETURN(("OOSvrBase::Win32::GetTokenInfo failed: %s",OOBase::Win32::FormatMessage(err).c_str()),true);
	}

	// Compare...
	bOk = (pStats1->SidCount==pStats2->SidCount &&
			pStats1->RestrictedSidCount==pStats2->RestrictedSidCount &&
			pStats1->PrivilegeCount==pStats2->PrivilegeCount &&
			OOSvrBase::Win32::MatchSids(pStats1->SidCount,pStats1->Sids,pStats2->Sids) &&
			OOSvrBase::Win32::MatchSids(pStats1->RestrictedSidCount,pStats1->RestrictedSids,pStats2->RestrictedSids) &&
			OOSvrBase::Win32::MatchPrivileges(pStats1->PrivilegeCount,pStats1->Privileges,pStats2->Privileges));

#elif defined(HAVE_UNISTD_H)

	bOk = (getuid() == uid);

#else
#error Fix me!
#endif

	if (!bOk)
	{
		LOG_WARNING(("User::Acceptor::on_accept: attempt to connect by invalid user"));
		pSocket->close();
		return true;
	}

	if (!m_pManager->on_accept(pSocket))
		pSocket->close();

	// Keep accepting, whatever...
	return true;
}

bool User::Acceptor::init_security(const std::string& pipe_name)
{
#if defined(_WIN32)

	OMEGA_UNUSED_ARG(pipe_name);

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);

	const int NUM_ACES = 1;
	EXPLICIT_ACCESSW ea[NUM_ACES] = {0};

	// Set full control for the Logon SID
	ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDLogon;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

#elif defined(HAVE_UNISTD_H)

	m_sa.mode = 0700;

#else
#error set security on pipe_name
#endif

	return true;
}
