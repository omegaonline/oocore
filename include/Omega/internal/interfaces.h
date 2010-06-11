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
	interface ICastException : public IException
	{
		virtual any_t GetValue() = 0;
		virtual any_t::CastResult_t GetReason() = 0;
		virtual Remoting::IMessage* GetDestinationType() = 0;
	};

	namespace Formatting
	{
		interface IFormattingException : public IException
		{
			static IFormattingException* Create(const string_t& strMsg);
		};
	}

	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject) = 0;
		};

		enum Flags
		{
			InProcess = 1,                         ///< Use dll/so if available
			OutOfProcess = 2,                      ///< Use exe if available
			Any = (InProcess | OutOfProcess),
						
			// Only one of the following...
			Surrogate = 8,                         ///< Launch dll/so in a surrogate wrapper
			PrivateSurrogate = (0x10 | Surrogate), ///< Launch dll/so in its own surrogate wrapper
			Sandbox = 0x20,                        ///< Launch as the sandbox user - implies surrogate if dll/so
			VM = 0x40,                             ///< Launch in the virtual machine - implies surrogate if dll/so

			// Add this for CreateInstance() if you want...
			RemoteActivation = 0x4000,             ///< Request is from a remote machine
			DontLaunch = 0x8000                    ///< Do not launch exe/dll/so if not already running		
		};
		typedef uint16_t Flags_t;

		IObjectFactory* GetObjectFactory(const any_t& oid, Activation::Flags_t flags);

		enum RegisterFlags
		{
			ProcessLocal = 1,    // Register for this process only
			UserLocal = 2,       // Register for this user only
			MachineLocal = 4,    // Register for this machine only
			Anywhere = 8,        // Register publicly

			MultipleUse = 0x0,
			SingleUse = 0x10,            // Auto Revoke after 1st GetObject
			MultipleRegistration = 0x20  // Allow multiple calls to Register with different flags
		};
		typedef uint16_t RegisterFlags_t;

		interface IOidNotFoundException : public IException
		{
			virtual any_t GetMissingOid() = 0;

			static IOidNotFoundException* Create(const any_t& oid);
		};

		interface INoAggregationException : public IException
		{
			virtual any_t GetFailingOid() = 0;

			static INoAggregationException* Create(const any_t& oid);
		};

		interface ILibraryNotFoundException : public IException
		{
			virtual string_t GetLibraryName() = 0;
		};

		interface IDuplicateRegistrationException : public IException
		{
			virtual any_t GetOid() = 0;
		};

		interface IRunningObjectTable : public IObject
		{
			virtual uint32_t RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags) = 0;
			virtual void RevokeObject(uint32_t cookie) = 0;
			virtual void GetObject(const any_t& oid, Activation::RegisterFlags_t flags, const guid_t& iid, IObject*& pObject) = 0;

			static IRunningObjectTable* GetRunningObjectTable();
		};
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
		enum ValueType
		{
			String = 0,
			Integer = 1,
			Binary = 2
		};
		typedef byte_t ValueType_t;

		interface IKey : public IObject
		{
			enum OpenFlags
			{
				OpenExisting = 0,
				OpenCreate = 1,
				CreateNew = 2
			};
			typedef uint16_t OpenFlags_t;

			virtual bool_t IsSubKey(const string_t& key) = 0;
			virtual bool_t IsValue(const string_t& name) = 0;
			virtual string_t GetStringValue(const string_t& name) = 0;
			virtual int64_t GetIntegerValue(const string_t& name) = 0;
			virtual void GetBinaryValue(const string_t& name, uint32_t& cbLen, byte_t* pBuffer) = 0;
			virtual void SetStringValue(const string_t& name, const string_t& val) = 0;
			virtual void SetIntegerValue(const string_t& name, const int64_t& val) = 0;
			virtual void SetBinaryValue(const string_t& name, uint32_t cbLen, const byte_t* val) = 0;
			virtual string_t GetDescription() = 0;
			virtual string_t GetValueDescription(const string_t& name) = 0;
			virtual void SetDescription(const string_t& desc) = 0;
			virtual void SetValueDescription(const string_t& name, const string_t& desc) = 0;
			virtual ValueType_t GetValueType(const string_t& name) = 0;
			virtual IKey* OpenSubKey(const string_t& key, OpenFlags_t flags = OpenExisting) = 0;
			virtual std::set<string_t> EnumSubKeys() = 0;
			virtual std::set<string_t> EnumValues() = 0;
			virtual void DeleteKey(const string_t& strKey) = 0;
			virtual void DeleteValue(const string_t& strValue) = 0;

			static IKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting);
		};

		interface INotFoundException : public IException
		{
			virtual string_t GetName() = 0;
		};

		interface IAlreadyExistsException : public IException
		{
			virtual string_t GetKeyName() = 0;
		};

		interface IBadNameException : public IException
		{
			virtual string_t GetName() = 0;
		};

		interface IWrongValueTypeException : public IException
		{
			virtual string_t GetValueName() = 0;
			virtual ValueType_t GetValueType() = 0;
		};

		interface IAccessDeniedException : public IException
		{
			virtual string_t GetKeyName() = 0;
		};
	}

	namespace IO
	{
		interface IStream : public IObject
		{
			virtual void ReadBytes(uint32_t& cbBytes, byte_t* val) = 0;
			virtual void WriteBytes(const uint32_t& cbBytes, const byte_t* val) = 0;
		};

		interface IAsyncStreamNotify : public IObject
		{
			virtual void OnOpened() = 0;
			virtual void OnRead(uint32_t cbBytes, const byte_t* pData) = 0;
			virtual void OnWritten(uint32_t cbBytes) = 0;
			virtual void OnError(IException* pE) = 0;
		};

		IStream* OpenStream(const string_t& strEndpoint, IAsyncStreamNotify* pNotify = 0);
	}

	namespace Net
	{
		interface IConnectedStream : public IO::IStream
		{
			virtual string_t GetRemoteEndpoint() = 0;
			virtual string_t GetLocalEndpoint() = 0;
		};

		// This may well change!!  You have been warned
		interface IProtocolHandler : public IObject
		{
			virtual IConnectedStream* OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify) = 0;
		};
	}

	namespace TypeInfo
	{
		interface IProvideObjectInfo : public IObject
		{
			virtual std::set<guid_t> EnumInterfaces() = 0;
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

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Formatting, IFormattingException, Omega, IException, "{EBCD8903-5C9B-4d48-BC3B-0427A4E294C4}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IObjectFactory, "{1BE2A9DF-A7CF-445e-8A06-C02256C4A460}",

	OMEGA_METHOD_VOID(CreateInstance,3,((in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
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
	Omega, INoInterfaceException, Omega, IException, "{68778245-9EA7-49f7-9EF4-D5D742E781D5}",

	OMEGA_METHOD(guid_t,GetUnsupportedIID,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, ITimeoutException, Omega, IException, "{63E8BFDE-D7AA-4675-B628-A1579B5AD8C7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, IOidNotFoundException, Omega, IException, "{162BBEBD-770B-4925-A8E7-48DEC7224ABE}",

	// Methods
	OMEGA_METHOD(any_t,GetMissingOid,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, INoAggregationException, Omega, IException, "{A752C1AF-68CB-4fab-926A-DFC3319CEDE1}",

	// Methods
	OMEGA_METHOD(any_t,GetFailingOid,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, ILibraryNotFoundException, Omega, IException, "{C7D970C0-D5D9-42e2-B927-E6E2E5624E50}",

	// Methods
	OMEGA_METHOD(string_t,GetLibraryName,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, IDuplicateRegistrationException, Omega, IException, "{35495CBA-13B0-4d56-BAA4-3DF328A3F1EE}",

	// Methods
	OMEGA_METHOD(any_t,GetOid,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IRunningObjectTable, "{0A36F849-8DBC-49c6-9ECA-8AD71BF3C8D0}",

	// Methods
	OMEGA_METHOD(uint32_t,RegisterObject,3,((in),const any_t&,oid,(in),IObject*,pObject,(in),Activation::RegisterFlags_t,flags))
	OMEGA_METHOD_VOID(RevokeObject,1,((in),uint32_t,cookie))
	OMEGA_METHOD_VOID(GetObject,4,((in),const any_t&,oid,(in),Activation::RegisterFlags_t,flags,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Registry, IKey, "{F33E828A-BF5E-4c26-A541-BDB2CA736DBD}",

	// Methods
	OMEGA_METHOD(bool_t,IsSubKey,1,((in),const string_t&,key))
	OMEGA_METHOD(bool_t,IsValue,1,((in),const string_t&,name))
	OMEGA_METHOD(string_t,GetStringValue,1,((in),const string_t&,name))
	OMEGA_METHOD(int64_t,GetIntegerValue,1,((in),const string_t&,name))
	OMEGA_METHOD_VOID(GetBinaryValue,3,((in),const string_t&,name,(in_out),uint32_t&,cbLen,(out)(size_is(cbLen)),byte_t*,pBuffer))
	OMEGA_METHOD_VOID(SetStringValue,2,((in),const string_t&,name,(in),const string_t&,val))
	OMEGA_METHOD_VOID(SetIntegerValue,2,((in),const string_t&,name,(in),const int64_t&,val))
	OMEGA_METHOD_VOID(SetBinaryValue,3,((in),const string_t&,name,(in),uint32_t,cbLen,(in)(size_is(cbLen)),const byte_t*,val))
	OMEGA_METHOD(string_t,GetDescription,0,())
	OMEGA_METHOD(string_t,GetValueDescription,1,((in),const string_t&,name))
	OMEGA_METHOD_VOID(SetDescription,1,((in),const string_t&,desc))
	OMEGA_METHOD_VOID(SetValueDescription,2,((in),const string_t&,name,(in),const string_t&,desc))
	OMEGA_METHOD(Registry::ValueType_t,GetValueType,1,((in),const string_t&,name))
	OMEGA_METHOD(Registry::IKey*,OpenSubKey,2,((in),const string_t&,key,(in),Registry::IKey::OpenFlags_t,flags))
	OMEGA_METHOD(std::set<string_t>,EnumSubKeys,0,())
	OMEGA_METHOD(std::set<string_t>,EnumValues,0,())
	OMEGA_METHOD_VOID(DeleteKey,1,((in),const string_t&,strKey))
	OMEGA_METHOD_VOID(DeleteValue,1,((in),const string_t&,strValue))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Registry, INotFoundException, Omega, IException, "{A851A685-A3AB-430b-BA52-E277655AC9CF}",

	// Methods
	OMEGA_METHOD(string_t,GetName,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Registry, IWrongValueTypeException, Omega, IException, "{B7FF3FE7-11AF-4f62-9341-8470BCB8F0D7}",

	// Methods
	OMEGA_METHOD(Registry::ValueType_t,GetValueType,0,())
	OMEGA_METHOD(string_t,GetValueName,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Registry, IBadNameException, Omega, IException, "{5ADD9FB6-2044-40fd-9F3C-31E9B66B865E}",

	// Methods
	OMEGA_METHOD(string_t,GetName,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Registry, IAlreadyExistsException, Omega, IException, "{5EC948EA-D7F1-4733-80A3-FF4BF5B2F4A7}",

	// Methods
	OMEGA_METHOD(string_t,GetKeyName,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Registry, IAccessDeniedException, Omega, IException, "{08AE0A04-1765-493b-93A3-8738768F09BC}",

	// Methods
	OMEGA_METHOD(string_t,GetKeyName,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::IO, IStream, "{D1072F9B-3E7C-4724-9246-46DC111AE69F}",

	// Methods
	OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
	OMEGA_METHOD_VOID(WriteBytes,2,((in),const uint32_t&,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::IO, IAsyncStreamNotify, "{1E587515-AE98-45ef-9E74-497784169F38}",

	// Methods
	OMEGA_METHOD_VOID(OnOpened,0,())
	OMEGA_METHOD_EX_VOID(Asynchronous,0,OnRead,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,pData))
	OMEGA_METHOD_EX_VOID(Asynchronous,0,OnWritten,1,((in),uint32_t,cbBytes))
	OMEGA_METHOD_EX_VOID(Asynchronous,0,OnError,1,((in),IException*,pE))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Net, IConnectedStream, Omega::IO, IStream, "{C5C3AB92-9127-4bb5-9AA8-AA0953843E5A}",

	// Methods
	OMEGA_METHOD(string_t,GetRemoteEndpoint,0,())
	OMEGA_METHOD(string_t,GetLocalEndpoint,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net, IProtocolHandler, "{76416648-0AFE-4474-BD8F-FEB033F17EAF}",

	// Methods
	OMEGA_METHOD(Net::IConnectedStream*,OpenStream,2,((in),const string_t&,strEndpoint,(in),IO::IAsyncStreamNotify*,pNotify))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::TypeInfo, IProvideObjectInfo, "{F66A857D-C474-4c9e-B08B-68135AC8459E}",

	// Methods
	OMEGA_METHOD(std::set<guid_t>,EnumInterfaces,0,())
)

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IRunningObjectTable*,OOCore_Activation_GetRunningObjectTable,0,())
inline Omega::Activation::IRunningObjectTable* Omega::Activation::IRunningObjectTable::GetRunningObjectTable()
{
	return OOCore_Activation_GetRunningObjectTable();
}

OOCORE_EXPORTED_FUNCTION(Omega::Activation::INoAggregationException*,OOCore_Activation_INoAggregationException_Create,1,((in),const Omega::any_t&,oid));
inline Omega::Activation::INoAggregationException* Omega::Activation::INoAggregationException::Create(const Omega::any_t& oid)
{
	return OOCore_Activation_INoAggregationException_Create(oid);
}

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IOidNotFoundException*,OOCore_Activation_IOidNotFoundException_Create,1,((in),const Omega::any_t&,oid));
inline Omega::Activation::IOidNotFoundException* Omega::Activation::IOidNotFoundException::Create(const Omega::any_t& oid)
{
	return OOCore_Activation_IOidNotFoundException_Create(oid);
}

OOCORE_EXPORTED_FUNCTION(Omega::Registry::IKey*,OOCore_IRegistryKey_OpenKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IKey::OpenFlags_t,flags));
inline Omega::Registry::IKey* Omega::Registry::IKey::OpenKey(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags)
{
	return OOCore_IRegistryKey_OpenKey(key,flags);
}

OOCORE_EXPORTED_FUNCTION(Omega::IO::IStream*,OOCore_IO_OpenStream,2,((in),const Omega::string_t&,strEndpoint,(in),Omega::IO::IAsyncStreamNotify*,pNotify));
inline Omega::IO::IStream* Omega::IO::OpenStream(const Omega::string_t& strEndpoint, Omega::IO::IAsyncStreamNotify* pNotify)
{
	return OOCore_IO_OpenStream(strEndpoint,pNotify);
}

OOCORE_EXPORTED_FUNCTION(Omega::TypeInfo::IInterfaceInfo*,OOCore_TypeInfo_GetInterfaceInfo,2,((in),const Omega::guid_t&,iid,(in),Omega::IObject*,pObject));
inline Omega::TypeInfo::IInterfaceInfo* Omega::TypeInfo::GetInterfaceInfo(const Omega::guid_t& iid, Omega::IObject* pObject)
{
	return OOCore_TypeInfo_GetInterfaceInfo(iid,pObject);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_IFACES_H_INCLUDED_
