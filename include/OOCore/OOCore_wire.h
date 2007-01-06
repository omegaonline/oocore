#ifndef OOCORE_WIRE_H_INCLUDED_
#define OOCORE_WIRE_H_INCLUDED_

namespace Omega
{	
	namespace Serialize
	{
		interface IStream : public IObject
		{
			virtual byte_t ReadByte() = 0;
			virtual void ReadBytes(byte_t* val, uint32_t cbBytes) = 0;
			virtual void WriteByte(byte_t val) = 0;
			virtual void WriteBytes(const byte_t* val, uint32_t cbBytes) = 0;
		};
		OMEGA_DECLARE_IID(IStream);

		interface IFormattedStream : public IStream
		{
			virtual uint16_t ReadUInt16() = 0;
			virtual uint32_t ReadUInt32() = 0;
			virtual uint64_t ReadUInt64() = 0;
			virtual guid_t ReadGuid() = 0;
			
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
		};
		OMEGA_DECLARE_IID(IFormattedStream);
	}

	namespace MetaInfo
	{
		interface IWireManager : public IObject
		{
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, IObject* pObject, const guid_t& iid) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject** pObject) = 0;
		};

		interface IWireStub : public IObject
		{
			virtual void Call(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		};





		template <class T>
		static void wire_read(IWireManager* pManager, Serialize::IFormattedStream*, T&);

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, byte_t& val)
		{
			val = pStream->ReadByte();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint16_t& val)
		{
			val = pStream->ReadUInt16();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint32_t& val)
		{
			val = pStream->ReadUInt32();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint64_t& val)
		{
			val = pStream->ReadUInt64();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, guid_t& val)
		{
			val = pStream->ReadGuid();
		}

		template <class iface>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, iface** ppI, const guid_t& iid = iid_traits<iface>::GetIID())
		{
			pManager->UnmarshalInterface(pStream,iid,ppI);
		}

		template <class T>
		static void wire_write(IWireManager* pManager, Serialize::IFormattedStream*, const T&);

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const byte_t& val)
		{
			pStream->WriteByte(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const uint16_t& val)
		{
			pStream->WriteUInt16(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const uint32_t& val)
		{
			pStream->WriteUInt32(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const uint64_t& val)
		{
			pStream->WriteUInt64(val);
		}

		/*template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const guid_t& val)
		{
			pStream->WriteGuid(val);
		}*/

		template <class iface>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, iface* pI, const guid_t& iid = iid_traits<iface>::GetIID())
		{
			pManager->MarshalInterface(pStream,pI,iid);
		}

	}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Serialize, IStream,
	0x5344a2f5, 0x5d58, 0x46c4, 0xa0, 0x96, 0xe1, 0x71, 0x98, 0xa8, 0x8, 0xd2,

	// Methods
	OMEGA_METHOD(byte_t,ReadByte,0,())
	OMEGA_METHOD_VOID(ReadBytes,2,((out)(size_is(cbBytes)),byte_t*,val,(in),uint32_t,cbBytes))
	OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
	OMEGA_METHOD_VOID(WriteBytes,2,((in)(size_is(cbBytes)),const byte_t*,val,(in),uint32_t,cbBytes))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Serialize, IFormattedStream, Omega::Serialize, IStream,
	0xc3df15dc, 0x10c, 0x4fa0, 0x8d, 0xe6, 0x24, 0x3b, 0x13, 0xe0, 0x1c, 0xf7,
	
	// Methods
	OMEGA_METHOD(uint16_t,ReadUInt16,0,())
	OMEGA_METHOD(uint32_t,ReadUInt32,0,())
	OMEGA_METHOD(uint64_t,ReadUInt64,0,())
	OMEGA_METHOD(guid_t,ReadGuid,0,())
	
	OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
	OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
	OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
)

#endif // OOCORE_WIRE_H_INCLUDED_
