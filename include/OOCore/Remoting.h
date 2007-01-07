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

	enum MethodAttributes
	{
		synchronous = 1,
		encrypted = 2
	};
	typedef uint16_t MethodAttributes_t;

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
	};
	OMEGA_DECLARE_IID(IObjectManager);

	

	//void MarshalObject(Serialize::IFormattedStream* pOutput, IObject* pObject, const guid_t& iid, IMarshal::Flags_t flags);
	//IObject* UnmarshalObject(Serialize::IFormattedStream* pInput, const guid_t& iid);*/
}
}

OMEGA_DEFINE_OID(Omega,OID_StdObjectManager, 0xa162a7a2, 0x6c69, 0x4ea8, 0xad, 0xc4, 0x81, 0x9e, 0xe, 0x2f, 0x3b, 0x3b);

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
)

#endif // OOCORE_REMOTING_H_INCLUDED_
