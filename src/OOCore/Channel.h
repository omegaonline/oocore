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
	ACE_CString string_t_to_utf8(const Omega::string_t& val);

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
		Omega::byte_t ReadByte()
			{ Omega::byte_t val; if (!get_input().read_octet(val)) OOCORE_THROW_LASTERROR(); return val; }
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!get_input().read_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }
		void WriteByte(Omega::byte_t val)
			{ if (!write_octet(val)) OOCORE_THROW_LASTERROR(); }
		void WriteBytes(Omega::uint32_t cbBytes, const Omega::byte_t* val)
			{ if (!write_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!get_input().read_boolean(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!get_input().read_ushort(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!get_input().read_ulong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!get_input().read_ulonglong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!get_input().read_string(val)) OOCORE_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		void WriteBoolean(Omega::bool_t val)
			{ if (!write_boolean(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) OOCORE_THROW_LASTERROR(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) OOCORE_THROW_LASTERROR(); }
		void WriteString(const Omega::string_t& val)
			{ if (!write_string(string_t_to_utf8(val))) OOCORE_THROW_LASTERROR(); }
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
		Omega::byte_t ReadByte()
			{ Omega::byte_t val; if (!read_octet(val)) OOCORE_THROW_LASTERROR(); return val; }
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!read_octet_array(val,cbBytes)) OOCORE_THROW_LASTERROR(); }
		void WriteByte(Omega::byte_t)
			{ OOCORE_THROW_ERRNO(EACCES); }
		void WriteBytes(Omega::uint32_t, const Omega::byte_t*)
			{ OOCORE_THROW_ERRNO(EACCES); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!read_boolean(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!read_ushort(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!read_ulong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!read_ulonglong(val)) OOCORE_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!read_string(val)) OOCORE_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		void WriteBoolean(Omega::bool_t)
			{ OOCORE_THROW_ERRNO(EACCES); }
		void WriteUInt16(Omega::uint16_t)
			{ OOCORE_THROW_ERRNO(EACCES); }
		void WriteUInt32(Omega::uint32_t)
			{ OOCORE_THROW_ERRNO(EACCES); }
		void WriteUInt64(const Omega::uint64_t&)
			{ OOCORE_THROW_ERRNO(EACCES); }
		void WriteString(const Omega::string_t&)
			{ OOCORE_THROW_ERRNO(EACCES); }
	};

	class Channel :
		public OTL::ObjectBase,
		public Omega::Remoting::IChannel,
		public Omega::Remoting::IMarshal
	{
	public:
		Channel();

		void init(ACE_CDR::UShort channel_id);
		
		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	private:
		ACE_CDR::UShort	m_channel_id;

		Channel(const Channel&) : OTL::ObjectBase(), Omega::Remoting::IChannel() {}
		Channel& operator = (const Channel&) { return *this; }

	// IChannel members
	public:
		Omega::Serialize::IFormattedStream* CreateOutputStream();
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pSend, Omega::Serialize::IFormattedStream*& pRecv, Omega::uint16_t timeout);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
	};

	// {7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}
	OOCORE_DECLARE_OID(OID_ChannelMarshalFactory);

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
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_CHANNEL_H_INCLUDED_
