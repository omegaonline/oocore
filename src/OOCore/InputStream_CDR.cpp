#include "./InputStream_CDR.h"

// Intentionally the same as OutputStream_CDR
DEFINE_IID(OOCore::Impl::InputStream_CDR,EF7C52B5-5631-4bdf-A213-7CFA7CF1F8DE);

OOCore::Impl::InputStream_CDR::InputStream_CDR(const ACE_InputCDR& in) :
	ACE_InputCDR(in)
{
}

int
OOCore::Impl::InputStream_CDR::ReadBoolean(OOObject::bool_t& val)
{
	return (read_boolean(val) ? 0 : -1);
}

int
OOCore::Impl::InputStream_CDR::ReadChar(OOObject::char_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadByte(OOObject::byte_t& val)
{
	return (read_octet(val) ? 0 : -1);
}

int
OOCore::Impl::InputStream_CDR::ReadShort(OOObject::int16_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadUShort(OOObject::uint16_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadLong(OOObject::int32_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadULong(OOObject::uint32_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadLongLong(OOObject::int64_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadULongLong(OOObject::uint64_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadFloat(OOObject::real4_t& val)
{
	return ReadVar(val);
}

int
OOCore::Impl::InputStream_CDR::ReadDouble(OOObject::real8_t& val)
{
	return ReadVar(val);
}
