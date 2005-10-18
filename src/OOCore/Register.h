#pragma once

#include "./Object.h"
#include "./Binding.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_Register
{
public:
	OOCore_Register(const OOObj::GUID& iid, const OOObj::char_t* dll_name)
	{
		BINDING::instance()->rebind(iid.to_string().c_str(),ACE_TEXT_ALWAYS_WCHAR(dll_name));
	}
};

#define REGISTER_PROXYSTUB(ns,cls,dll_name) static OOCore_Register register_##cls(ns::cls::IID,#dll_name);
#define REGISTER_PROXYSTUB_NO_NAMESPACE(cls,dll_name) static OOCore_Register register_##cls(cls::IID,#dll_name);
