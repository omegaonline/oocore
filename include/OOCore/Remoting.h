#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega
{
	namespace Remoting
	{
		// Beware... this interface may well change in the future...
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
			virtual void Marshal(Serialize::IFormattedStream* pOutput, const guid_t& iid, Flags_t flags) = 0;
			virtual IObject* Unmarshal(Serialize::IFormattedStream* pInput, const guid_t& iid) = 0;
			//virtual void ReleaseMarshalData(Serialize::IFormattedStream* pInput) = 0;
			//virtual void DisconnectObject() = 0;
		};

		interface ICallContext : public IObject
		{
		};

		interface IChannel : public IObject
		{
			virtual Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0) = 0;
			virtual Serialize::IFormattedStream* SendAndReceive(MethodAttributes_t attribs, Serialize::IFormattedStream* pStream, uint16_t timeout) = 0;
		};

		interface IObjectManager : public System::MetaInfo::IWireManager
		{
			virtual void Connect(IChannel* pChannel) = 0;
			virtual void Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
			virtual void Disconnect() = 0;
			virtual void CreateUnboundProxy(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;
		};

		interface IInterProcessService : public IObject
		{
			virtual Registry::IRegistryKey* GetRegistry() = 0;
			virtual Activation::IServiceTable* GetServiceTable() = 0;
		};

		// {63EB243E-6AE3-43bd-B073-764E096775F8}
		OOCORE_DECLARE_OID(OID_StdObjectManager);

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		OOCORE_DECLARE_OID(OID_InterProcess);
	}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",

	OMEGA_METHOD(guid_t,GetUnmarshalOID,2,((in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
    OMEGA_METHOD_VOID(Marshal,3,((in),Serialize::IFormattedStream*,pOutput,(in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
    OMEGA_METHOD(IObject*,Unmarshal,2,((in),Serialize::IFormattedStream*,pInput,(in),const guid_t&,iid))
    //OMEGA_METHOD_VOID(ReleaseMarshalData,1,((in),Serialize::IFormattedStream*,pInput))
    //OMEGA_METHOD_VOID(DisconnectObject,0,())
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IChannel, "{F18430B0-8AC5-4b57-9B66-56B3BE867C24}",

	OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,1,((in),IObject*,pOuter))
	OMEGA_METHOD(Serialize::IFormattedStream*,SendAndReceive,3,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pStream,(in),uint16_t,timeout))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Remoting, IObjectManager, Omega::System::MetaInfo, IWireManager, "{0A6F7B1B-26A0-403c-AC80-ADFADA83615D}",

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD_VOID(Invoke,3,((in),Serialize::IFormattedStream*,pParamsIn,(in),Serialize::IFormattedStream*,pParamsOut,(in),uint32_t,timeout))
	OMEGA_METHOD_VOID(Disconnect,0,())
	OMEGA_METHOD_VOID(CreateUnboundProxy,3,((in),const guid_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IRegistryKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IServiceTable*,GetServiceTable,0,())
)

#endif // OOCORE_REMOTING_H_INCLUDED_
