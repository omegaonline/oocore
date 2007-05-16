#ifndef OOCORE_IFACES_H_INCLUDED_
#define OOCORE_IFACES_H_INCLUDED_

namespace Omega
{
	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateInstance(IObject* pOuter, const Omega::guid_t& iid, IObject*& pObject) = 0;
		};
		
		enum Flags
		{
			InProcess = 1,
			OutOfProcess = 2,
			Any = 3,
			DontLaunch = 0x10
		};
		typedef uint16_t Flags_t;

		inline IObjectFactory* GetObjectFactory(const guid_t& oid, Flags_t flags);
		inline guid_t NameToOid(const string_t& strObjectName);

		interface IOidNotFoundException : public IException
		{
			virtual guid_t GetMissingOid() = 0;
		};
		
		interface INoAggregationException : public IException
		{
			virtual guid_t GetFailingOid() = 0;

			inline static void Throw(const guid_t& oid, IException* pCause = 0);
		};
		
		interface ILibraryNotFoundException : public IException
		{
			virtual string_t GetLibraryName() = 0;
		};
		
		interface IServiceTable : public IObject
		{
			enum Flags
			{
				Default = 0,
				AllowAnyUser = 1
			};
			typedef uint16_t Flags_t;

			virtual void Register(const guid_t& oid, Flags_t flags, IObject* pObject) = 0;
			virtual void Revoke(const guid_t& oid) = 0;
			virtual void GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;

			inline static IServiceTable* GetServiceTable();
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
		interface IRegistryKey : public IObject
		{
			enum ValueType
			{
				String = 0,
				UInt32 = 1,
				Binary = 2
			};
			typedef byte_t ValueType_t;

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
			virtual uint32_t GetUIntValue(const string_t& name) = 0;
			virtual void GetBinaryValue(const string_t& name, uint32_t& cbLen, byte_t* pBuffer) = 0;
			virtual void SetStringValue(const string_t& name, const string_t& val) = 0;
			virtual void SetUIntValue(const string_t& name, uint32_t val) = 0;
			virtual void SetBinaryValue(const string_t& name, uint32_t cbLen, const byte_t* val) = 0;
			virtual ValueType_t GetValueType(const string_t& name) = 0;
			virtual IRegistryKey* OpenSubKey(const string_t& key, OpenFlags_t flags = OpenExisting) = 0;
			virtual IEnumString* EnumSubKeys() = 0;
			virtual IEnumString* EnumValues() = 0;
			virtual void DeleteKey(const string_t& strKey) = 0;
			virtual void DeleteValue(const string_t& strValue) = 0;

			inline static IRegistryKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting);
		};
		
		interface INotFoundException : public IException
		{
			virtual string_t GetValueName() = 0;
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
			virtual IRegistryKey::ValueType_t GetValueType() = 0;
		};
		
		interface IAccessDeniedException : public IException
		{
			virtual string_t GetName() = 0;
		};
	}

	inline IObject* CreateInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IObjectFactory, "{1BE2A9DF-A7CF-445e-8A06-C02256C4A460}",

	// Methods
	OMEGA_METHOD_VOID(CreateInstance,3,((in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega, INoInterfaceException, Omega, IException, "{68778245-9EA7-49f7-9EF4-D5D742E781D5}",

	OMEGA_METHOD(Omega::guid_t,GetUnsupportedIID,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, IOidNotFoundException, Omega, IException, "{162BBEBD-770B-4925-A8E7-48DEC7224ABE}",

	// Methods
	OMEGA_METHOD(Omega::guid_t,GetMissingOid,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, INoAggregationException, Omega, IException, "{A752C1AF-68CB-4fab-926A-DFC3319CEDE1}",

	// Methods
	OMEGA_METHOD(Omega::guid_t,GetFailingOid,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, ILibraryNotFoundException, Omega, IException, "{C7D970C0-D5D9-42e2-B927-E6E2E5624E50}",

	// Methods
	OMEGA_METHOD(Omega::string_t,GetLibraryName,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IServiceTable, "{0A36F849-8DBC-49c6-9ECA-8AD71BF3C8D0}",

	// Methods
	OMEGA_METHOD_VOID(Register,3,((in),const Omega::guid_t&,oid,(in),Omega::Activation::IServiceTable::Flags_t,flags,(in),Omega::IObject*,pObject))
	OMEGA_METHOD_VOID(Revoke,1,((in),const Omega::guid_t&,oid))
	OMEGA_METHOD_VOID(GetObject,3,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE
(
	Omega, IEnumString, "{0D796351-7197-4444-B6E0-74A669289D8F}",

	// Methods
	OMEGA_METHOD(bool_t,Next,2,((in_out),Omega::uint32_t&,count,(out)(size_is(count)),Omega::string_t*,parrVals))
	OMEGA_METHOD(bool_t,Skip,1,((in),Omega::uint32_t,count))
	OMEGA_METHOD_VOID(Reset,0,())
	OMEGA_METHOD(IEnumString*,Clone,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Registry, IRegistryKey, "{F33E828A-BF5E-4c26-A541-BDB2CA736DBD}",

	// Methods
	OMEGA_METHOD(Omega::bool_t,IsSubKey,1,((in),const Omega::string_t&,key))
	OMEGA_METHOD(Omega::bool_t,IsValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(Omega::string_t,GetStringValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(uint32_t,GetUIntValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD_VOID(GetBinaryValue,3,((in),const Omega::string_t&,name,(in_out),Omega::uint32_t&,cbLen,(out)(size_is(cbLen)),Omega::byte_t*,pBuffer))
	OMEGA_METHOD_VOID(SetStringValue,2,((in),const Omega::string_t&,name,(in),const Omega::string_t&,val))
	OMEGA_METHOD_VOID(SetUIntValue,2,((in),const Omega::string_t&,name,(in),Omega::uint32_t,val))
	OMEGA_METHOD_VOID(SetBinaryValue,3,((in),const Omega::string_t&,name,(in),Omega::uint32_t,cbLen,(in)(size_is(cbLen)),const Omega::byte_t*,val))
	OMEGA_METHOD(Omega::Registry::IRegistryKey::ValueType_t,GetValueType,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(Omega::Registry::IRegistryKey*,OpenSubKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IRegistryKey::OpenFlags_t,flags))
	OMEGA_METHOD(IEnumString*,EnumSubKeys,0,())
	OMEGA_METHOD(IEnumString*,EnumValues,0,())
	OMEGA_METHOD_VOID(DeleteKey,1,((in),const Omega::string_t&,strKey))
	OMEGA_METHOD_VOID(DeleteValue,1,((in),const Omega::string_t&,strValue))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, INotFoundException, Omega, IException, "{A851A685-A3AB-430b-BA52-E277655AC9CF}",

	// Methods
	OMEGA_METHOD(Omega::string_t,GetValueName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IWrongValueTypeException, Omega, IException, "{B7FF3FE7-11AF-4f62-9341-8470BCB8F0D7}",

	// Methods
	OMEGA_METHOD(Omega::Registry::IRegistryKey::ValueType_t,GetValueType,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IBadNameException, Omega, IException, "{5ADD9FB6-2044-40fd-9F3C-31E9B66B865E}",

	// Methods
	OMEGA_METHOD(Omega::string_t,GetName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IAlreadyExistsException, Omega, IException, "{5EC948EA-D7F1-4733-80A3-FF4BF5B2F4A7}",

	// Methods
	OMEGA_METHOD(Omega::string_t,GetKeyName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IAccessDeniedException, Omega, IException, "{08AE0A04-1765-493b-93A3-8738768F09BC}",
	
	// Methods
	OMEGA_METHOD(Omega::string_t,GetName,0,())
)

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IServiceTable*,Activation_GetServiceTable,0,());
Omega::Activation::IServiceTable* Omega::Activation::IServiceTable::GetServiceTable()
{
	return Activation_GetServiceTable();
}

OOCORE_EXPORTED_FUNCTION_VOID(INoInterfaceException_Throw,2,((in),const Omega::guid_t&,iid,(in),const Omega::char_t*,source));
void Omega::INoInterfaceException::Throw(const Omega::guid_t& iid, const char_t* source)
{
	INoInterfaceException_Throw(iid,source);
}

OOCORE_EXPORTED_FUNCTION_VOID(Activation_INoAggregationException_Throw,2,((in),const Omega::guid_t&,oid,(in),Omega::IException*,pCause));
void Omega::Activation::INoAggregationException::Throw(const Omega::guid_t& oid, Omega::IException* pCause)
{
	Activation_INoAggregationException_Throw(oid,pCause);
}

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,Activation_GetObjectFactory,2,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags));
Omega::Activation::IObjectFactory* Omega::Activation::GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags)
{
	return Activation_GetObjectFactory(oid,flags);
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,Activation_NameToOid,1,((in),const Omega::string_t&,strObjectName));
Omega::guid_t Omega::Activation::NameToOid(const Omega::string_t& strObjectName)
{
	return Activation_NameToOid(strObjectName);
}

OOCORE_EXPORTED_FUNCTION(Omega::Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IRegistryKey::OpenFlags_t,flags));
Omega::Registry::IRegistryKey* Omega::Registry::IRegistryKey::OpenKey(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags)
{
	return IRegistryKey_OpenKey(key,flags);
}

OOCORE_EXPORTED_FUNCTION_VOID(Omega_CreateInstance,5,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
Omega::IObject* Omega::CreateInstance(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter, const Omega::guid_t& iid)
{
	IObject* pObj = 0;
	Omega_CreateInstance(oid,flags,pOuter,iid,pObj);
	return pObj;
}

#endif // OOCORE_IFACES_H_INCLUDED_
