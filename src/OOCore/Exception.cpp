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

namespace OOCore
{
	TypeInfo::IInterfaceInfo* GetInterfaceInfo(const guid_t& iid);
}

OMEGA_DEFINE_OID(OOCore,OID_SystemExceptionMarshalFactory, "{35F2702C-0A1B-4962-A012-F6BBBF4B0732}");
OMEGA_DEFINE_OID(OOCore,OID_InternalExceptionMarshalFactory, "{47E86F31-E9E9-4667-89CA-40EB048DA2B7}");
OMEGA_DEFINE_OID(OOCore,OID_NoInterfaceExceptionMarshalFactory, "{1E127359-1542-4329-8E30-FED8FF810960}");
OMEGA_DEFINE_OID(OOCore,OID_TimeoutExceptionMarshalFactory, "{8FA37F2C-8252-437e-9C54-F07C13152E94}");
OMEGA_DEFINE_OID(OOCore,OID_ChannelClosedExceptionMarshalFactory, "{029B38C5-CC76-4d13-98A4-83A65D40710A}");

namespace OOBase
{
	// This is the critical failure hook
	void OnCriticalFailure(const char* msg)
	{
		throw OOCore_IInternalException_Create(msg,"OOCore Critical Failure",size_t(-1),0);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,OOCore_ISystemException_Create_errno,2,((in),uint32_t,e,(in),Omega::IException*,pCause))
{
	ObjectImpl<OOCore::SystemException>* pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();

	pExcept->m_strDesc = string_t(OOBase::system_error_text(e),false);
	pExcept->m_errno = e;
	pExcept->m_ptrCause = pCause;

	return pExcept;
}

namespace OOCore
{
	ObjectPtr<ObjectImpl<OOCore::InternalException> > CreateInternalException(const char* desc, const char* pszFile, size_t nLine, const char* pszFunc)
	{
		ObjectPtr<ObjectImpl<OOCore::InternalException> > pExcept = ObjectImpl<OOCore::InternalException>::CreateInstancePtr();

		// Make this use gettext etc...
		void* TODO;
		
		pExcept->m_strDesc = string_t(desc,false);

		if (nLine != size_t(-1))
		{
			if (pszFunc)
				pExcept->m_strSource = (L"{0}({1}): {2}" % Omega::string_t(pszFile,false) % nLine % Omega::string_t(pszFunc,false));
			else
				pExcept->m_strSource = (L"{0}({1})" % Omega::string_t(pszFile,false) % nLine);
		}
		else
			pExcept->m_strSource = (L"{0}" % Omega::string_t(pszFile,false));

		return pExcept;
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_Create_errno,4,((in),int32_t,e,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc))
{
	ObjectPtr<ObjectImpl<OOCore::InternalException> > ptrExcept = OOCore::CreateInternalException("Operating system error",pszFile,nLine,pszFunc);
	ptrExcept->m_ptrCause.Attach(OOCore_ISystemException_Create_errno(e,0));

	return ptrExcept.AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_Create,4,((in),const char*,desc,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc))
{
	return OOCore::CreateInternalException(desc,pszFile,nLine,pszFunc).AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,OOCore_INoInterfaceException_Create,1,((in),const guid_t&,iid))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();

	string_t strIID(L"Unknown");

	ObjectPtr<TypeInfo::IInterfaceInfo> ptrII;
	ptrII.Attach(OOCore::GetInterfaceInfo(iid));
	if (ptrII)
		strIID = ptrII->GetName();

	pExcept->m_strDesc = L"Object does not support the requested interface: " + strIID;
	pExcept->m_iid = iid;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ITimeoutException*,OOCore_ITimeoutException_Create,0,())
{
	ObjectImpl<OOCore::TimeoutException>* pExcept = ObjectImpl<OOCore::TimeoutException>::CreateInstance();

	pExcept->m_strDesc = L"The operation timed out";
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,1,((in),Omega::IException*,pCause))
{
	ObjectImpl<OOCore::ChannelClosedException>* pExcept = ObjectImpl<OOCore::ChannelClosedException>::CreateInstance();

	pExcept->m_ptrCause = pCause;
	pExcept->m_strDesc = L"The remoting channel has closed";
	return pExcept;
}
