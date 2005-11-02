#ifndef _OOCORE_OOCORE_H_INCLUDED_
#define _OOCORE_OOCORE_H_INCLUDED_

#include <ace/Time_Value.h>
#include <ace/Method_Request.h>

#include "./OOCore_export.h"

OOCore_Export void* OOCore_Alloc(size_t size);
OOCore_Export void OOCore_Free(void* p);

typedef bool (*CONDITION_FN)(void*);
OOCore_Export int OOCore_RunReactorEx(ACE_Time_Value* timeout = 0, CONDITION_FN cond_fn = 0, void* p = 0);

OOCore_Export int OOCore_RunReactor(ACE_Time_Value* timeout = 0);
OOCore_Export int OOCore_PostRequest(ACE_Method_Request* req, ACE_Time_Value* wait = 0);
OOCore_Export int OOCore_PostCloseRequest(ACE_Method_Request* req, ACE_Time_Value* wait = 0);

#endif // _OOCORE_OOCORE_H_INCLUDED_
