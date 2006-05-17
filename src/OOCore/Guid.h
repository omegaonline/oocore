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

namespace OOCore
{
namespace Impl
{
	OOObject::guid_t OOCore_Export create_guid(const ACE_CString& uuidString);
	OOObject::guid_t create_guid(const ACE_Utils::UUID& uuid);

	ACE_TString guid_to_string(const OOObject::guid_t& guid);
};
};

#endif // OOCORE_GUID_H_INCLUDED_
