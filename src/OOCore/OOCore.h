#ifndef _OOCORE_OOCORE_H_INCLUDED_
#define _OOCORE_OOCORE_H_INCLUDED_

#include <ace/Time_Value.h>
#include <ace/Method_Request.h>

#include "./OOCore_export.h"

OOCore_Export void* OOCore_Alloc(size_t size);
OOCore_Export void OOCore_Free(void* p);

#endif // _OOCORE_OOCORE_H_INCLUDED_
