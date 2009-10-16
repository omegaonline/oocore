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

#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoTypeInfo,3,((in),const Omega::guid_t&,iid,(in),const wchar_t*,pszName,(in),const void*,type_info));
void Omega::System::MetaInfo::RegisterAutoTypeInfo(const guid_t& iid, const wchar_t* pszName, const typeinfo_rtti* type_info)
{
	OOCore_RegisterAutoTypeInfo(iid,pszName,(const void*)type_info);
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_UnregisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),const void*,type_info));
void Omega::System::MetaInfo::UnregisterAutoTypeInfo(const guid_t& iid, const typeinfo_rtti* type_info)
{
	OOCore_UnregisterAutoTypeInfo(iid,(const void*)type_info);
}

bool Omega::System::PinObjectPointer(IObject* pObject)
{
	if (pObject)
	{
		Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::MetaInfo::ISafeProxy))));
		if (ptrProxy)
		{
			ptrProxy->Pin();
			return true;
		}
	}

	return false;
}

void Omega::System::UnpinObjectPointer(IObject* pObject)
{
	Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::MetaInfo::ISafeProxy))));
	if (ptrProxy)
		ptrProxy->Unpin();
}

OOCORE_EXPORTED_FUNCTION(Omega::ISystemException*,OOCore_ISystemException_Create_errno,2,((in),Omega::uint32_t,e,(in),const Omega::string_t&,source))
Omega::ISystemException* Omega::ISystemException::Create(uint32_t errno_val, const string_t& source)
{
	return OOCore_ISystemException_Create_errno(errno_val,source);
}

OOCORE_EXPORTED_FUNCTION(Omega::ISystemException*,OOCore_ISystemException_Create,2,((in),const Omega::string_t&,desc,(in),const Omega::string_t&,source))
Omega::ISystemException* Omega::ISystemException::Create(const std::exception& e, const string_t& source)
{
	return OOCore_ISystemException_Create(string_t(e.what(),false),source);
}

Omega::ISystemException* Omega::ISystemException::Create(const string_t& desc, const string_t& source)
{
	return OOCore_ISystemException_Create(desc,source);
}

OOCORE_EXPORTED_FUNCTION(Omega::INoInterfaceException*,OOCore_INoInterfaceException_Create,2,((in),const Omega::guid_t&,iid,(in),const Omega::string_t&,source))
Omega::INoInterfaceException* Omega::INoInterfaceException::Create(const guid_t& iid, const string_t& source)
{
	return OOCore_INoInterfaceException_Create(iid,source);
}

OOCORE_EXPORTED_FUNCTION(Omega::ITimeoutException*,OOCore_ITimeoutException_Create,0,())
Omega::ITimeoutException* Omega::ITimeoutException::Create()
{
	return OOCore_ITimeoutException_Create();
}

OOCORE_EXPORTED_FUNCTION(Omega::Formatting::IFormattingException*,OOCore_IFormattingException_Create,3,((in),const Omega::string_t&,msg,(in),const Omega::string_t&,source,(in),Omega::IException*,pCause))
Omega::Formatting::IFormattingException* Omega::Formatting::IFormattingException::Create(const string_t& strMsg, const string_t& strSource, IException* pE)
{
	return OOCore_IFormattingException_Create(strMsg,strSource,pE);
}

#endif // OOCORE_RTTI_INL_INCLUDED_

