//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OBJECT_TYPES_H_INCLUDED_
#define OOCORE_OBJECT_TYPES_H_INCLUDED_

#include <ace/CDR_Stream.h>
#include <ace/OS.h>

#include "./OOCore_export.h"

namespace OOObject
{
	typedef ACE_CDR::Boolean			bool_t;
	typedef ACE_CDR::Char				char_t;
	typedef ACE_CDR::Octet				byte_t;
    typedef ACE_CDR::Short				int16_t;
	typedef ACE_CDR::UShort				uint16_t;
	typedef ACE_CDR::Long				int32_t;
	typedef ACE_CDR::ULong				uint32_t;
	typedef ACE_CDR::LongLong			int64_t;
	typedef ACE_CDR::ULongLong			uint64_t;
	typedef ACE_CDR::Float				real4_t;
	typedef ACE_CDR::Double				real8_t;
	
	struct OOCore_Export guid_t
	{
		guid_t() : Data1(0), Data2(0), Data3(0)
		{ 
			ACE_OS::memset(Data4,0,8);
		}

		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		bool operator==(const OOObject::guid_t& rhs) const;
		bool operator<(const OOObject::guid_t& rhs) const;

		static const guid_t NIL;
	};
};

#endif // OOCORE_OBJECT_TYPES_H_INCLUDED_
