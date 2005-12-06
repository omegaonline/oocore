#include "./Object.h"

#include <ace/Init_ACE.h>

#include "./Connection_Manager.h"
#include "./Object_Factory.h"
#include "./Engine.h"
#include "./Binding.h"

#ifdef _DEBUG
#include "./Test.h"
#endif

OOCore_Export int 
OOObject::Init()
{
	if (g_IsServer)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")),-1);

	int ret = 0;

	// Make sure ACE is loaded
	if ((ret = (ACE::init()==-1 ? -1 : 0)) == 0)
	{
		#ifdef _DEBUG
			OOCore::RegisterProxyStub(OOCore::Server::IID,"OOCore");
			OOCore::RegisterProxyStub(OOCore::Test::IID,"OOCore");
		#endif

		if ((ret = OOCore::ENGINE::instance()->open()) == 0)
		{
			if ((ret = OOCore::Impl::Connection_Manager::init()) == 0)
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
OOObject::Term()
{
	if (g_IsServer)
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")));
	else
	{
		OOCore::Impl::CONNECTION_MANAGER::instance()->close();
		OOCore::Impl::CONNECTION_MANAGER::instance()->shutdown();
		
		OOCore::ENGINE::instance()->shutdown();

		ACE::fini();
	}
}

OOCore_Export OOObject::int32_t 
OOObject::CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (g_IsServer)
	{
		return OOCore::Impl::OBJECT_FACTORY::instance()->create_object(clsid,iid,ppVal);
	}
	else
	{
		return OOCore::Impl::CONNECTION_MANAGER::instance()->CreateObject(clsid,iid,ppVal);
	}
}

OOCore_Export void* 
OOObject::Alloc(size_t size)
{
	return ACE_OS::malloc(size);
}

OOCore_Export void 
OOObject::Free(void* p)
{
	ACE_OS::free(p);
}

OOCore_Export int 
OOCore::RegisterProxyStub(const OOObject::guid_t& iid, const char* dll_name)
{
	ACE_TString value(OOCore::Impl::guid_to_string(iid));

	return OOCore::Impl::BINDING::instance()->rebind(value.c_str(),ACE_TEXT_ALWAYS_WCHAR(dll_name));
}
