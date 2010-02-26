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

#include "Exception.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(OOCore,OID_SystemExceptionMarshalFactory, "{35F2702C-0A1B-4962-A012-F6BBBF4B0732}");
OMEGA_DEFINE_OID(OOCore,OID_NoInterfaceExceptionMarshalFactory, "{1E127359-1542-4329-8E30-FED8FF810960}");
OMEGA_DEFINE_OID(OOCore,OID_TimeoutExceptionMarshalFactory, "{8FA37F2C-8252-437e-9C54-F07C13152E94}");
OMEGA_DEFINE_OID(OOCore,OID_ChannelClosedExceptionMarshalFactory, "{029B38C5-CC76-4d13-98A4-83A65D40710A}");

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		throw OOCore_ISystemException_Create(string_t(msg,false),0);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,OOCore_ISystemException_Create_errno,2,((in),uint32_t,e,(in),const string_t&,source))
{
	ObjectImpl<OOCore::SystemException>* pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();
	pExcept->m_strSource = source;

#if defined(_WIN32)

	pExcept->m_strDesc = string_t(OOBase::Win32::FormatMessage(static_cast<DWORD>(e)).c_str(),false);

#elif defined(HAVE_TR_24731)

	char szBuf[1024] = {0};
	strerror_s(szBuf,sizeof(szBuf),e);
	pExcept->m_strDesc = string_t(szBuf,false);

#else

	pExcept->m_strDesc = string_t(strerror(e),false);

#endif

	pExcept->m_errno = e;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,OOCore_ISystemException_Create,2,((in),const string_t&,desc,(in),const string_t&,source))
{
	ObjectImpl<OOCore::SystemException>* pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	pExcept->m_errno = EINVAL;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,OOCore_INoInterfaceException_Create,2,((in),const guid_t&,iid,(in),const string_t&,source))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();

	string_t strIID = L"Unknown";

	const Omega::System::Internal::qi_rtti* pRtti = Omega::System::Internal::get_qi_rtti_info(iid);
	if (pRtti && pRtti->pszName)
		strIID = pRtti->pszName;

	pExcept->m_strDesc = L"Object does not support the requested interface: " + strIID;
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ITimeoutException*,OOCore_ITimeoutException_Create,0,())
{
	ObjectImpl<OOCore::TimeoutException>* pExcept = ObjectImpl<OOCore::TimeoutException>::CreateInstance();

	pExcept->m_strDesc = L"The operation timed out";
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,0,())
{
	ObjectImpl<OOCore::ChannelClosedException>* pExcept = ObjectImpl<OOCore::ChannelClosedException>::CreateInstance();

	pExcept->m_strDesc = L"The remoting channel has closed";
	return pExcept;
}
