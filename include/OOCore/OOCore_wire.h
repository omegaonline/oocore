#ifndef OOCORE_WIRE_H_INCLUDED_
#define OOCORE_WIRE_H_INCLUDED_

namespace Omega
{	
	namespace MetaInfo
	{
		template <class T>
		static void wire_read(Remoting::IProxyManager* pManager, Serialize::IFormattedStream*, T&);

		template <>
		inline static void wire_read(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, byte_t& val)
		{
			pStream->ReadByte(val);
		}

		template <>
		inline static void wire_read(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, uint16_t& val)
		{
			pStream->ReadUInt16(val);
		}

		template <>
		inline static void wire_read(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, uint32_t& val)
		{
			pStream->ReadUInt32(val);
		}

		template <>
		inline static void wire_read(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, uint64_t& val)
		{
			pStream->ReadUInt64(val);
		}

		template <>
		inline static void wire_read(Remoting::IProxyManager* pManager, Serialize::IFormattedStream* pStream, guid_t& val)
		{
			wire_read(pManager,pStream,val.Data1);
			wire_read(pManager,pStream,val.Data2);
			wire_read(pManager,pStream,val.Data3);
			pStream->ReadBytes(val.Data4,8);
		}

		template <class T>
		static void wire_write(Remoting::IProxyManager* pManager, Serialize::IFormattedStream*, const T&);
		
		template <>
		inline static void wire_write(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, const byte_t& val)
		{
			pStream->WriteByte(val);
		}

		template <>
		inline static void wire_write(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, const uint16_t& val)
		{
			pStream->WriteUInt16(val);
		}

		template <>
		inline static void wire_write(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, const uint32_t& val)
		{
			pStream->WriteUInt32(val);
		}

		template <>
		inline static void wire_write(Remoting::IProxyManager*, Serialize::IFormattedStream* pStream, const uint64_t& val)
		{
			pStream->WriteUInt64(val);
		}

		template <>
		inline static void wire_write(Remoting::IProxyManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& val)
		{
			wire_write(pManager,pStream,val.Data1);
			wire_write(pManager,pStream,val.Data2);
			wire_write(pManager,pStream,val.Data3);
			pStream->WriteBytes(val.Data4,8);
		}

	}
}

#endif // OOCORE_WIRE_H_INCLUDED_
