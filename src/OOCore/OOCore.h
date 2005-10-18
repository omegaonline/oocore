#pragma once

#include "./Object.h"

#include "./OOCore_export.h"

class OOCore_Object_Stub_Base;

extern "C"
{
	OOCore_Export void* OOCore_Alloc(size_t size);
	OOCore_Export void OOCore_Free(void* p);
	OOCore_Export int CreateStub(const OOObj::GUID& iid, OOObj::Object* obj, OOCore_Object_Stub_Base** stub);
	OOCore_Export int CreateProxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler, OOObj::Object** proxy);
}
