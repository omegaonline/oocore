#ifndef OOCORE_CHANNEL_H_INCLUDED_
#define OOCORE_CHANNEL_H_INCLUDED_

namespace OOCore
{
	OMEGA_DECLARE_IID(OutputCDR);

	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public Omega::Serialize::IFormattedStream
	{
	public:
		ACE_Message_Block* GetMessageBlock()
		{
			return begin()->duplicate();
		}

		BEGIN_INTERFACE_MAP(OutputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
			INTERFACE_ENTRY_IID(IID_OutputCDR,OutputCDR)
		END_INTERFACE_MAP()

	private:
		void no_access()
		{
			OMEGA_THROW(ACE_OS::strerror(EACCES));
		}

		void throw_errno()
		{
			OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
		}

	// IStream members
	public:
		Omega::byte_t ReadByte() 
			{ no_access(); return 0; }
		Omega::uint32_t ReadBytes(Omega::uint32_t, Omega::byte_t*) 
			{ no_access(); return 0; }
		void WriteByte(Omega::byte_t val) 
			{ if (!write_octet(val)) throw_errno(); }
		void WriteBytes(Omega::uint32_t cbBytes, const Omega::byte_t* val) 
			{ if (!write_octet_array(val,cbBytes)) throw_errno(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean() 
			{ no_access(); return 0; }
		Omega::uint16_t ReadUInt16() 
			{ no_access(); return 0; }
		Omega::uint32_t ReadUInt32() 
			{ no_access(); return 0; }
		Omega::uint64_t ReadUInt64() 
			{ no_access(); return 0; }
		Omega::string_t ReadString()
		{ no_access(); return Omega::string_t(); }
		void WriteBoolean(Omega::bool_t val)
			{ if (!write_boolean(val)) throw_errno(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) throw_errno(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) throw_errno(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) throw_errno(); }
		void WriteString(const Omega::string_t& val)
		{ if (!write_string(static_cast<ACE_CDR::ULong>(val.Length()),val)) throw_errno(); }
	};

	OMEGA_DECLARE_IID(InputCDR);

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
			INTERFACE_ENTRY_IID(IID_InputCDR,InputCDR)
		END_INTERFACE_MAP()

	private:
		void no_access()
		{
			OMEGA_THROW(ACE_OS::strerror(EACCES));
		}

		void throw_errno()
		{
			OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
		}

	// IStream members
	public:
		Omega::byte_t ReadByte()
			{ Omega::byte_t val; if (!read_octet(val)) throw_errno(); return val; }
		Omega::uint32_t ReadBytes(Omega::uint32_t cbBytes, Omega::byte_t* val)
			{ if (!read_octet_array(val,cbBytes)) throw_errno(); return cbBytes; }
		void WriteByte(Omega::byte_t) 
			{ no_access(); }
		void WriteBytes(Omega::uint32_t, const Omega::byte_t*) 
			{ no_access(); }

	// IFormattedStream members
	public:
		Omega::bool_t ReadBoolean()
			{ Omega::bool_t val; if (!read_boolean(val)) throw_errno(); return val; }
		Omega::uint16_t ReadUInt16()
			{ Omega::uint16_t val; if (!read_ushort(val)) throw_errno(); return val; }
		Omega::uint32_t ReadUInt32()
			{ Omega::uint32_t val; if (!read_ulong(val)) throw_errno(); return val; }
		Omega::uint64_t ReadUInt64()
			{ Omega::uint64_t val; if (!read_ulonglong(val)) throw_errno(); return val; }
		Omega::string_t ReadString()
			{ ACE_CString val; if (!read_string(val)) throw_errno(); return Omega::string_t(val.c_str()); }
		void WriteBoolean(Omega::bool_t) 
			{ no_access(); }
		void WriteUInt16(Omega::uint16_t) 
			{ no_access(); }
		void WriteUInt32(Omega::uint32_t) 
			{ no_access(); }
		void WriteUInt64(const Omega::uint64_t&) 
			{ no_access(); }
		void WriteString(const Omega::string_t&) 
			{ no_access(); }
	};
}

class UserSession;

class Channel :
	public OTL::ObjectBase,
	public Omega::Remoting::IChannel
{
public:
	Channel();

	void init(UserSession* pSession, ACE_CDR::UShort dest_channel_id);
	
	BEGIN_INTERFACE_MAP(Channel)
		INTERFACE_ENTRY(Omega::Remoting::IChannel)
	END_INTERFACE_MAP()

private:
	UserSession*	m_pSession;
	ACE_CDR::UShort	m_id;

// IChannel members
public: 
	Omega::Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0);
	Omega::Serialize::IFormattedStream* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pStream);
};

#endif // OOCORE_CHANNEL_H_INCLUDED_
