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

#ifndef OOCORE_IFACES_H_INCLUDED_
#define OOCORE_IFACES_H_INCLUDED_

namespace Omega
{
	interface ISystemException : public IException
	{
		virtual uint32_t GetErrorCode() = 0;

		static ISystemException* Create(uint32_t errno_val, IException* pCause = NULL);
	};

	interface ITimeoutException : public IException
	{
		static ITimeoutException* Create();
	};

	interface INotFoundException : public IException
	{
		static INotFoundException* Create(const string_t& strDesc, IException* pCause = NULL);
	};

	interface IAlreadyExistsException : public IException
	{
		static IAlreadyExistsException* Create(const string_t& strDesc);
	};

	interface IAccessDeniedException : public IException
	{
		static IAccessDeniedException* Create(const string_t& strDesc, IException* pCause = NULL);
	};

	interface ICastException : public IException
	{
		virtual any_t GetValue() = 0;
		virtual any_t::CastResult_t GetReason() = 0;
		virtual Remoting::IMessage* GetDestinationType() = 0;
	};

	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateInstance(const guid_t& iid, IObject*& pObject) = 0;
		};

		enum Flags
		{	
			Default = 0,                         ///< Use a dll/so or executable as available
			Library = 1,                         ///< Only use dll/so
			Process = 2,                         ///< Launch as current user - implies surrogate if dll/so
			Sandbox = 3,                         ///< Launch as the sandbox user - implies surrogate if dll/so
			
			OwnSurrogate = 0x10,                 ///< Launch dll/so in its own surrogate wrapper
			RemoteActivation = 0x20,             ///< Request is from a remote machine
			DontLaunch = 0x40                    ///< Do not launch exe/dll/so if not already running		
		};
		typedef uint16_t Flags_t;

		IObjectFactory* GetObjectFactory(const any_t& oid, Activation::Flags_t flags);

		enum RegisterFlags
		{
			ProcessScope = 1,    // Register for calling process only
			UserScope = 3,       // Register for calling user only
			PublicScope = 7,     // Register for all users
			ExternalPublic = 8,  // Register as externally accessible
			
			MultipleUse = 0,
			SingleUse = 0x10,           // Auto Revoke after 1st GetObject
			MultipleRegistration = 0x20 // Allow multiple calls to Register with different flags
		};
		typedef uint16_t RegisterFlags_t;

		interface IRunningObjectTable : public IObject
		{
			virtual uint32_t RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags) = 0;
			virtual void RevokeObject(uint32_t cookie) = 0;
			virtual void GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject, bool_t remote = false) = 0;
		};
		
		// {F67F5A41-BA32-48C9-BFD2-7B3701984DC8}
		OOCORE_DECLARE_OID(OID_RunningObjectTable);
	}

	namespace TypeInfo
	{
		interface IInterfaceInfo : public IObject
		{
			virtual string_t GetName() = 0;
			virtual guid_t GetIID() = 0;
			virtual uint32_t GetMethodCount() = 0;
			virtual IInterfaceInfo* GetBaseType() = 0;
			virtual void GetMethodInfo(uint32_t method_idx, string_t& strName, MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, Remoting::IMessage*& return_type) = 0;
			virtual void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, ParamAttributes_t& attribs) = 0;
			virtual byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, ParamAttributes_t attrib) = 0;
		};
	}

	namespace Registry
	{
		interface IKey : public IObject
		{
			enum OpenFlags
			{
				OpenExisting = 0,
				OpenCreate = 1,
				CreateNew = 2
			};
			typedef uint16_t OpenFlags_t;

			typedef std::set<string_t,std::less<string_t>,System::STLAllocator<string_t> > string_set_t;

			virtual string_t GetName() = 0;
			virtual bool_t IsKey(const string_t& key) = 0;
			virtual string_set_t EnumSubKeys() = 0;
			virtual IKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting) = 0;
			virtual void DeleteSubKey(const string_t& strKey) = 0;

			virtual bool_t IsValue(const string_t& name) = 0;
			virtual string_set_t EnumValues() = 0;
			virtual any_t GetValue(const string_t& name) = 0;
			virtual void SetValue(const string_t& name, const any_t& val) = 0;
			virtual void DeleteValue(const string_t& strValue) = 0;
		};

		// {EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}
		OOCORE_DECLARE_OID(OID_Registry);

		interface IOverlayKeyFactory : public IObject
		{
			virtual IKey* Overlay(const string_t& strOver, const string_t& strUnder) = 0;
		};

		// {7A351233-8363-BA15-B443-31DD1C8FC587}
		OOCORE_DECLARE_OID(OID_OverlayKeyFactory);
	}

	namespace TypeInfo
	{
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
	OMEGA_METHOD_VOID(GetMethodInfo,6,((in),uint32_t,method_idx,(out),string_t&,strName,(out),TypeInfo::MethodAttributes_t&,attribs,(out),uint32_t&,timeout,(out),byte_t&,param_count,(out),Remoting::IMessage*&,return_type))
	OMEGA_METHOD_VOID(GetParamInfo,5,((in),uint32_t,method_idx,(in),byte_t,param_idx,(out),string_t&,strName,(out),Remoting::IMessage*&,type,(out),TypeInfo::ParamAttributes_t&,attribs))
	OMEGA_METHOD(byte_t,GetAttributeRef,3,((in),uint32_t,method_idx,(in),byte_t,param_idx,(in),TypeInfo::ParamAttributes_t,attrib))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, ICastException, Omega, IException, "{F79A88F6-B2C4-490F-A11D-7D9B3894BD5D}",

	OMEGA_METHOD(any_t,GetValue,0,())
	OMEGA_METHOD(any_t::CastResult_t,GetReason,0,())
	OMEGA_METHOD(Remoting::IMessage*,GetDestinationType,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IObjectFactory, "{1BE2A9DF-A7CF-445e-8A06-C02256C4A460}",

	OMEGA_METHOD_VOID(CreateInstance,2,((in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, ISystemException, Omega, IException, "{A0E1EEDB-BA00-4078-B67B-D990D43D5E7C}",

	OMEGA_METHOD(uint32_t,GetErrorCode,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, IInternalException, Omega, IException, "{6559CBE8-DB92-4D8A-8D28-75C40E9A1F93}",

	OMEGA_METHOD(string_t,GetSource,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, INotFoundException, Omega, IException, "{A851A685-A3AB-430b-BA52-E277655AC9CF}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, ITimeoutException, Omega, IException, "{63E8BFDE-D7AA-4675-B628-A1579B5AD8C7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, IAlreadyExistsException, Omega, IException, "{5EC948EA-D7F1-4733-80A3-FF4BF5B2F4A7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, IAccessDeniedException, Omega, IException, "{A752C1AF-68CB-4fab-926A-DFC3319CEDE1}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IRunningObjectTable, "{0A36F849-8DBC-49c6-9ECA-8AD71BF3C8D0}",

	// Methods
	OMEGA_METHOD(uint32_t,RegisterObject,3,((in),const any_t&,oid,(in),IObject*,pObject,(in),Activation::RegisterFlags_t,flags))
	OMEGA_METHOD_VOID(RevokeObject,1,((in),uint32_t,cookie))
	OMEGA_METHOD_VOID(GetObject,4,((in),const any_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject,(in),bool_t,remote))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Registry, IKey, "{F33E828A-BF5E-4c26-A541-BDB2CA736DBD}",

	// Methods
	OMEGA_METHOD(string_t,GetName,0,())
	OMEGA_METHOD(bool_t,IsKey,1,((in),const string_t&,key))
	OMEGA_METHOD(Omega::Registry::IKey::string_set_t,EnumSubKeys,0,())
	OMEGA_METHOD(Registry::IKey*,OpenKey,2,((in),const string_t&,key,(in),Registry::IKey::OpenFlags_t,flags))
	OMEGA_METHOD_VOID(DeleteSubKey,1,((in),const string_t&,strKey))
	OMEGA_METHOD(bool_t,IsValue,1,((in),const string_t&,name))
	OMEGA_METHOD(Omega::Registry::IKey::string_set_t,EnumValues,0,())
	OMEGA_METHOD(any_t,GetValue,1,((in),const string_t&,name))
	OMEGA_METHOD_VOID(SetValue,2,((in),const string_t&,name,(in),const any_t&,val))
	OMEGA_METHOD_VOID(DeleteValue,1,((in),const string_t&,strValue))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Registry, IOverlayKeyFactory, "{D83FC506-5939-AB15-6018-A55090AB03DE}",

	// Methods
	OMEGA_METHOD(Registry::IKey*,Overlay,2,((in),const string_t&,strOver,(in),const string_t&,strUnder))
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

#endif // OOCORE_IFACES_H_INCLUDED_
