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

namespace
{
	class OutOfMemoryException : public ISystemException
	{
	public:
		OutOfMemoryException() : m_strError(OOBase::system_error_text(ERROR_OUTOFMEMORY))
		{ }

		static OutOfMemoryException s_instance;
		
	private:
		string_t m_strError;	

	// IObject members
	public:
		void AddRef() {}
		void Release() {}
		IObject* QueryInterface(const guid_t& iid)
		{
			if (iid == OMEGA_GUIDOF(IObject) ||
				iid == OMEGA_GUIDOF(IException) ||
				iid == OMEGA_GUIDOF(ISystemException))
			{
				return this;
			}
			
			return NULL;
		}

	// IException members
	public:
		void Rethrow()
		{
			throw static_cast<IException*>(this);
		}

		guid_t GetThrownIID()
		{
			return OMEGA_GUIDOF(ISystemException);
		}

		IException* GetCause()
		{
			return NULL;
		}

		string_t GetDescription()
		{
			return m_strError;
		}

	// ISystemException members
	public:
		uint32_t GetErrorCode()
		{
			return ERROR_OUTOFMEMORY;
		}
	};

	OutOfMemoryException OutOfMemoryException::s_instance;
}

OMEGA_DEFINE_OID(OOCore,OID_SystemExceptionMarshalFactory, "{35F2702C-0A1B-4962-A012-F6BBBF4B0732}");
OMEGA_DEFINE_OID(OOCore,OID_InternalExceptionMarshalFactory, "{47E86F31-E9E9-4667-89CA-40EB048DA2B7}");
OMEGA_DEFINE_OID(OOCore,OID_NoInterfaceExceptionMarshalFactory, "{1E127359-1542-4329-8E30-FED8FF810960}");
OMEGA_DEFINE_OID(OOCore,OID_TimeoutExceptionMarshalFactory, "{8FA37F2C-8252-437e-9C54-F07C13152E94}");
OMEGA_DEFINE_OID(OOCore,OID_ChannelClosedExceptionMarshalFactory, "{029B38C5-CC76-4d13-98A4-83A65D40710A}");

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,OOCore_ISystemException_Create_errno,2,((in),uint32_t,e,(in),IException*,pCause))
{
	if (e == ERROR_OUTOFMEMORY)
		OutOfMemoryException::s_instance.Rethrow();

	ObjectPtr<ObjectImpl<OOCore::SystemException> > pExcept = ObjectImpl<OOCore::SystemException>::CreateInstance();
	pExcept->m_strDesc = OOBase::system_error_text(e);
	pExcept->m_errno = e;
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	return pExcept.AddRef();
}

namespace OOCore
{
	ObjectPtr<ObjectImpl<OOCore::InternalException> > CreateInternalException(const string_t& desc, const char* pszFile, size_t nLine, const char* pszFunc, IException* pCause)
	{
		ObjectPtr<ObjectImpl<OOCore::InternalException> > pExcept = ObjectImpl<OOCore::InternalException>::CreateInstance();

		pExcept->m_strDesc = desc;
		pExcept->m_ptrCause = pCause;
		pExcept->m_ptrCause.AddRef();

		if (pszFile)
		{
			static const char szOurFile[] = __FILE__;
			size_t s=0;
			for (;;)
			{
				size_t s1 = s;
				while (szOurFile[s1] == pszFile[s1] && szOurFile[s1] != '\\' && szOurFile[s1] != '/')
					++s1;

				if (szOurFile[s1] == '\\' || szOurFile[s1] == '/')
					s = s1+1;
				else
					break;
			}
			pszFile += s;

			if (nLine != size_t(-1))
			{
				if (pszFunc)
					pExcept->m_strSource = string_t::constant("{0}({1}): {2}") % pszFile % nLine % pszFunc;
				else
					pExcept->m_strSource = string_t::constant("{0}({1})") % pszFile % nLine;
			}
			else
				pExcept->m_strSource = string_t::constant("{0}") % pszFile;
		}

		return pExcept;
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_Create_errno,4,((in),int32_t,e,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc))
{
	ObjectPtr<IException> ptrE = OOCore_ISystemException_Create_errno(e,NULL);
	return OOCore::CreateInternalException("Operating system error",pszFile,nLine,pszFunc,ptrE).AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_Create,5,((in),const Omega::string_t&,desc,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc,(in),Omega::IException*,pCause))
{
	return OOCore::CreateInternalException(desc,pszFile,nLine,pszFunc,pCause).AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,OOCore_INoInterfaceException_Create,1,((in),const guid_t&,iid))
{
	ObjectPtr<ObjectImpl<OOCore::NoInterfaceException> > pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();

	string_t strIID = OOCore::get_text("Unknown interface {0}") % iid.ToString();

	ObjectPtr<TypeInfo::IInterfaceInfo> ptrII = OOCore::GetInterfaceInfo(iid);
	if (ptrII)
		strIID = ptrII->GetName();

	pExcept->m_strDesc = OOCore::get_text("Object does not support the requested interface {0}") % strIID;
	pExcept->m_iid = iid;
	return pExcept.AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ITimeoutException*,OOCore_ITimeoutException_Create,0,())
{
	ObjectPtr<ObjectImpl<OOCore::TimeoutException> > pExcept = ObjectImpl<OOCore::TimeoutException>::CreateInstance();
	pExcept->m_strDesc = OOCore::get_text("The operation timed out");
	return pExcept.AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,1,((in),IException*,pCause))
{
	ObjectPtr<ObjectImpl<OOCore::ChannelClosedException> > pExcept = ObjectImpl<OOCore::ChannelClosedException>::CreateInstance();
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	pExcept->m_strDesc = OOCore::get_text("The remoting channel has closed");
	return pExcept.AddRef();
}
