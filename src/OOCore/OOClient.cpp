#include "./Object.h"

#include <ace/Init_ACE.h>

#include "./Connection_Manager.h"
#include "./Engine.h"
#include "./Binding.h"

OOCore_Export int 
OOObj::Init()
{
	int ret = 0;

	// Make sure ACE is loaded
	if ((ret = (ACE::init()==-1 ? -1 : 0)) == 0)
	{
		if ((ret = OOCore::ENGINE::instance()->open()) == 0)
		{
			if ((ret = Impl::Connection_Manager::init()) == 0)
			{
			}

			if (ret!=0)
				OOCore::ENGINE::instance()->shutdown();
		}

		if (ret!=0)
			ACE::fini();
	}

	return ret;
}

OOCore_Export void 
OOObj::Term()
{
	Impl::CONNECTION_MANAGER::instance()->close();

	OOCore::ENGINE::instance()->shutdown();

	ACE::fini();
}

OOCore_Export int 
OOObj::CreateObject(const OOObj::string_t class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal)
{
	return Impl::CONNECTION_MANAGER::instance()->CreateObject(class_name,iid,ppVal);
}

OOCore_Export void* 
OOObj::Alloc(size_t size)
{
	return ACE_OS::malloc(size);
}

OOCore_Export void 
OOObj::Free(void* p)
{
	ACE_OS::free(p);
}

OOCore_Export int 
OOObj::RegisterProxyStub(const OOObj::guid_t& iid, const char* dll_name)
{
	return Impl::BINDING::instance()->rebind(Impl::guid_to_string(iid).c_str(),ACE_TEXT_ALWAYS_WCHAR(dll_name));
}
