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

		interface IApartment : public IObject
		{
			typedef bool_t (OMEGA_CALL *PUMP_CONDITION_FN)(void*);
			typedef void (OMEGA_CALL *REQUEST_FN)(void*);

			virtual bool_t PumpRequests(uint32_t* timeout = 0, PUMP_CONDITION_FN cond_fn = 0, void* cond_fn_args = 0) = 0;
			virtual bool_t PostRequest(REQUEST_FN request_fn, void* request_fn_args, uint32_t wait = 0) = 0;

			static Activation::IApartment* GetCurrentApartment();
		};
		OMEGA_DECLARE_IID(IApartment);

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

	namespace Serialize
	{
		interface IStream : public IObject
		{
			virtual uint64_t Size() = 0;
			virtual void ReadByte(byte_t& val) = 0;
			virtual void ReadBytes(byte_t* val, uint32_t cbBytes) = 0;
			virtual void WriteByte(byte_t val) = 0;
			virtual void WriteBytes(const byte_t* val, uint32_t cbBytes) = 0;
		};
		OMEGA_DECLARE_IID(IStream);

		interface IFormattedStream : public IStream
		{
			virtual void ReadUInt16(uint16_t& val) = 0;
			virtual void ReadUInt32(uint32_t& val) = 0;
			virtual void ReadUInt64(uint64_t& val) = 0;
			
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
		};
		OMEGA_DECLARE_IID(IFormattedStream);
	}

	namespace Remoting
	{
		enum MethodAttributes
		{
			synchronous = 1,
			has_timeout = 2,
			secure = 8
		};
		typedef uint16_t MethodAttributes_t;

		interface IStub : public IObject
		{
			virtual void Invoke(uint32_t method, Serialize::IFormattedStream* pInput, Serialize::IFormattedStream* pOutput);
		};
		OMEGA_DECLARE_IID(IStub);

		interface IProxyManager : public IObject
		{
			virtual Serialize::IFormattedStream* PrepareRequest(MethodAttributes_t flags) = 0;
			virtual Serialize::IFormattedStream* SendAndReceive(uint32_t* timeout, Serialize::IFormattedStream* pToSend) = 0;
		};
		OMEGA_DECLARE_IID(IProxyManager);
	}
}

OMEGA_DEFINE_IID(Omega::Activation, IApartment, 0xb1687b7d, 0xbd0, 0x4b41, 0xa1, 0xd1, 0x5, 0xd1, 0xd4, 0x65, 0x11, 0x56)

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

OMEGA_EXPORT_INTERFACE
(
	Omega::Serialize, IStream,
	0x5344a2f5, 0x5d58, 0x46c4, 0xa0, 0x96, 0xe1, 0x71, 0x98, 0xa8, 0x8, 0xd2,

	// Methods
	OMEGA_METHOD(uint64_t,Size,0,())
	OMEGA_METHOD_VOID(ReadByte,1,((out),byte_t&,val))
	OMEGA_METHOD_VOID(ReadBytes,2,((out)(size_is(cbBytes)),byte_t*,val,(in),uint32_t,cbBytes))
	OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
	OMEGA_METHOD_VOID(WriteBytes,2,((in)(size_is(cbBytes)),const byte_t*,val,(in),uint32_t,cbBytes))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Serialize, IFormattedStream, Omega::Serialize, IStream,
	0xc3df15dc, 0x10c, 0x4fa0, 0x8d, 0xe6, 0x24, 0x3b, 0x13, 0xe0, 0x1c, 0xf7,
	
	// Methods
	OMEGA_METHOD_VOID(ReadUInt16,1,((out),uint16_t&,val))
	OMEGA_METHOD_VOID(ReadUInt32,1,((out),uint32_t&,val))
	OMEGA_METHOD_VOID(ReadUInt64,1,((out),uint64_t&,val))
	
	OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
	OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
	OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IProxyManager,
	0xd17b8a2d, 0x64ac, 0x4cc3, 0xb6, 0x2b, 0xc, 0x92, 0xea, 0xa3, 0xd0, 0x98,

	OMEGA_METHOD(Omega::Serialize::IFormattedStream*,PrepareRequest,1,((in),Omega::Remoting::MethodAttributes_t,flags))
	OMEGA_METHOD(Omega::Serialize::IFormattedStream*,SendAndReceive,2,((in_out),uint32_t*,timeout,(in),Omega::Serialize::IFormattedStream*,pToSend))
)

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IApartment*,IApartment_GetCurrentApartment,0,())
inline Omega::Activation::IApartment* Omega::Activation::IApartment::GetCurrentApartment()
{
	return IApartment_GetCurrentApartment();
}

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