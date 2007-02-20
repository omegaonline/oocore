#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega {
namespace Remoting
{
	interface IMarshal : public IObject
	{
		enum Flags
		{
			apartment = 0,
			process = 1,
			machine = 2
		};
		typedef uint16_t Flags_t;

		virtual guid_t GetUnmarshalOID(const guid_t& iid, Flags_t flags) = 0;
        virtual void MarshalInterface(Serialize::IFormattedStream* pOutput, const guid_t& iid, Flags_t flags) = 0;
        virtual IObject* UnmarshalObject(Serialize::IFormattedStream* pInput, const guid_t& iid) = 0;
        virtual void ReleaseMarshalData(Serialize::IFormattedStream* pInput) = 0;
        virtual void DisconnectObject() = 0;
	};
	OMEGA_DECLARE_IID(IMarshal);

	interface ICallContext : public IObject
	{
	};

	interface IChannel : public IObject
	{
		virtual Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0) = 0;
		virtual Serialize::IFormattedStream* SendAndReceive(MethodAttributes_t attribs, Serialize::IFormattedStream* pStream) = 0;
	};
	OMEGA_DECLARE_IID(IChannel);

	interface IObjectManager : public IObject
	{
		virtual void Connect(IChannel* pChannel) = 0;
		virtual void Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		virtual void Disconnect() = 0;
		virtual void CreateUnboundProxy(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;
	};
	OMEGA_DECLARE_IID(IObjectManager);

	class IInterProcessService : public IObject
	{
	public:
		virtual Registry::IRegistryKey* GetRegistryKey() = 0;
		virtual Activation::IServiceTable* GetServiceTable() = 0;
	};
	OMEGA_DECLARE_IID(IInterProcessService);

	//void MarshalObject(Serialize::IFormattedStream* pOutput, IObject* pObject, const guid_t& iid, IMarshal::Flags_t flags);
	//IObject* UnmarshalObject(Serialize::IFormattedStream* pInput, const guid_t& iid);*/
}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IMarshal,
	0x2b0ccbdb, 0x4f77, 0x45fe, 0x8c, 0x9c, 0xf7, 0x81, 0xc5, 0x9a, 0x9f, 0x2b,

	OMEGA_METHOD(guid_t,GetUnmarshalOID,2,((in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
    OMEGA_METHOD_VOID(MarshalInterface,3,((in),Serialize::IFormattedStream*,pOutput,(in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
    OMEGA_METHOD(IObject*,UnmarshalObject,2,((in),Serialize::IFormattedStream*,pInput,(in),const guid_t&,iid))
    OMEGA_METHOD_VOID(ReleaseMarshalData,1,((in),Serialize::IFormattedStream*,pInput))
    OMEGA_METHOD_VOID(DisconnectObject,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IChannel,
	0x3aa1189, 0x6f97, 0x4352, 0xa4, 0x24, 0x6, 0x2b, 0x33, 0x7, 0x7b, 0x3b,

	OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,1,((in),IObject*,pOuter))
	OMEGA_METHOD(Serialize::IFormattedStream*,SendAndReceive,2,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pStream))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IObjectManager,
	0xd89cbe88, 0xf0ff, 0x498e, 0x84, 0x6a, 0x86, 0x88, 0x64, 0x63, 0xdd, 0x9b,

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD_VOID(Invoke,3,((in),Serialize::IFormattedStream*,pParamsIn,(in),Serialize::IFormattedStream*,pParamsOut,(in),uint32_t,timeout))
	OMEGA_METHOD_VOID(Disconnect,0,())
	OMEGA_METHOD_VOID(CreateUnboundProxy,3,((in),const guid_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IInterProcessService,
	0x355c529b, 0x579a, 0x411d, 0xb1, 0x6c, 0x31, 0x23, 0xbc, 0x85, 0x5f, 0xbf,

	OMEGA_METHOD(Registry::IRegistryKey*,GetRegistryKey,0,())
	OMEGA_METHOD(Activation::IServiceTable*,GetServiceTable,0,())
)

OMEGA_DEFINE_OID(Omega,OID_StdObjectManager, 0xa162a7a2, 0x6c69, 0x4ea8, 0xad, 0xc4, 0x81, 0x9e, 0xe, 0x2f, 0x3b, 0x3b);

namespace Omega
{
	OMEGA_DEFINE_OID(Remoting, OID_InterProcess, 0x525e8e60, 0xdb0f, 0x42d7, 0x93, 0x8b, 0x86, 0x7e, 0x0, 0x8b, 0xe3, 0x6);
}

#endif // OOCORE_REMOTING_H_INCLUDED_
