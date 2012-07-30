///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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

#ifndef OMEGA_TYPEINFO_H_INCLUDED_
#define OMEGA_TYPEINFO_H_INCLUDED_

#include "./Omega.h"

namespace Omega
{
	namespace TypeInfo
	{
		interface IInterfaceInfo : public IObject
		{
			virtual string_t GetName() = 0;
			virtual guid_t GetIID() = 0;
			virtual uint32_t GetMethodCount() = 0;
			virtual IInterfaceInfo* GetBaseType() = 0;
			virtual void GetMethodInfo(uint32_t method_idx, string_t& strName, MethodAttributes_t& attribs, byte_t& param_count, Remoting::IMessage*& return_type) = 0;
			virtual void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, ParamAttributes_t& attribs) = 0;
			virtual byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, ParamAttributes_t attrib) = 0;
		};

		interface IProvideObjectInfo : public IObject
		{
			typedef std::vector<guid_t,System::STLAllocator<guid_t> > iid_list_t;

			virtual iid_list_t EnumInterfaces() = 0;
		};

		static IInterfaceInfo* GetInterfaceInfo(const guid_t& iid, IObject* pObject = 0);
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::TypeInfo, IInterfaceInfo, "{13EC66A0-D266-4682-9A47-6E2F178C40BD}",

	OMEGA_METHOD(string_t,GetName,0,())
	OMEGA_METHOD(guid_t,GetIID,0,())
	OMEGA_METHOD(Omega::TypeInfo::IInterfaceInfo*,GetBaseType,0,())
	OMEGA_METHOD(uint32_t,GetMethodCount,0,())
	OMEGA_METHOD_VOID(GetMethodInfo,5,((in),uint32_t,method_idx,(out),string_t&,strName,(out),TypeInfo::MethodAttributes_t&,attribs,(out),byte_t&,param_count,(out),Remoting::IMessage*&,return_type))
	OMEGA_METHOD_VOID(GetParamInfo,5,((in),uint32_t,method_idx,(in),byte_t,param_idx,(out),string_t&,strName,(out),Remoting::IMessage*&,type,(out),TypeInfo::ParamAttributes_t&,attribs))
	OMEGA_METHOD(byte_t,GetAttributeRef,3,((in),uint32_t,method_idx,(in),byte_t,param_idx,(in),TypeInfo::ParamAttributes_t,attrib))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::TypeInfo, IProvideObjectInfo, "{F66A857D-C474-4c9e-B08B-68135AC8459E}",

	// Methods
	OMEGA_METHOD(Omega::TypeInfo::IProvideObjectInfo::iid_list_t,EnumInterfaces,0,())
)

OOCORE_EXPORTED_FUNCTION(Omega::TypeInfo::IInterfaceInfo*,OOCore_TypeInfo_GetInterfaceInfo,2,((in),const Omega::guid_t&,iid,(in),Omega::IObject*,pObject));
inline Omega::TypeInfo::IInterfaceInfo* Omega::TypeInfo::GetInterfaceInfo(const Omega::guid_t& iid, Omega::IObject* pObject)
{
	return OOCore_TypeInfo_GetInterfaceInfo(iid,pObject);
}

#endif // !defined(DOXYGEN)

#endif // OMEGA_TYPEINFO_H_INCLUDED_
