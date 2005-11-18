#include "./OutputStream_CDR.h"

Impl::OutputStream_CDR::OutputStream_CDR(unsigned long magic) :
	m_magic(magic)
{
}

Impl::OutputStream_CDR::~OutputStream_CDR(void)
{
}

unsigned long 
Impl::OutputStream_CDR::get_magic() const
{
	return m_magic;
}

int 
Impl::OutputStream_CDR::Append(OutputStream* add)
{
	Impl::OutputStream_CDR* append = reinterpret_cast<Impl::OutputStream_CDR*>(add);
	if (!append || append->get_magic()!=m_magic)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream passed in to Append\n")),-1);

	return (write_octet_array_mb(append->begin()) ? 0 : -1);
}

int 
Impl::OutputStream_CDR::WriteBoolean(OOObj::bool_t val)
{
	return (write_boolean(val) ? 0 : -1);
}

int 
Impl::OutputStream_CDR::WriteChar(OOObj::char_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteByte(OOObj::byte_t val)
{
	return (write_octet(val) ? 0 : -1);
}

int 
Impl::OutputStream_CDR::WriteShort(OOObj::int16_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteUShort(OOObj::uint16_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteLong(OOObj::int32_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteULong(OOObj::uint32_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteLongLong(OOObj::int64_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteULongLong(OOObj::uint64_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteFloat(OOObj::real4_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteDouble(OOObj::real8_t val)
{
	return WriteVar(val);
}

int 
Impl::OutputStream_CDR::WriteCookie(const OOObj::cookie_t& val)
{
	ACE_CDR::Octet* buf;
	ACE_NEW_RETURN(buf,ACE_CDR::Octet[ACE_Active_Map_Manager_Key::size()],-1);
	
	val.encode(buf);

	bool ret = write_octet_array(buf,ACE_Active_Map_Manager_Key::size());
	delete [] buf;
	if (!ret)
		return -1;
		
	return 0;	
}

int 
Impl::OutputStream_CDR::WriteGuid(const OOObj::guid_t& val)
{
	if (WriteULong(val.Data1) != 0)
		return -1;

	if (WriteUShort(val.Data2) != 0)
		return -1;

	if (WriteUShort(val.Data3) != 0)
		return -1;

	return WriteBytes(val.Data4,8);
}

int 
Impl::OutputStream_CDR::WriteBytes(const OOObj::byte_t* val, OOObj::uint32_t len)
{
	return write_octet_array(val,len);
}
