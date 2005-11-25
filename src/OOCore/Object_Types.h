//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OBJECT_TYPES_H_INCLUDED_
#define OOCORE_OBJECT_TYPES_H_INCLUDED_

#include <ace/Active_Map_Manager.h>
#include <ace/CDR_Stream.h>

#include "./OOCore_export.h"

namespace OOObj
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
	typedef ACE_Active_Map_Manager_Key	cookie_t;
	
	struct OOCore_Export guid_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		bool operator==(const OOObj::guid_t& rhs) const;
		bool operator<(const OOObj::guid_t& rhs) const;

		static const guid_t NIL;
	};
};

#endif // OOCORE_OBJECT_TYPES_H_INCLUDED_
