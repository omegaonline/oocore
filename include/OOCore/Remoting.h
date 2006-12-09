#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include <OOCore/OOCore.h>

// Remove annoying windows.h defines
#if defined(OMEGA_WIN32) && defined(SendMessage)
#undef SendMessage
#endif

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

		virtual guid_t GetUnmarshalOID(const guid_t& iid, IObject* pObject, Flags_t flags) = 0;
        virtual void MarshalObject(Serialize::IFormattedStream* pOutput, IObject* pObject, const guid_t& iid, Flags_t flags) = 0;
        virtual IObject* UnmarshalObject(Serialize::IFormattedStream* pInput, const guid_t& iid) = 0;
        virtual void ReleaseMarshalData(Serialize::IFormattedStream* pInput) = 0;
        virtual void DisconnectObject() = 0;
	};
	OMEGA_DECLARE_IID(IMarshal);

	interface IChannelSink : public IObject
	{
		virtual void OnReceiveMessage(Serialize::IFormattedStream* pStream, uint32_t cookie) = 0;
		virtual void OnDisconnect(uint32_t cookie) = 0;
	};
	OMEGA_DECLARE_IID(IChannelSink);

	interface IChannel : public IObject
	{
		virtual void Attach(IChannel* pOverlay, IChannelSink* pSink, uint32_t cookie) = 0;
		virtual void Detach() = 0;
		virtual Serialize::IFormattedStream* CreateStream(IObject* pOuter) = 0;
		virtual void SendMessage(Serialize::IFormattedStream* pStream) = 0;
	};
	OMEGA_DECLARE_IID(IChannel);

	interface IObjectManager : public IObject
	{
		virtual void Attach(IChannel* pChannel) = 0;
		virtual IObject* PrepareStaticInterface(const guid_t& oid, const guid_t& iid) = 0;
	};
	OMEGA_DECLARE_IID(IObjectManager);

	//void MarshalObject(Serialize::IFormattedStream* pOutput, IObject* pObject, const guid_t& iid, IMarshal::Flags_t flags);
	//IObject* UnmarshalObject(Serialize::IFormattedStream* pInput, const guid_t& iid);*/
}
}

OMEGA_DEFINE_OID(Omega,OID_StdObjectManager, 0xa162a7a2, 0x6c69, 0x4ea8, 0xad, 0xc4, 0x81, 0x9e, 0xe, 0x2f, 0x3b, 0x3b);

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IChannelSink,
	0xc9f3a9d0, 0x976e, 0x4760, 0xa7, 0xf, 0x89, 0x1e, 0x7b, 0xf6, 0x18, 0x70,

	OMEGA_METHOD_VOID(OnReceiveMessage,2,((in),Serialize::IFormattedStream*,pStream,(in),uint32_t,cookie))
	OMEGA_METHOD_VOID(OnDisconnect,1,((in),uint32_t,cookie))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IChannel,
	0x3aa1189, 0x6f97, 0x4352, 0xa4, 0x24, 0x6, 0x2b, 0x33, 0x7, 0x7b, 0x3b,

	OMEGA_METHOD_VOID(Attach,3,((in),Remoting::IChannel*,pOverlay,(in),Remoting::IChannelSink*,pSink,(in),uint32_t,cookie))
	OMEGA_METHOD_VOID(Detach,0,())
	OMEGA_METHOD(Serialize::IFormattedStream*,CreateStream,1,((in),IObject*,pOuter))
	OMEGA_METHOD_VOID(SendMessage,1,((in),Serialize::IFormattedStream*,pStream))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::Remoting, IObjectManager,
	0xd89cbe88, 0xf0ff, 0x498e, 0x84, 0x6a, 0x86, 0x88, 0x64, 0x63, 0xdd, 0x9b,

	OMEGA_METHOD_VOID(Attach,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD(Omega::IObject*,PrepareStaticInterface,2,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid))
)

#endif // OOCORE_REMOTING_H_INCLUDED_
