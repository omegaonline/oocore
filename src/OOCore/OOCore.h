#pragma once

#include <ace/Time_Value.h>
#include <ace/Method_Request.h>

#include "./OOCore_export.h"

OOCore_Export void* OOCore_Alloc(size_t size);
OOCore_Export void OOCore_Free(void* p);
OOCore_Export int OOCore_RunReactor(ACE_Time_Value* timeout = 0);
OOCore_Export int OOCore_PostRequest(ACE_Method_Request* req, ACE_Time_Value* wait = 0);

