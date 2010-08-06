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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "ClientAcceptor.h"
#include "RootManager.h"

#include "../../include/Omega/version.h"

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
	std::string pipe_name = "OmegaOnline";
#elif defined(HAVE_UNISTD_H)
	std::string pipe_name = "/tmp/omegaonline";
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
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name.c_str(),OOBase::system_error_text(err).c_str()),false);

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
		LOG_ERROR_RETURN(("Root::ClientAcceptor::on_accept: accept failure: %s",OOBase::system_error_text(err).c_str()),false);

	err = pSocket->close_on_exec();
	if (err != 0)
	{
		LOG_WARNING(("Root::ClientAcceptor::on_accept: close_on_exec failure: %s",OOSvrBase::Logger::format_error(err).c_str()));
		pSocket->close();
		return true;
	}

	// Read 4 bytes - This forces credential passing
	Omega::uint32_t version = 0;
	err = pSocket->recv(version);
	if (err != 0)
	{
		LOG_WARNING(("Root::ClientAcceptor::on_accept: receive failure: %s",OOBase::system_error_text(err).c_str()));
		pSocket->close();
		return true;
	}

	// Check the versions are correct
	if (version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
	{
		LOG_WARNING(("Root::ClientAcceptor::on_accept: unsupported version received: %u",version));
		pSocket->close();
		return true;
	}

	m_pManager->accept_client(pSocket);

	return true;
}

bool Root::ClientAcceptor::init_security(const std::string& pipe_name)
{
#if defined(_WIN32)

	assert(!pipe_name.empty());

	const int NUM_ACES  = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES] = {0};

	PSID pSID;
	SID_IDENTIFIER_AUTHORITY SIDAuthCreator = SECURITY_CREATOR_SID_AUTHORITY;
	if (!AllocateAndInitializeSid(&SIDAuthCreator, 1,
								  SECURITY_CREATOR_OWNER_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	}
	OOBase::SmartPtr<void,OOSvrBase::Win32::SIDDestructor<void> > pSIDOwner(pSID);

	// Set full control for the creating process SID
	ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPWSTR)pSIDOwner;

	// Create a SID for the Local users group.
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_LOCAL_SID_AUTHORITY;
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
								  SECURITY_LOCAL_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	}
	OOBase::SmartPtr<void,OOSvrBase::Win32::SIDDestructor<void> > pSIDUsers(pSID);

	// Set read/write access
	ea[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDUsers;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(m_sa);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

#elif defined(HAVE_UNISTD_H)

	m_sa.mode = 0777;

#else
#error Fix me!
#endif

	return true;
}
