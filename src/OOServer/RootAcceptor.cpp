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
#include "RootAcceptor.h"
#include "RootManager.h"

#if defined(OMEGA_WIN32)
#include <sddl.h>
#include <aclapi.h>
#endif

namespace
{
	static std::string unique_name(OOBase::Socket::uid_t uid)
	{
	#if defined(ACE_WIN32)

		std::string strSid;

		DWORD dwLen = 0;
		GetTokenInformation(uid,TokenUser,NULL,0,&dwLen);
		if (dwLen == 0)
			OOBase::CallCriticalFailure();
		else
		{
			TOKEN_USER* pSIDProcess = static_cast<TOKEN_USER*>(ACE_OS::malloc(dwLen));
			if (!pSIDProcess)
				OOBase::CallCriticalFailure();
			else
			{
				if (!GetTokenInformation(uid,TokenUser,pSIDProcess,dwLen,&dwLen))
					OOBase::CallCriticalFailure();
				else
				{
					LPSTR pszSid = 0;
					if (!ConvertSidToStringSidA(pSIDProcess->User.Sid,&pszSid))
						OOBase::CallCriticalFailure();
					else
					{
						strSid = pszSid;
						LocalFree(pszSid);
					}
				}

				ACE_OS::free(pSIDProcess);
			}
		}

	#if defined(OMEGA_DEBUG)
		char szBuf[64];
		OOBase::timeval_t now = OOBase::gettimeofday();
		ACE_OS::snprintf(szBuf,63,"-%lx",now.tv_usec);
		strSid += szBuf;
	#endif

		return "oor" + strSid;

	#else

		char szBuf[64];
	#if defined(OMEGA_DEBUG)
		OOBase::timeval_t now = OOBase::gettimeofday();
		ACE_OS::snprintf(szBuf,63,"%lx-%lx",uid,now.tv_usec);
	#else
		ACE_OS::snprintf(szBuf,63,"%lx",uid);
	#endif

		if (ACE_OS::mkdir("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO) != 0)
		{
			int err = ACE_OS::last_error();
			if (err != EEXIST)
				return "";
		}

		// Try to make it public
		chmod("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO);

		// Attempt to remove anything already there
		ACE_OS::unlink(("/tmp/omegaonline/" + strPrefix + szBuf).c_str());

		return "/tmp/omegaonline/oor" + szBuf;
	#endif
	}
}

OOSvrBase::AsyncSocket* Root::UserAcceptor::accept(MessageHandler* message_handler, OOBase::Socket::uid_t uid, int* perr, const OOBase::timeval_t* wait)
{
	std::string path = unique_name(uid);

	void* SECURITY; // TODO
	return Root::Manager::Proactor::instance()->accept_local_blocking(message_handler,path,perr,wait,0);
}
