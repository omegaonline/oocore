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

namespace OOCore
{
namespace Impl
{

class InputStream_CDR;

class OutputStream_CDR :
	public OOCore::Object_Impl<OOCore::OutputStream>,
	public ACE_OutputCDR
{
public:
	OutputStream_CDR(size_t magic);

	size_t get_magic() const;
	int copy_from(InputStream_CDR* in);

private:
	virtual ~OutputStream_CDR(void);

	template <class T>
	int WriteVar(const T& val)
	{
		return ((*this << val) ? 0 : -1);
	}

	const unsigned long m_magic;

public:
	int WriteBoolean(OOObject::bool_t val);
	int WriteChar(OOObject::char_t val);
	int WriteByte(OOObject::byte_t val);
	int WriteShort(OOObject::int16_t val);
	int WriteUShort(OOObject::uint16_t val);
	int WriteLong(OOObject::int32_t val);
	int WriteULong(OOObject::uint32_t val);
	int WriteLongLong(OOObject::int64_t val);
	int WriteULongLong(OOObject::uint64_t val);
	int WriteFloat(OOObject::real4_t val);
	int WriteDouble(OOObject::real8_t val);
};

};
};

#endif // OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_
