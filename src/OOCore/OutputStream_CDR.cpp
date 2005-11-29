#include "./OutputStream_CDR.h"

OOCore::Impl::OutputStream_CDR::OutputStream_CDR(unsigned long magic) :
	m_magic(magic)
{
}

OOCore::Impl::OutputStream_CDR::~OutputStream_CDR(void)
{
}

unsigned long 
OOCore::Impl::OutputStream_CDR::get_magic() const
{
	return m_magic;
}

int 
OOCore::Impl::OutputStream_CDR::Append(OutputStream* add)
{
	OutputStream_CDR* append = reinterpret_cast<OutputStream_CDR*>(add);
	if (!append || append->get_magic()!=m_magic)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream passed in to Append\n")),-1);

	return (write_octet_array_mb(append->begin()) ? 0 : -1);
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
