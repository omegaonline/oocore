///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSVRBASE_SECURITY_WIN32_H_INCLUDED_
#define OOSVRBASE_SECURITY_WIN32_H_INCLUDED_

#if defined(_WIN32)

#include "SmartPtr.h"

#include <string>

#include <userenv.h>
#include <lm.h>
#include <aclapi.h>

#if defined(__MINGW32__)

typedef struct _TOKEN_GROUPS_AND_PRIVILEGES {
    DWORD SidCount;
    DWORD SidLength;
    PSID_AND_ATTRIBUTES Sids;
    DWORD RestrictedSidCount;
    DWORD RestrictedSidLength;
    PSID_AND_ATTRIBUTES RestrictedSids;
    DWORD PrivilegeCount;
    DWORD PrivilegeLength;
    PLUID_AND_ATTRIBUTES Privileges;
    LUID AuthenticationId;
} TOKEN_GROUPS_AND_PRIVILEGES;

extern "C"
WINADVAPI
BOOL
APIENTRY
CreateRestrictedToken(
    HANDLE ExistingTokenHandle,
    DWORD Flags,
    DWORD DisableSidCount,
    PSID_AND_ATTRIBUTES SidsToDisable,
    DWORD DeletePrivilegeCount,
    PLUID_AND_ATTRIBUTES PrivilegesToDelete,
    DWORD RestrictedSidCount,
    PSID_AND_ATTRIBUTES SidsToRestrict,
    PHANDLE NewTokenHandle
    );

#define DISABLE_MAX_PRIVILEGE   0x1
#define SANDBOX_INERT           0x2

#else
// Not available under MinGW
#include <WinSafer.h>
#endif

namespace OOSvrBase
{
	namespace Win32
	{
		class sec_descript_t
		{
		public:
			sec_descript_t();
			~sec_descript_t();

			DWORD SetEntriesInAcl(ULONG cCountOfExplicitEntries, PEXPLICIT_ACCESSW pListOfExplicitEntries, PACL OldAcl);

			void* descriptor()
			{
				return m_psd.value();
			}

		private:
			OOBase::SmartPtr<ACL,OOBase::Win32::LocalAllocDestructor<ACL> >   m_pACL;
			OOBase::SmartPtr<void,OOBase::Win32::LocalAllocDestructor<void> > m_psd;
		};

		DWORD RestrictToken(HANDLE& hToken);
		DWORD SetTokenDefaultDACL(HANDLE hToken);
		DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile);
		void* GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls);
		DWORD GetNameFromToken(HANDLE hToken, std::wstring& strUserName, std::wstring& strDomainName);
		DWORD GetLogonSID(HANDLE hToken, OOBase::SmartPtr<void,OOBase::FreeDestructor<void> >& pSIDLogon);
		DWORD EnableUserAccessToDir(const wchar_t* pszPath, const TOKEN_USER* pUser);
		bool MatchSids(ULONG count, PSID_AND_ATTRIBUTES pSids1, PSID_AND_ATTRIBUTES pSids2);
		bool MatchPrivileges(ULONG count, PLUID_AND_ATTRIBUTES Privs1, PLUID_AND_ATTRIBUTES Privs2);
	}
}

#endif // _WIN32

#endif // OOSVRBASE_SECURITY_WIN32_H_INCLUDED_
