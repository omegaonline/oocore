//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_
#define OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_

#include <ace/CDR_Stream.h>

#include "./OOCore_Util.h"

namespace Impl
{

class OutputStream_CDR :
	public OOCore::Object_Impl<OOCore::OutputStream>,
	public ACE_OutputCDR
{
public:
	OutputStream_CDR(unsigned long magic);

	unsigned long get_magic() const;

private:
	virtual ~OutputStream_CDR(void);

	template <class T>
	int WriteVar(const T& val)
	{
		return ((*this << val) ? 0 : -1);
	}

	const unsigned long m_magic;

public:
	int Append(OutputStream* add);
	int WriteBoolean(OOObj::bool_t val);
	int WriteChar(OOObj::char_t val);
	int WriteByte(OOObj::byte_t val);
	int WriteShort(OOObj::int16_t val);
	int WriteUShort(OOObj::uint16_t val);
	int WriteLong(OOObj::int32_t val);
	int WriteULong(OOObj::uint32_t val);
	int WriteLongLong(OOObj::int64_t val);
	int WriteULongLong(OOObj::uint64_t val);
	int WriteFloat(OOObj::real4_t val);
	int WriteDouble(OOObj::real8_t val);
	int WriteCookie(const OOObj::cookie_t& val);
	int WriteGuid(const OOObj::guid_t& val);
	int WriteBytes(const OOObj::byte_t* val, OOObj::uint32_t len);
};

};

#endif // OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_
