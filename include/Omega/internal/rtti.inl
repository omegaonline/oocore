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

inline bool Omega::System::PinObjectPointer(Omega::IObject* pObject)
{
	if (pObject)
	{
		Omega::System::Internal::auto_iface_ptr<Omega::System::Internal::ISafeProxy> ptrProxy(static_cast<Omega::System::Internal::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::Internal::ISafeProxy))));
		if (ptrProxy)
		{
			ptrProxy->Pin();
			return true;
		}
	}

	return false;
}

inline void Omega::System::UnpinObjectPointer(Omega::IObject* pObject)
{
	Omega::System::Internal::auto_iface_ptr<Omega::System::Internal::ISafeProxy> ptrProxy(static_cast<Omega::System::Internal::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::Internal::ISafeProxy))));
	if (ptrProxy)
		ptrProxy->Unpin();
}

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Internal_RegisterAutoTypeInfo,3,((in),const Omega::guid_t&,iid,(in),const wchar_t*,pszName,(in),const void*,type_info));
inline void Omega::System::Internal::register_typeinfo(const Omega::guid_t& iid, const wchar_t* pszName, const typeinfo_rtti* type_info)
{
	OOCore_Internal_RegisterAutoTypeInfo(iid,pszName,(const void*)type_info);
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Internal_UnregisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),const void*,type_info));
inline void Omega::System::Internal::unregister_typeinfo(const Omega::guid_t& iid, const typeinfo_rtti* type_info)
{
	OOCore_Internal_UnregisterAutoTypeInfo(iid,(const void*)type_info);
}

OOCORE_EXPORTED_FUNCTION(Omega::ISystemException*,OOCore_ISystemException_Create_errno,2,((in),Omega::uint32_t,e,(in),Omega::IException*,pCause))
inline Omega::ISystemException* Omega::ISystemException::Create(uint32_t errno_val, Omega::IException* pCause)
{
	return OOCore_ISystemException_Create_errno(errno_val,pCause);
}

OOCORE_EXPORTED_FUNCTION(Omega::IInternalException*,OOCore_IInternalException_Create_errno,4,((in),Omega::int32_t,e,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc))
inline Omega::IInternalException* Omega::IInternalException::Create(int32_t errno_val, const char* pszFile, size_t nLine, const char* pszFunc)
{
	return OOCore_IInternalException_Create_errno(errno_val,pszFile,nLine,pszFunc);
}

OOCORE_EXPORTED_FUNCTION(Omega::IInternalException*,OOCore_IInternalException_Create,4,((in),const char*,desc,(in),const char*,pszFile,(in),size_t,nLine,(in),const char*,pszFunc))
inline Omega::IInternalException* Omega::IInternalException::Create(const std::exception& e, const char* pszFile, size_t nLine, const char* pszFunc)
{
	std::basic_string<char, std::char_traits<char>, Omega::System::stl_allocator<char> > str("STL exception: ");
	str += e.what();
	return OOCore_IInternalException_Create(str.c_str(),pszFile,nLine,pszFunc);
}

inline Omega::IInternalException* Omega::IInternalException::Create(const char* desc, const char* pszFile, size_t nLine, const char* pszFunc)
{
	return OOCore_IInternalException_Create(desc,pszFile,nLine,pszFunc);
}

OOCORE_EXPORTED_FUNCTION(Omega::INoInterfaceException*,OOCore_INoInterfaceException_Create,1,((in),const Omega::guid_t&,iid))
inline Omega::INoInterfaceException* Omega::INoInterfaceException::Create(const guid_t& iid)
{
	return OOCore_INoInterfaceException_Create(iid);
}

OOCORE_EXPORTED_FUNCTION(Omega::ITimeoutException*,OOCore_ITimeoutException_Create,0,())
inline Omega::ITimeoutException* Omega::ITimeoutException::Create()
{
	return OOCore_ITimeoutException_Create();
}

OOCORE_EXPORTED_FUNCTION(Omega::Formatting::IFormattingException*,OOCore_IFormattingException_Create,1,((in),const Omega::string_t&,msg))
inline Omega::Formatting::IFormattingException* Omega::Formatting::IFormattingException::Create(const string_t& strMsg)
{
	return OOCore_IFormattingException_Create(strMsg);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_RTTI_INL_INCLUDED_

