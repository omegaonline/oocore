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

OOBase::string User::Acceptor::unique_name()
{
	// Create a new unique pipe
	try
	{
		OOBase::ostringstream ssPipe;
		ssPipe.imbue(std::locale::classic());
		ssPipe.setf(std::ios_base::hex,std::ios_base::basefield);

#if defined(_WIN32)
		ssPipe << "OOU";

		// Get the current user's Logon SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),"");

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::FreeDestructor<1> > ptrSIDLogon;
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
		ssPipe << "-" << OOBase::timeval_t::gettimeofday().tv_usec();

		return ssPipe.str();
	}
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),OOBase::string());
	}
}

bool User::Acceptor::start(Manager* pManager, const char* pipe_name)
{
	assert(!m_pManager);
	m_pManager = pManager;

	if (!init_security(pipe_name))
		return false;

	int err = 0;
	m_pSocket = Proactor::instance().accept_local(this,pipe_name,&err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err).c_str()),false);

	return true;
}

void User::Acceptor::stop()
{
	m_pSocket = 0;
}

bool User::Acceptor::on_accept(OOSvrBase::AsyncLocalSocketPtr ptrSocket, const char* /*strAddress*/, int err)
{
	if (err != 0)
		LOG_ERROR_RETURN(("User::Acceptor::on_accept: accept failure: %s",OOBase::system_error_text(err).c_str()),false);

	// Read 4 bytes - This forces credential passing
	OOBase::CDRStream stream;
	err = ptrSocket->recv(stream.buffer(),sizeof(Omega::uint32_t));
	if (err != 0)
	{
		LOG_WARNING(("User::Acceptor::on_accept: receive failure: %s",OOBase::system_error_text(err).c_str()));
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
		LOG_WARNING(("User::Acceptor::on_accept: get_uid failure: %s",OOBase::system_error_text(err).c_str()));
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

bool User::Acceptor::init_security(const char* pipe_name)
{
#if defined(_WIN32)

	OMEGA_UNUSED_ARG(pipe_name);

	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::FreeDestructor<1> > ptrSIDLogon;
	DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);

	PSID pSID;
	SID_IDENTIFIER_AUTHORITY SIDAuthCreator = {SECURITY_CREATOR_SID_AUTHORITY};
	if (!AllocateAndInitializeSid(&SIDAuthCreator, 1,
								  SECURITY_CREATOR_OWNER_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	}
	OOBase::SmartPtr<void,OOSvrBase::Win32::SIDDestructor<void> > pSIDOwner(pSID);

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
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

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
