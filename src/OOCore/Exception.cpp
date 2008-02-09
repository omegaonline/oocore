///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class Exception :
		public ExceptionImpl<IException>
	{
	public:
		BEGIN_INTERFACE_MAP(Exception)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IException>)
		END_INTERFACE_MAP()
	};

	class NoInterfaceException :
		public ExceptionImpl<INoInterfaceException>
	{
	public:
		guid_t m_iid;

		BEGIN_INTERFACE_MAP(NoInterfaceException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<INoInterfaceException>)
		END_INTERFACE_MAP()

	// INoInterfaceException members
	public:
		inline guid_t GetUnsupportedIID()
		{
			return m_iid;
		}
	};
}

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

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,IException_Create_GLE,3,((in),DWORD,GLE,(in),const string_t&,source,(in),IException*,pCause))
{
    ObjectImpl<OOCore::Exception>* pExcept = ObjectImpl<OOCore::Exception>::CreateInstance();
	pExcept->m_ptrCause = pCause;
	pExcept->m_strSource = source;
	pExcept->m_strDesc = Win32Msg(GLE);
	return pExcept;
}
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,IException_Create_errno,3,((in),int,e,(in),const string_t&,source,(in),IException*,pCause))
{
    ObjectImpl<OOCore::Exception>* pExcept = ObjectImpl<OOCore::Exception>::CreateInstance();
	pExcept->m_ptrCause = pCause;
	pExcept->m_strSource = source;
	pExcept->m_strDesc = string_t(ACE_OS::strerror(e),false);

#if defined(OMEGA_WIN32)
	// If errno is not set, then it isn't a std error
	if (errno == 0 || e<0 || e>42)
		pExcept->m_strDesc = Win32Msg(static_cast<DWORD>(e));
#endif	

	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,IException_Create,3,((in),const string_t&,desc,(in),const string_t&,source,(in),IException*,pCause))
{
    ObjectImpl<OOCore::Exception>* pExcept = ObjectImpl<OOCore::Exception>::CreateInstance();
	pExcept->m_ptrCause = pCause;
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,INoInterfaceException_Create,2,((in),const guid_t&,iid,(in),const string_t&,source))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();
	pExcept->m_strDesc = L"Object does not support the requested interface: " + Omega::System::MetaInfo::lookup_iid(iid);
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	return pExcept;
}
