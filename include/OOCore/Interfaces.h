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
	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateInstance(IObject* pOuter, guid_t& iid, IObject*& pObject) = 0;
		};
		
		enum Flags
		{
			InProcess = 1,
			OutOfProcess = 2,
			Any = (InProcess | OutOfProcess),
			RemoteServer = 4,
			DontLaunch = 8
		};
		typedef uint16_t Flags_t;

		enum RegisterFlags
		{
			MultipleUse = 0,
			SingleUse = 1,              // Auto Revoke after 1st GetObject
			MultipleRegistration = 2,   // Allow multiple calls to Register with different flags
			Suspended = 4
		};
		typedef uint16_t RegisterFlags_t;

		inline uint32_t RegisterObject(const guid_t& oid, IObject* pObject, Activation::Flags_t flags, Activation::RegisterFlags_t reg_flags);
		inline IObject* GetRegisteredObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid);
		inline void RevokeObject(uint32_t cookie);

		inline guid_t NameToOid(const string_t& strObjectName);
		
		interface IOidNotFoundException : public IException
		{
			virtual guid_t GetMissingOid() = 0;
		};
		
		interface INoAggregationException : public IException
		{
			virtual guid_t GetFailingOid() = 0;

			inline static INoAggregationException* Create(const guid_t& oid);
		};
		
		interface ILibraryNotFoundException : public IException
		{
			virtual string_t GetLibraryName() = 0;
		};
		
		// WARNING - THIS IS GOING TO CHANGE!!!
		interface IRunningObjectTable : public IObject
		{
			virtual uint32_t Register(const guid_t& oid, IObject* pObject) = 0;
			virtual void Revoke(uint32_t cookie) = 0;
			virtual IObject* GetObject(const guid_t& oid) = 0;

			inline static IRunningObjectTable* GetRunningObjectTable();
		};
	}

	interface IEnumString : public IObject
	{
		virtual bool_t Next(uint32_t& count, string_t* parrVals) = 0;
		virtual bool_t Skip(uint32_t count) = 0;
		virtual void Reset() = 0;
		virtual IEnumString* Clone() = 0;
	};
	
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
				Create = 1,
				FailIfThere = 2
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
			virtual IEnumString* EnumSubKeys() = 0;
			virtual IEnumString* EnumValues() = 0;
			virtual void DeleteKey(const string_t& strKey) = 0;
			virtual void DeleteValue(const string_t& strValue) = 0;

			inline static IKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting);
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

		inline void AddXML(const string_t& strXML, bool_t bAdd = true, const string_t& strSubstitutions = L"");
	}

	namespace IO
	{
		interface IStream : public IObject
		{
			virtual void ReadBytes(uint64_t& cbBytes, byte_t* val) = 0;
			virtual void WriteBytes(const uint64_t& cbBytes, const byte_t* val) = 0;
		};

		interface IAsyncStreamNotify : public IObject
		{
			virtual void OnOpened() = 0;
			virtual void OnRead(const uint64_t& cbBytes, const byte_t* pData) = 0;
			virtual void OnWritten(const uint64_t& cbBytes) = 0;
			virtual void OnError(IException* pE) = 0;
		};

		inline IStream* OpenStream(const string_t& strEndpoint, IAsyncStreamNotify* pNotify = 0);
	}

	namespace Net
	{
		interface IConnectedStream : public IO::IStream
		{
			virtual string_t RemoteEndpoint() = 0;
			virtual string_t LocalEndpoint() = 0;
		};

		// This may well change!!  You have been warned
		interface IProtocolHandler : public IObject
		{
			virtual IConnectedStream* OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify) = 0;
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IObjectFactory, "{1BE2A9DF-A7CF-445e-8A06-C02256C4A460}",

	// Methods
	OMEGA_METHOD_VOID(CreateInstance,3,((in),IObject*,pOuter,(in_out),guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, ISystemException, Omega, IException, "{A0E1EEDB-BA00-4078-B67B-D990D43D5E7C}",

	OMEGA_METHOD(uint32_t,ErrorCode,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, INoInterfaceException, Omega, IException, "{68778245-9EA7-49f7-9EF4-D5D742E781D5}",

	OMEGA_METHOD(guid_t,GetUnsupportedIID,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, IOidNotFoundException, Omega, IException, "{162BBEBD-770B-4925-A8E7-48DEC7224ABE}",

	// Methods
	OMEGA_METHOD(guid_t,GetMissingOid,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, INoAggregationException, Omega, IException, "{A752C1AF-68CB-4fab-926A-DFC3319CEDE1}",

	// Methods
	OMEGA_METHOD(guid_t,GetFailingOid,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Activation, ILibraryNotFoundException, Omega, IException, "{C7D970C0-D5D9-42e2-B927-E6E2E5624E50}",

	// Methods
	OMEGA_METHOD(string_t,GetLibraryName,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IRunningObjectTable, "{0A36F849-8DBC-49c6-9ECA-8AD71BF3C8D0}",

	// Methods
	OMEGA_METHOD(uint32_t,Register,2,((in),const guid_t&,oid,(in),IObject*,pObject))
	OMEGA_METHOD_VOID(Revoke,1,((in),uint32_t,cookie))
	OMEGA_METHOD(IObject*,GetObject,1,((in),const guid_t&,oid))
)

OMEGA_DEFINE_INTERFACE
(
	Omega, IEnumString, "{0D796351-7197-4444-B6E0-74A669289D8F}",

	// Methods
	OMEGA_METHOD(bool_t,Next,2,((in_out),uint32_t&,count,(out)(size_is(count)),string_t*,parrVals))
	OMEGA_METHOD(bool_t,Skip,1,((in),uint32_t,count))
	OMEGA_METHOD_VOID(Reset,0,())
	OMEGA_METHOD(IEnumString*,Clone,0,())
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
	OMEGA_METHOD(IEnumString*,EnumSubKeys,0,())
	OMEGA_METHOD(IEnumString*,EnumValues,0,())
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
	OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint64_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
	OMEGA_METHOD_VOID(WriteBytes,2,((in),const uint64_t&,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::IO, IAsyncStreamNotify, "{1E587515-AE98-45ef-9E74-497784169F38}",
	
	// Methods
	OMEGA_METHOD_VOID(OnOpened,0,())
	OMEGA_METHOD_EX_VOID(Remoting::Asynchronous,0,OnRead,2,((in),const uint64_t&,cbBytes,(in)(size_is(cbBytes)),const byte_t*,pData))
	OMEGA_METHOD_EX_VOID(Remoting::Asynchronous,0,OnWritten,1,((in),const uint64_t&,cbBytes))
	OMEGA_METHOD_EX_VOID(Remoting::Asynchronous,0,OnError,1,((in),IException*,pE))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Net, IConnectedStream, Omega::IO, IStream, "{C5C3AB92-9127-4bb5-9AA8-AA0953843E5A}",
	
	// Methods
	OMEGA_METHOD(string_t,RemoteEndpoint,0,())
	OMEGA_METHOD(string_t,LocalEndpoint,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net, IProtocolHandler, "{76416648-0AFE-4474-BD8F-FEB033F17EAF}",
	
	// Methods
	OMEGA_METHOD(Net::IConnectedStream*,OpenStream,2,((in),const string_t&,strEndpoint,(in),IO::IAsyncStreamNotify*,pNotify))
)

OMEGA_EXPORTED_FUNCTION(Omega::Activation::IRunningObjectTable*,Activation_GetRunningObjectTable,0,())
Omega::Activation::IRunningObjectTable* Omega::Activation::IRunningObjectTable::GetRunningObjectTable()
{
	return Activation_GetRunningObjectTable();
}

OMEGA_EXPORTED_FUNCTION(Omega::Activation::INoAggregationException*,Activation_INoAggregationException_Create,1,((in),const Omega::guid_t&,oid));
Omega::Activation::INoAggregationException* Omega::Activation::INoAggregationException::Create(const Omega::guid_t& oid)
{
	return Activation_INoAggregationException_Create(oid);
}

OMEGA_EXPORTED_FUNCTION(Omega::uint32_t,Activation_RegisterObject,4,((in),const Omega::guid_t&,oid,(in),Omega::IObject*,pObject,(in),Omega::Activation::Flags_t,flags,(in),Omega::Activation::RegisterFlags_t,reg_flags));
Omega::uint32_t Omega::Activation::RegisterObject(const Omega::guid_t& oid, Omega::IObject* pObject, Omega::Activation::Flags_t flags, Omega::Activation::RegisterFlags_t reg_flags)
{
	return Activation_RegisterObject(oid,pObject,flags,reg_flags);
}

OMEGA_EXPORTED_FUNCTION_VOID(Activation_GetRegisteredObject,4,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
Omega::IObject* Omega::Activation::GetRegisteredObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid)
{
	Omega::IObject* pObject = 0;
	Activation_GetRegisteredObject(oid,flags,iid,pObject);
	return pObject;
}

OMEGA_EXPORTED_FUNCTION_VOID(Activation_RevokeObject,1,((in),Omega::uint32_t,cookie));
void Omega::Activation::RevokeObject(Omega::uint32_t cookie)
{
	Activation_RevokeObject(cookie);
}

OMEGA_EXPORTED_FUNCTION(Omega::guid_t,Activation_NameToOid,1,((in),const Omega::string_t&,strObjectName));
Omega::guid_t Omega::Activation::NameToOid(const Omega::string_t& strObjectName)
{
	return Activation_NameToOid(strObjectName);
}

OMEGA_EXPORTED_FUNCTION_VOID(Registry_AddXML,3,((in),const Omega::string_t&,strXML,(in),Omega::bool_t,bAdd,(in),const Omega::string_t&,strSubstitutions));
void Omega::Registry::AddXML(const Omega::string_t& strXML, Omega::bool_t bAdd, const Omega::string_t& strSubstitutions)
{
	Registry_AddXML(strXML,bAdd,strSubstitutions);
}

OMEGA_EXPORTED_FUNCTION(Omega::Registry::IKey*,IRegistryKey_OpenKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IKey::OpenFlags_t,flags));
Omega::Registry::IKey* Omega::Registry::IKey::OpenKey(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags)
{
	return IRegistryKey_OpenKey(key,flags);
}

OMEGA_EXPORTED_FUNCTION(Omega::IO::IStream*,Omega_IO_OpenStream,2,((in),const Omega::string_t&,strEndpoint,(in),Omega::IO::IAsyncStreamNotify*,pNotify));
Omega::IO::IStream* Omega::IO::OpenStream(const Omega::string_t& strEndpoint, Omega::IO::IAsyncStreamNotify* pNotify)
{
	return Omega_IO_OpenStream(strEndpoint,pNotify);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_IFACES_H_INCLUDED_
