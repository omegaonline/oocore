//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_OBJECT_TYPES_H_INCLUDED_
#define _OOCORE_OBJECT_TYPES_H_INCLUDED_

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
};

#endif // _OOCORE_OBJECT_TYPES_H_INCLUDED_
