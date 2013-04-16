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
			throw static_cast<ISystemException*>(this);
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

const Omega::guid_t OOCore::OID_SystemExceptionMarshalFactory("{35F2702C-0A1B-4962-A012-F6BBBF4B0732}");
const Omega::guid_t OOCore::OID_InternalExceptionMarshalFactory("{47E86F31-E9E9-4667-89CA-40EB048DA2B7}");
const Omega::guid_t OOCore::OID_NotFoundExceptionMarshalFactory("{1E127359-1542-4329-8E30-FED8FF810960}");
const Omega::guid_t OOCore::OID_AccessDeniedExceptionMarshalFactory("{5CA887CE-648C-BBE4-9B66-14F275879CFB}");
const Omega::guid_t OOCore::OID_TimeoutExceptionMarshalFactory("{8FA37F2C-8252-437e-9C54-F07C13152E94}");
const Omega::guid_t OOCore::OID_ChannelClosedExceptionMarshalFactory("{029B38C5-CC76-4d13-98A4-83A65D40710A}");
const Omega::guid_t OOCore::OID_AlreadyExistsExceptionMarshalFactory("{BA90E55F-E0B6-0528-C45F-32DD9C3A414E}");

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_ISystemException_ThrowNoMem,0,())
{
	OutOfMemoryException::s_instance.Rethrow();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ISystemException*,OOCore_ISystemException_Create_errno,2,((in),uint32_t,e,(in),IException*,pCause))
{
	if (e == ERROR_OUTOFMEMORY)
		OutOfMemoryException::s_instance.Rethrow();

	ObjectPtr<ObjectImpl<OOCore::SystemException> > pExcept = ObjectImpl<OOCore::SystemException>::CreateObject();
	pExcept->m_strDesc = OOBase::system_error_text(e);
	pExcept->m_errno = e;
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	return pExcept.Detach();
}

namespace OOCore
{
	ObjectPtr<ObjectImpl<OOCore::InternalException> > CreateInternalException(const string_t& desc, const char* pszFile, size_t nLine, const char* pszFunc, IException* pCause)
	{
		ObjectPtr<ObjectImpl<OOCore::InternalException> > pExcept = ObjectImpl<OOCore::InternalException>::CreateObject();

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

			if (nLine)
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
	return OOCore::CreateInternalException("Operating system error",pszFile,nLine,pszFunc,ptrE).Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_Create,5,((in),const string_t&,desc,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc,(in),IException*,pCause))
{
	return OOCore::CreateInternalException(desc,pszFile,nLine,pszFunc,pCause).Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_NullReference,0,())
{
	return OOCore::CreateInternalException(OOCore::get_text("NULL pointer passed as reference"),NULL,0,NULL,NULL).Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IInternalException*,OOCore_IInternalException_BadException,1,((in),const char*,location))
{
	return OOCore::CreateInternalException(OOCore::get_text("An unrecognized C++ exception was caught in {0}") % location,NULL,0,NULL,NULL).Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INotFoundException*,OOCore_INotFoundException_Create,2,((in),const string_t&,strDesc,(in),Omega::IException*,pCause))
{
	ObjectPtr<ObjectImpl<OOCore::NotFoundException> > pExcept = ObjectImpl<OOCore::NotFoundException>::CreateObject();
	pExcept->m_strDesc = strDesc;
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INotFoundException*,OOCore_INotFoundException_MissingIID,1,((in),const guid_t&,iid))
{
	ObjectPtr<ObjectImpl<OOCore::NotFoundException> > pExcept = ObjectImpl<OOCore::NotFoundException>::CreateObject();

	string_t strIID = iid.ToString();

	ObjectPtr<TypeInfo::IInterfaceInfo> ptrII = OOCore::GetInterfaceInfo(iid);
	if (ptrII)
		strIID = ptrII->GetName();

	pExcept->m_strDesc = OOCore::get_text("Object does not support the requested interface: {0}") % strIID;
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INotFoundException*,OOCore_INotFoundException_MissingRTTI,1,((in),const guid_t&,iid))
{
	ObjectPtr<ObjectImpl<OOCore::NotFoundException> > pExcept = ObjectImpl<OOCore::NotFoundException>::CreateObject();

	string_t strIID = iid.ToString();

	ObjectPtr<TypeInfo::IInterfaceInfo> ptrII = OOCore::GetInterfaceInfo(iid);
	if (ptrII)
		strIID = ptrII->GetName();

	pExcept->m_strDesc = OOCore::get_text("Failed to find required type information for interface {0}") % strIID;
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INotFoundException*,OOCore_INotFoundException_BadInvoke,1,((in),uint32_t,method_id))
{
	ObjectPtr<ObjectImpl<OOCore::NotFoundException> > pExcept = ObjectImpl<OOCore::NotFoundException>::CreateObject();
	pExcept->m_strDesc = OOCore::get_text("Invoke called with invalid method index {0}") % method_id;
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IAlreadyExistsException*,OOCore_IAlreadyExistsException_Create,1,((in),const string_t&,strDesc))
{
	ObjectPtr<ObjectImpl<OOCore::AlreadyExistsException> > pExcept = ObjectImpl<OOCore::AlreadyExistsException>::CreateObject();
	pExcept->m_strDesc = strDesc;
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IAccessDeniedException*,OOCore_IAccessDeniedException_Create,2,((in),const string_t&,strDesc,(in),Omega::IException*,pCause))
{
	ObjectPtr<ObjectImpl<OOCore::AccessDeniedException> > pExcept = ObjectImpl<OOCore::AccessDeniedException>::CreateObject();
	pExcept->m_strDesc = strDesc;
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(ITimeoutException*,OOCore_ITimeoutException_Create,0,())
{
	ObjectPtr<ObjectImpl<OOCore::TimeoutException> > pExcept = ObjectImpl<OOCore::TimeoutException>::CreateObject();
	pExcept->m_strDesc = OOCore::get_text("The operation timed out");
	return pExcept.Detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,1,((in),IException*,pCause))
{
	ObjectPtr<ObjectImpl<OOCore::ChannelClosedException> > pExcept = ObjectImpl<OOCore::ChannelClosedException>::CreateObject();
	pExcept->m_ptrCause = pCause;
	pExcept->m_ptrCause.AddRef();
	pExcept->m_strDesc = OOCore::get_text("The remoting channel has closed");
	return pExcept.Detach();
}
