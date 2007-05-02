#ifndef OOCORE_IFACES_H_INCLUDED_
#define OOCORE_IFACES_H_INCLUDED_

namespace Omega
{
	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateObject(IObject* pOuter, const Omega::guid_t& iid, IObject*& pObject) = 0;
		};
		OMEGA_DECLARE_IID(IObjectFactory);

		enum Flags
		{
			InProcess = 1,
			OutOfProcess = 2,
			Any = 3,
			DontLaunch = 0x10
		};
		typedef uint16_t Flags_t;

		inline IObjectFactory* GetObjectFactory(const guid_t& oid, Flags_t flags);
		inline IObject* CreateObject(const guid_t& oid, Flags_t flags, IObject* pOuter, const guid_t& iid);
		inline guid_t NameToOid(const string_t& strObjectName);

		interface IOidNotFoundException : public IException
		{
			virtual guid_t GetMissingOid() = 0;
		};
		OMEGA_DECLARE_IID(IOidNotFoundException);

		interface INoAggregationException : public IException
		{
			virtual guid_t GetFailingOid() = 0;

			inline static void Throw(const guid_t& oid, IException* pCause = 0);
		};
		OMEGA_DECLARE_IID(INoAggregationException);

		interface ILibraryNotFoundException : public IException
		{
			virtual string_t GetLibraryName() = 0;
		};
		OMEGA_DECLARE_IID(ILibraryNotFoundException);

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
		OMEGA_DECLARE_IID(IServiceTable);
	}

	interface IEnumString : public IObject
	{
		virtual bool_t Next(uint32_t& count, string_t* parrVals) = 0;
		virtual bool_t Skip(uint32_t count) = 0;
		virtual void Reset() = 0;
		virtual IEnumString* Clone() = 0;
	};
	OMEGA_DECLARE_IID(IEnumString);

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
		OMEGA_DECLARE_IID(IRegistryKey);

		interface INotFoundException : public IException
		{
			virtual string_t GetValueName() = 0;
		};
		OMEGA_DECLARE_IID(INotFoundException);

		interface IAlreadyExistsException : public IException
		{
			virtual string_t GetKeyName() = 0;
		};
		OMEGA_DECLARE_IID(IAlreadyExistsException);

		interface IBadNameException : public IException
		{
			virtual string_t GetName() = 0;
		};
		OMEGA_DECLARE_IID(IBadNameException);

        interface IWrongValueTypeException : public IException
		{
			virtual IRegistryKey::ValueType_t GetValueType() = 0;
		};
		OMEGA_DECLARE_IID(IWrongValueTypeException);

		interface IAccessDeniedException : public IException
		{
			virtual string_t GetName() = 0;
		};
		OMEGA_DECLARE_IID(IAccessDeniedException);
	}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IObjectFactory,
	0xd94853ed, 0x35c6, 0x4594, 0x88, 0x2, 0x33, 0xf2, 0x1a, 0xbf, 0xbe, 0xbe,

	// Methods
	OMEGA_METHOD_VOID(CreateObject,3,((in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega, INoInterfaceException, Omega, IException,
	0x5634e9bf, 0x50e7, 0x47a1, 0xb0, 0xbb, 0x9c, 0xf1, 0x64, 0x12, 0x24, 0x4e,

	OMEGA_METHOD(Omega::guid_t,GetUnsupportedIID,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, IOidNotFoundException, Omega, IException,
	0xcf1e01c0, 0x458, 0x41a4, 0x87, 0xa8, 0xbf, 0x86, 0x62, 0xb2, 0x4d, 0x76,

	// Methods
	OMEGA_METHOD(Omega::guid_t,GetMissingOid,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, INoAggregationException, Omega, IException,
	0x327157ac, 0xe474, 0x4c19, 0x9b, 0xb, 0x8e, 0x73, 0xa2, 0x58, 0x91, 0xce,

	// Methods
	OMEGA_METHOD(Omega::guid_t,GetFailingOid,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Activation, ILibraryNotFoundException, Omega, IException,
	0x1b1baeb6, 0x7fb9, 0x4373, 0x8f, 0x34, 0x48, 0x3f, 0x15, 0xfa, 0x54, 0x21,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetLibraryName,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IServiceTable,
	0x8e26d026, 0x9988, 0x4f69, 0x93, 0xc3, 0xc4, 0x72, 0x43, 0x4f, 0x9d, 0xde,

	// Methods
	OMEGA_METHOD_VOID(Register,3,((in),const Omega::guid_t&,oid,(in),Omega::Activation::IServiceTable::Flags_t,flags,(in),Omega::IObject*,pObject))
	OMEGA_METHOD_VOID(Revoke,1,((in),const Omega::guid_t&,oid))
	OMEGA_METHOD_VOID(GetObject,3,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE
(
	Omega, IEnumString,
	0x154dd0d9, 0xc452, 0x4847, 0xb4, 0xf9, 0xda, 0x64, 0xc0, 0x22, 0xb2, 0x43,

	// Methods
	OMEGA_METHOD(bool_t,Next,2,((in_out),Omega::uint32_t&,count,(out)(size_is(count)),Omega::string_t*,parrVals))
	OMEGA_METHOD(bool_t,Skip,1,((in),Omega::uint32_t,count))
	OMEGA_METHOD_VOID(Reset,0,())
	OMEGA_METHOD(IEnumString*,Clone,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Registry, IRegistryKey,
	0x8bc942a4, 0xf2f1, 0x4ef9, 0xbf, 0x9c, 0xd9, 0x20, 0xbe, 0x56, 0xeb, 0x9c,

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
	Omega::Registry, INotFoundException, Omega, IException,
	0x4ab1051, 0x81cf, 0x4c8c, 0x89, 0xd8, 0x21, 0x7f, 0x3c, 0x9a, 0x42, 0x49,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetValueName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IWrongValueTypeException, Omega, IException,
	0x96b103c, 0x360f, 0x4ada, 0x80, 0x6f, 0xd3, 0x37, 0xd6, 0x32, 0x2e, 0xb6,

	// Methods
	OMEGA_METHOD(Omega::Registry::IRegistryKey::ValueType_t,GetValueType,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IBadNameException, Omega, IException,
	0x18677696, 0x219c, 0x4e0a, 0xa9, 0x97, 0xc7, 0x56, 0x50, 0xe8, 0x46, 0x6e,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IAlreadyExistsException, Omega, IException,
	0xee80fdeb, 0xc1ea, 0x49d1, 0x9f, 0x76, 0x9a, 0x1b, 0xb4, 0xdc, 0x97, 0x6,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetKeyName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IAccessDeniedException, Omega, IException,
	0xcc5f1542, 0x1475, 0x481f, 0x82, 0xdc, 0x96, 0x6e, 0x5b, 0x87, 0xc, 0x99,

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

OOCORE_EXPORTED_FUNCTION_VOID(Activation_CreateObject,5,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
Omega::IObject* Omega::Activation::CreateObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter, const Omega::guid_t& iid)
{
	Omega::IObject* pObj = 0;
	Activation_CreateObject(oid,flags,pOuter,iid,pObj);
	return pObj;
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

#endif // OOCORE_IFACES_H_INCLUDED_
