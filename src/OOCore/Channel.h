///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_CHANNEL_H_INCLUDED_
#define OOCORE_CHANNEL_H_INCLUDED_

namespace OOCore
{
	interface IOutputCDR : public Omega::Serialize::IFormattedStream
	{
		virtual void* GetMessageBlock() = 0;
	};
}

OMEGA_DEFINE_INTERFACE_DERIVED
(
	OOCore, IOutputCDR, Omega::Serialize, IFormattedStream, "{5251283B-95C8-4e5b-9136-5DDCBE636A4E}",

	// Methods
	OMEGA_METHOD(void*,GetMessageBlock,0,())
)

namespace OOCore
{
	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public OOCore::IOutputCDR
	{
	public:
		OutputCDR() : m_pInput(0)
		{ }

		virtual ~OutputCDR()
		{
			if (m_pInput)
				delete m_pInput;
		}

		void* GetMessageBlock()
		{
			return begin()->duplicate();
		}

		BEGIN_INTERFACE_MAP(OutputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
			INTERFACE_ENTRY(OOCore::IOutputCDR)
		END_INTERFACE_MAP()

	private:
		ACE_InputCDR* m_pInput;

		ACE_InputCDR& get_input()
		{
			if (!m_pInput)
				OMEGA_NEW(m_pInput,ACE_InputCDR(*this));

			return *m_pInput;
		}

	// IStream members
	public:
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!get_input().read_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }
		void WriteBytes(Omega::uint32_t cbBytes, const Omega::byte_t* val)
			{ if (!write_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!get_input().read_boolean(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::byte_t ReadByte()
			{ Omega::byte_t val; if (!get_input().read_octet(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int16_t ReadInt16()
			{ Omega::int16_t val; if (!get_input().read_short(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!get_input().read_ushort(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int32_t ReadInt32()
			{ Omega::int32_t val; if (!get_input().read_long(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!get_input().read_ulong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int64_t ReadInt64()
			{ Omega::int64_t val; if (!get_input().read_longlong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!get_input().read_ulonglong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!get_input().read_string(val)) OOCORE_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		Omega::guid_t ReadGuid()
		{
			Omega::guid_t g;
			g.Data1 = ReadUInt32();
			g.Data2 = ReadUInt16();
			g.Data3 = ReadUInt16();
			Omega::uint32_t bytes = 8;
			ReadBytes(bytes,g.Data4);
			if (bytes != 8)
				throw Omega::ISystemException::Create(EIO);
			return g;
		}

		void WriteBoolean(Omega::bool_t val)
			{ if (!write_boolean(val)) OOCORE_THROW_LASTERROR(); }
		void WriteByte(Omega::byte_t val)
			{ if (!write_octet(val)) OOCORE_THROW_LASTERROR(); }
		void WriteInt16(Omega::int16_t val)
			{ if (!write_short(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) OOCORE_THROW_LASTERROR(); }
		void WriteInt32(Omega::int32_t val)
			{ if (!write_long(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) OOCORE_THROW_LASTERROR(); }
		void WriteInt64(const Omega::int64_t& val)
			{ if (!write_longlong(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) OOCORE_THROW_LASTERROR(); }
		void WriteString(const Omega::string_t& val)
			{ if (!write_string(val.ToUTF8().c_str())) OOCORE_THROW_LASTERROR(); }
		void WriteGuid(const Omega::guid_t& val)
		{
			WriteUInt32(val.Data1);
			WriteUInt16(val.Data2);
			WriteUInt16(val.Data3);
			WriteBytes(8,val.Data4);
		}
	};

	class InputCDR :
		public OTL::ObjectBase,
		public ACE_InputCDR,
		public Omega::Serialize::IFormattedStream
	{
	public:
		InputCDR() : ACE_InputCDR(size_t(0))
		{}

		void init(const ACE_InputCDR& i)
		{
			*static_cast<ACE_InputCDR*>(this) = i;
		}

		BEGIN_INTERFACE_MAP(InputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
		END_INTERFACE_MAP()

	// IStream members
	public:
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!read_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }
		void WriteBytes(Omega::uint32_t, const Omega::byte_t*)
			{ OMEGA_THROW(EACCES); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!read_boolean(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::byte_t ReadByte()
			{ Omega::byte_t val; if (!read_octet(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int16_t ReadInt16()
			{ Omega::int16_t val; if (!read_short(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!read_ushort(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int32_t ReadInt32()
			{ Omega::int32_t val; if (!read_long(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!read_ulong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::int64_t ReadInt64()
			{ Omega::int64_t val; if (!read_longlong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!read_ulonglong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!read_string(val)) OOCORE_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		Omega::guid_t ReadGuid()
		{
			Omega::guid_t g;
			g.Data1 = ReadUInt32();
			g.Data2 = ReadUInt16();
			g.Data3 = ReadUInt16();
			Omega::uint32_t bytes = 8;
			ReadBytes(bytes,g.Data4);
			if (bytes != 8)
				throw Omega::ISystemException::Create(EIO);
			return g;
		}
		void WriteBoolean(Omega::bool_t)
			{ OMEGA_THROW(EACCES); }
		void WriteByte(Omega::byte_t)
			{ OMEGA_THROW(EACCES); }
		void WriteInt16(Omega::int16_t)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt16(Omega::uint16_t)
			{ OMEGA_THROW(EACCES); }
		void WriteInt32(Omega::int32_t)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt32(Omega::uint32_t)
			{ OMEGA_THROW(EACCES); }
		void WriteInt64(const Omega::int64_t&)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt64(const Omega::uint64_t&)
			{ OMEGA_THROW(EACCES); }
		void WriteString(const Omega::string_t&)
			{ OMEGA_THROW(EACCES); }
		void WriteGuid(const Omega::guid_t&)
			{ OMEGA_THROW(EACCES); }
	};

	class Channel :
		public OTL::ObjectBase,
		public Omega::Remoting::IChannel,
		public Omega::Remoting::IMarshal
	{
	public:
		Channel();

		void init(ACE_CDR::ULong channel_id);
		
		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	private:
		ACE_CDR::ULong	m_channel_id;

		Channel(const Channel&) : OTL::ObjectBase(), Omega::Remoting::IChannel() {}
		Channel& operator = (const Channel&) { return *this; }

	// IChannel members
	public:
		Omega::Serialize::IFormattedStream* CreateOutputStream();
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pSend, Omega::Serialize::IFormattedStream*& pRecv, Omega::uint16_t timeout);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};

	// {7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}
	OMEGA_DECLARE_OID(OID_ChannelMarshalFactory);

	class ChannelMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<ChannelMarshalFactory,&OID_ChannelMarshalFactory>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ChannelMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_CHANNEL_H_INCLUDED_
