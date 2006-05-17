#include "./OutputStream_CDR.h"
#include "./guid.h"

using namespace OOCore::Impl;

// Intentionally the same as InputStream_CDR
DEFINE_IID(OutputStream_CDR,EF7C52B5-5631-4bdf-A213-7CFA7CF1F8DE);

OutputStream_CDR::OutputStream_CDR()
{
}

int 
OutputStream_CDR::copy_from(InputStream_CDR* in)
{
	return (write_octet_array_mb(in->start()) ? 0 : -1);
}

int 
OutputStream_CDR::WriteBoolean(OOObject::bool_t val)
{
	return (write_boolean(val) ? 0 : -1);
}

int 
OutputStream_CDR::WriteChar(OOObject::char_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteByte(OOObject::byte_t val)
{
	return (write_octet(val) ? 0 : -1);
}

int 
OutputStream_CDR::WriteShort(OOObject::int16_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteUShort(OOObject::uint16_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteLong(OOObject::int32_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteULong(OOObject::uint32_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteLongLong(OOObject::int64_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteULongLong(OOObject::uint64_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteFloat(OOObject::real4_t val)
{
	return WriteVar(val);
}

int 
OutputStream_CDR::WriteDouble(OOObject::real8_t val)
{
	return WriteVar(val);
}
