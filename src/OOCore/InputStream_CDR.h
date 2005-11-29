//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_INPUTSTREAM_CDR_H_INCLUDED_
#define OOCORE_INPUTSTREAM_CDR_H_INCLUDED_

#include <ace/CDR_Stream.h>

#include "./OOCore_Util.h"

namespace OOCore
{
namespace Impl
{

class InputStream_CDR : 
	public OOCore::Object_Impl<OOCore::InputStream>,
	public ACE_InputCDR
{
public:
	InputStream_CDR(ACE_InputCDR& in);
	
private:
	virtual ~InputStream_CDR() {};

	template <class T>
	int ReadVar(T& val)
	{
		return ((*this >> val) ? 0 : -1);
	}

// OOObject::InputStream members
public:
	int ReadBoolean(OOObject::bool_t& val);
	int ReadChar(OOObject::char_t& val);
	int ReadByte(OOObject::byte_t& val);
	int ReadShort(OOObject::int16_t& val);
	int ReadUShort(OOObject::uint16_t& val);
	int ReadLong(OOObject::int32_t& val);
	int ReadULong(OOObject::uint32_t& val);
	int ReadLongLong(OOObject::int64_t& val);
	int ReadULongLong(OOObject::uint64_t& val);
	int ReadFloat(OOObject::real4_t& val);
	int ReadDouble(OOObject::real8_t& val);
};

};
};

#endif // OOCORE_INPUTSTREAM_CDR_H_INCLUDED_
