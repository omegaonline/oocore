#include "./OutputStream_CDR.h"
#include "./InputStream_CDR.h"

OOCore::Impl::OutputStream_CDR::OutputStream_CDR(size_t magic) :
	m_magic(magic)
{
}

OOCore::Impl::OutputStream_CDR::~OutputStream_CDR(void)
{
}

size_t 
OOCore::Impl::OutputStream_CDR::get_magic() const
{
	return m_magic;
}

int 
OOCore::Impl::OutputStream_CDR::copy_from(InputStream_CDR* in)
{
	return (write_octet_array_mb(in->start()) ? 0 : -1);
}

int 
OOCore::Impl::OutputStream_CDR::WriteBoolean(OOObject::bool_t val)
{
	return (write_boolean(val) ? 0 : -1);
}

int 
OOCore::Impl::OutputStream_CDR::WriteChar(OOObject::char_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteByte(OOObject::byte_t val)
{
	return (write_octet(val) ? 0 : -1);
}

int 
OOCore::Impl::OutputStream_CDR::WriteShort(OOObject::int16_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteUShort(OOObject::uint16_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteLong(OOObject::int32_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteULong(OOObject::uint32_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteLongLong(OOObject::int64_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteULongLong(OOObject::uint64_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteFloat(OOObject::real4_t val)
{
	return WriteVar(val);
}

int 
OOCore::Impl::OutputStream_CDR::WriteDouble(OOObject::real8_t val)
{
	return WriteVar(val);
}
