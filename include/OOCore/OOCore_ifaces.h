#ifndef OOCORE_IFACES_H_INCLUDED_
#define OOCORE_IFACES_H_INCLUDED_

namespace Omega
{
	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateObject(IObject* pOuter, const Omega::guid_t& iid, IObject** ppObject) = 0;
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

		IObject* GetObjectFactory(const guid_t& oid, Flags_t flags, const guid_t& iid);
		IObject* CreateObject(const guid_t& oid, Flags_t flags, IObject* pOuter, const guid_t& iid);
		guid_t NameToOid(const string_t& strObjectName);

		interface IOidNotFoundException : public IException
		{
			virtual guid_t GetMissingOid() = 0;

			static void Throw(const guid_t& oid, IException* pCause = 0);
		};
		OMEGA_DECLARE_IID(IOidNotFoundException);

		interface INoAggregationException : public IException
		{
			virtual guid_t GetFailingOid() = 0;

			static void Throw(const guid_t& oid, IException* pCause = 0);
		};
		OMEGA_DECLARE_IID(INoAggregationException);

		interface ILibraryNotFoundException : public IException
		{
			virtual string_t GetLibraryName() = 0;
		};
		OMEGA_DECLARE_IID(ILibraryNotFoundException);
	}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IObjectFactory, 
	0xd94853ed, 0x35c6, 0x4594, 0x88, 0x2, 0x33, 0xf2, 0x1a, 0xbf, 0xbe, 0xbe,

	// Methods
	OMEGA_METHOD_VOID(CreateObject,3,((in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),IObject**,ppObject))
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

OOCORE_EXPORTED_FUNCTION_VOID(INoInterfaceException_Throw,2,((in),const Omega::guid_t&,iid,(in),const Omega::char_t*,source));
inline void Omega::INoInterfaceException::Throw(const Omega::guid_t& iid, const char_t* source)
{
	INoInterfaceException_Throw(iid,source);
}

OOCORE_EXPORTED_FUNCTION_VOID(Activation_IOidNotFoundException_Throw,2,((in),const Omega::guid_t&,oid,(in),Omega::IException*,pCause));
inline void Omega::Activation::IOidNotFoundException::Throw(const Omega::guid_t& oid, Omega::IException* pCause)
{
	Activation_IOidNotFoundException_Throw(oid,pCause);
}

OOCORE_EXPORTED_FUNCTION_VOID(Activation_INoAggregationException_Throw,2,((in),const Omega::guid_t&,oid,(in),Omega::IException*,pCause));
inline void Omega::Activation::INoAggregationException::Throw(const Omega::guid_t& oid, Omega::IException* pCause)
{
	Activation_INoAggregationException_Throw(oid,pCause);
}

OOCORE_EXPORTED_FUNCTION_VOID(Activation_GetObjectFactory,4,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject**,ppObject));
inline Omega::IObject* Omega::Activation::GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid)
{
	Omega::IObject* pObj = 0;
	Activation_GetObjectFactory(oid,flags,iid,&pObj);
	return pObj;
}

OOCORE_EXPORTED_FUNCTION_VOID(Activation_CreateObject,5,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject**,ppObject));
inline Omega::IObject* Omega::Activation::CreateObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter, const Omega::guid_t& iid)
{
	Omega::IObject* pObj = 0;
	Activation_CreateObject(oid,flags,pOuter,iid,&pObj);
	return pObj;
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,Activation_NameToOid,1,((in),const Omega::string_t&,strObjectName));
inline Omega::guid_t Omega::Activation::NameToOid(const Omega::string_t& strObjectName)
{
	return Activation_NameToOid(strObjectName);
}

#endif // OOCORE_IFACES_H_INCLUDED_