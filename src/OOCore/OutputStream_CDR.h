//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_
#define OOCORE_OUTPUTSTREAM_CDR_H_INCLUDED_

#include "./InputStream_CDR.h"

namespace OOCore
{
namespace Impl
{

class InputStream_CDR;

class OutputStream_CDR :
	public OOUtil::Object_Root<OutputStream_CDR>,
	public OOObject::OutputStream,
	public ACE_OutputCDR
{
	friend class InputStream_CDR;
	
public:
	int copy_from(InputStream_CDR* in);

	HAS_IID;

BEGIN_INTERFACE_MAP(OutputStream_CDR)
	INTERFACE_ENTRY(OOObject::OutputStream)
	INTERFACE_ENTRY(OutputStream_CDR)
END_INTERFACE_MAP()

protected:
	OutputStream_CDR();

	virtual ~OutputStream_CDR() {}

private:
	template <class T>
	int WriteVar(const T& val)
	{
		return ((*this << val) ? 0 : -1);
	}

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
