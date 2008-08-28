///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"
#include "./Exception.h"

using namespace Omega;
using namespace OTL;

// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
OMEGA_DEFINE_OID(OOCore,OID_SystemExceptionMarshalFactory, "{35F2702C-0A1B-4962-A012-F6BBBF4B0732}");

// {1E127359-1542-4329-8E30-FED8FF810960}
OMEGA_DEFINE_OID(OOCore,OID_NoInterfaceExceptionMarshalFactory, "{1E127359-1542-4329-8E30-FED8FF810960}");

#if defined(OMEGA_WIN32)
static string_t Win32Msg(DWORD dwErr)
{
	string_t strRet;

	LPVOID lpMsgBuf = 0;
	if (FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		0,
		(LPWSTR)&lpMsgBuf,
		0,	NULL))
	{
		strRet = (LPCWSTR)lpMsgBuf;

		// Free the buffer.
		LocalFree(lpMsgBuf);
	}
	else
	{
		strRet = string_t::Format(L"Unknown system err %#x\n",dwErr);
	}

	return strRet;
}
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,ISystemException_Create_errno,2,((in),uint32_t,e,(in),const string_t&,source))
{
	ObjectImpl<OOCore::SystemException>* pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();
	pExcept->m_strSource = source;
	pExcept->m_strDesc = string_t(ACE_OS::strerror(e),false);
	pExcept->m_errno = e;

#if defined(OMEGA_WIN32)
	if (e >= 42)
		pExcept->m_strDesc = Win32Msg(static_cast<DWORD>(e));
#endif

	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,ISystemException_Create,2,((in),const string_t&,desc,(in),const string_t&,source))
{
	ObjectImpl<OOCore::SystemException>* pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	pExcept->m_errno = EINVAL;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,INoInterfaceException_Create,2,((in),const guid_t&,iid,(in),const string_t&,source))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();

	string_t strIID = L"Unknown";

	const Omega::System::MetaInfo::qi_rtti* pRtti = Omega::System::MetaInfo::get_qi_rtti_info(iid);
	if (pRtti && pRtti->pszName)
		strIID = pRtti->pszName;
	
	pExcept->m_strDesc = L"Object does not support the requested interface: " + strIID;
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	return pExcept;
}
