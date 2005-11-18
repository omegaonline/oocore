#include "./InputStream_CDR.h"

Impl::InputStream_CDR::InputStream_CDR(ACE_InputCDR& in) :
	ACE_InputCDR(in)
{
}

int
Impl::InputStream_CDR::ReadBoolean(OOObj::bool_t& val)
{
	return (read_boolean(val) ? 0 : -1);
}

int
Impl::InputStream_CDR::ReadChar(OOObj::char_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadByte(OOObj::byte_t& val)
{
	return (read_octet(val) ? 0 : -1);
}

int
Impl::InputStream_CDR::ReadShort(OOObj::int16_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadUShort(OOObj::uint16_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadLong(OOObj::int32_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadULong(OOObj::uint32_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadLongLong(OOObj::int64_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadULongLong(OOObj::uint64_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadFloat(OOObj::real4_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadDouble(OOObj::real8_t& val)
{
	return ReadVar(val);
}

int
Impl::InputStream_CDR::ReadCookie(OOObj::cookie_t& val)
{
	// Read the channel key
	val.decode(rd_ptr());
	if (!skip_bytes(ACE_Active_Map_Manager_Key::size()))
		return -1;

	return 0;
}

int
Impl::InputStream_CDR::ReadGuid(OOObj::guid_t& val)
{
	if (ReadULong(val.Data1) != 0)
		return -1;

	if (ReadUShort(val.Data2) != 0)
		return -1;

	if (ReadUShort(val.Data3) != 0)
		return -1;

	return ReadBytes(val.Data4,8);
}

int 
Impl::InputStream_CDR::ReadBytes(OOObj::byte_t* val, OOObj::uint32_t len)
{
	return (read_octet_array(val,len) ? 0 : -1);
}
