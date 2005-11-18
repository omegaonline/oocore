#ifndef OOCORE_REGISTER_H_INCLUDED_
#define OOCORE_REGISTER_H_INCLUDED_

#include "./Object.h"

#include "./OOCore_export.h"

namespace Impl
{

class Register
{
public:
	Register(const OOObj::guid_t& iid, const char* dll_name)
	{
		OOObj::RegisterProxyStub(iid,dll_name);
	}
};

};

#define REGISTER_PROXYSTUB(ns,cls,dll_name) static Impl::Register cls##_register(ns::cls::IID,#dll_name);

#endif // OOCORE_REGISTER_H_INCLUDED_
