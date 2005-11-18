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

// OOObj::InputStream members
public:
	int ReadBoolean(OOObj::bool_t& val);
	int ReadChar(OOObj::char_t& val);
	int ReadByte(OOObj::byte_t& val);
	int ReadShort(OOObj::int16_t& val);
	int ReadUShort(OOObj::uint16_t& val);
	int ReadLong(OOObj::int32_t& val);
	int ReadULong(OOObj::uint32_t& val);
	int ReadLongLong(OOObj::int64_t& val);
	int ReadULongLong(OOObj::uint64_t& val);
	int ReadFloat(OOObj::real4_t& val);
	int ReadDouble(OOObj::real8_t& val);
	int ReadCookie(OOObj::cookie_t& val);
	int ReadGuid(OOObj::guid_t& val);
	int ReadBytes(OOObj::byte_t* val, OOObj::uint32_t len);
};

};

#endif // OOCORE_INPUTSTREAM_CDR_H_INCLUDED_
