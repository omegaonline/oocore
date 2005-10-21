#pragma once

#include <ace/Time_Value.h>

#include "./OOCore_export.h"

OOCore_Export void* OOCore_Alloc(size_t size);
OOCore_Export void OOCore_Free(void* p);
OOCore_Export int OOCore_RunReactor(ACE_Time_Value* timeout = 0);

