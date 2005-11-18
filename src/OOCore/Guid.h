//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_GUID_H_INCLUDED_
#define OOCORE_GUID_H_INCLUDED_

#include <ace/UUID.h>
#include <ace/CDR_Stream.h>

#include "./Object_Types.h"

namespace Impl
{
	OOObj::guid_t create_guid(const ACE_CString& uuidString);
	OOObj::guid_t create_guid(const ACE_Utils::UUID& uuid);

	ACE_TString guid_to_string(const OOObj::guid_t& guid);
};

#endif // OOCORE_GUID_H_INCLUDED_
