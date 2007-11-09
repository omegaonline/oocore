#ifndef OOSERVER_CHANNEL_H_INCLUDED_
#define OOSERVER_CHANNEL_H_INCLUDED_

namespace User
{
	class Manager;

	interface IOutputCDR : public Omega::Serialize::IFormattedStream
	{
		virtual void* GetMessageBlock() = 0;
	};
}

OMEGA_DEFINE_INTERFACE_DERIVED
(
	User, IOutputCDR, Omega::Serialize, IFormattedStream, "{9C4FFF8C-93E4-49f9-A11C-99249C321206}",

	// Methods
	OMEGA_METHOD(void*,GetMessageBlock,0,())
)

namespace User
{
	ACE_CString string_t_to_utf8(const Omega::string_t& val);

	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public IOutputCDR
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
			INTERFACE_ENTRY(IOutputCDR)
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
			{ Omega::byte_t val; if (!get_input().read_octet(val)) OOSERVER_THROW_LASTERROR(); return val; }
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!get_input().read_octet_array(val,cbBytes)) OOSERVER_THROW_LASTERROR(); }
		void WriteByte(Omega::byte_t val)
			{ if (!write_octet(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteBytes(Omega::uint32_t cbBytes, const Omega::byte_t* val)
			{ if (!write_octet_array(val,cbBytes)) OOSERVER_THROW_LASTERROR(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!get_input().read_boolean(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!get_input().read_ushort(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!get_input().read_ulong(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!get_input().read_ulonglong(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!get_input().read_string(val)) OOSERVER_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		void WriteBoolean(Omega::bool_t val)
			{ if (!write_boolean(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteString(const Omega::string_t& val)
			{ if (!write_string(string_t_to_utf8(val))) OOSERVER_THROW_LASTERROR(); }
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
			{ Omega::byte_t val; if (!read_octet(val)) OOSERVER_THROW_LASTERROR(); return val; }
		void ReadBytes(Omega::uint32_t& cbBytes, Omega::byte_t* val)
			{ if (!read_octet_array(val,cbBytes)) OOSERVER_THROW_LASTERROR(); }
		void WriteByte(Omega::byte_t)
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteBytes(Omega::uint32_t, const Omega::byte_t*)
			{ OOSERVER_THROW_ERRNO(EACCES); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!read_boolean(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!read_ushort(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!read_ulong(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!read_ulonglong(val)) OOSERVER_THROW_LASTERROR(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!read_string(val)) OOSERVER_THROW_LASTERROR(); return Omega::string_t(val.c_str(),true); }
		void WriteBoolean(Omega::bool_t)
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteUInt16(Omega::uint16_t)
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteUInt32(Omega::uint32_t)
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteUInt64(const Omega::uint64_t&)
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteString(const Omega::string_t&)
			{ OOSERVER_THROW_ERRNO(EACCES); }
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
		ACE_CDR::UShort  m_channel_id;

		Channel(const Channel&) : OTL::ObjectBase(), Omega::Remoting::IChannel() {}
		Channel& operator = (const Channel&) { return *this; }

	// IChannel members
	public:
		Omega::Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter);
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pSend, Omega::Serialize::IFormattedStream*& pRecv, Omega::uint16_t timeout);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags);
	};

	// {1A7672C5-8478-4e5a-9D8B-D5D019E25D15}
	extern const Omega::guid_t OID_ChannelMarshalFactory;

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

#endif // OOSERVER_CHANNEL_H_INCLUDED_
