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
		m_pManager(0)
{
}

bool User::Acceptor::unique_name(OOBase::LocalString& name)
{
	// Create a new unique pipe

#if defined(_WIN32)
	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

	char* pszSid;
	if (!ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
		LOG_ERROR_RETURN(("ConvertSidToStringSidA failed: %s",OOBase::system_error_text()),false);
	
	int err = name.printf("OOU%s-%ld",pszSid,GetCurrentProcessId());

	LocalFree(pszSid);
		
#elif defined(HAVE_UNISTD_H)

	int err = name.printf("/tmp/oo-%d-%d",getuid(),getpid());

#else
#error Fix me!
#endif

	if (err != 0)
		LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err)),false);		

	return true;
}

bool User::Acceptor::start(Manager* pManager, const char* pipe_name)
{
	assert(!m_pManager);
	m_pManager = pManager;

	if (!init_security())
		return false;

	int err = 0;
	m_pSocket = Proactor::instance().accept_local(this,pipe_name,&err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err)),false);

	return true;
}

void User::Acceptor::stop()
{
	m_pSocket = 0;
}

bool User::Acceptor::on_accept(OOSvrBase::AsyncLocalSocketPtr ptrSocket, const char* /*strAddress*/, int err)
{
	if (err != 0)
		LOG_ERROR_RETURN(("User::Acceptor::on_accept: accept failure: %s",OOBase::system_error_text(err)),false);

	// Read 4 bytes - This forces credential passing
	OOBase::CDRStream stream;
	err = ptrSocket->recv(stream.buffer(),sizeof(Omega::uint32_t));
	if (err != 0)
	{
		LOG_WARNING(("User::Acceptor::on_accept: receive failure: %s",OOBase::system_error_text(err)));
		return true;
	}

	// Check the versions are correct
	Omega::uint32_t version = 0;
	if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
	{
		LOG_WARNING(("User::Acceptor::on_accept: version received too early: %u",version));
		return true;
	}

#if defined(HAVE_UNISTD_H)

	// Check to see if the connection came from a process with our uid
	OOSvrBase::AsyncLocalSocket::uid_t uid;
	err = ptrSocket->get_uid(uid);
	if (err != 0)
	{
		LOG_WARNING(("User::Acceptor::on_accept: get_uid failure: %s",OOBase::system_error_text(err)));
		return true;
	}

	if (getuid() != uid)
	{
		LOG_WARNING(("User::Acceptor::on_accept: attempt to connect by invalid user"));
		return true;
	}

#endif

	m_pManager->on_accept(ptrSocket);

	// Keep accepting, whatever...
	return true;
}

bool User::Acceptor::init_security()
{
#if defined(_WIN32)

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

	// Set full control for the Logon SID only
	EXPLICIT_ACCESSW ea = {0};
	ea.grfAccessPermissions = FILE_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = (LPWSTR)ptrSIDLogon;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(1,&ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwErr)),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(m_sa);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

#elif defined(HAVE_UNISTD_H)

	m_sa.mode = 0700;

#else
#error set security on pipe_name
#endif

	return true;
}
