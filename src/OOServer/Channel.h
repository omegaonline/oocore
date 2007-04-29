#ifndef OOSERVER_CHANNEL_H_INCLUDED_
#define OOSERVER_CHANNEL_H_INCLUDED_

namespace User
{
	class Manager;

	interface IOutputCDR : public Omega::Serialize::IFormattedStream
	{
		virtual void* GetMessageBlock() = 0;
	};
	OMEGA_DECLARE_IID(IOutputCDR);
		
	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public IOutputCDR
	{
	public:
		OutputCDR()
		{ }

		void* GetMessageBlock()
		{
			return begin()->duplicate();
		}

		BEGIN_INTERFACE_MAP(OutputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
			INTERFACE_ENTRY_IID(IID_IOutputCDR,IOutputCDR)
		END_INTERFACE_MAP()

	// IStream members
	public:
		Omega::byte_t ReadByte() 
			{ OOSERVER_THROW_ERRNO(EACCES); return 0; }
		void ReadBytes(Omega::uint32_t&, Omega::byte_t*) 
			{ OOSERVER_THROW_ERRNO(EACCES); }
		void WriteByte(Omega::byte_t val) 
			{ if (!write_octet(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteBytes(Omega::uint32_t cbBytes, const Omega::byte_t* val) 
			{ if (!write_octet_array(val,cbBytes)) OOSERVER_THROW_LASTERROR(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean() 
			{ OOSERVER_THROW_ERRNO(EACCES); return 0; }
		Omega::uint16_t ReadUInt16() 
			{ OOSERVER_THROW_ERRNO(EACCES); return 0; }
		Omega::uint32_t ReadUInt32() 
			{ OOSERVER_THROW_ERRNO(EACCES); return 0; }
		Omega::uint64_t ReadUInt64() 
			{ OOSERVER_THROW_ERRNO(EACCES); return 0; }
		Omega::string_t ReadString()
			{ OOSERVER_THROW_ERRNO(EACCES); return Omega::string_t(); }
		void WriteBoolean(Omega::bool_t val)
			{ if (!write_boolean(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) OOSERVER_THROW_LASTERROR(); }
		void WriteString(const Omega::string_t& val)
			{ if (!write_string(static_cast<ACE_CDR::ULong>(val.Length()),val)) OOSERVER_THROW_LASTERROR(); }
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
			{ ACE_CString val; if (!read_string(val)) OOSERVER_THROW_LASTERROR(); return Omega::string_t(val.c_str()); }
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
		public Omega::Remoting::IChannel
	{
	public:
		Channel();

		void init(Manager* pManager, ACE_HANDLE handle, ACE_CDR::UShort channel_id);
		
		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
		END_INTERFACE_MAP()

	private:
		Manager*         m_pManager;
		ACE_HANDLE       m_handle;
		ACE_CDR::UShort  m_channel_id;

	// IChannel members
	public: 
		Omega::Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0);
		Omega::Serialize::IFormattedStream* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pStream);
	};
}

#endif // OOSERVER_CHANNEL_H_INCLUDED_