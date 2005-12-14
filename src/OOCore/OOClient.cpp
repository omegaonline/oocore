#include "./Object.h"

#include <ace/ACE.h>

#include "./Connection_Manager.h"
#include "./Object_Factory.h"
#include "./Engine.h"

OOCore_Export int 
OOObject::Init(unsigned int threads)
{
	if (OOCore::Impl::g_IsServer)
	{
		errno = EACCES;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")),-1);
	}

	int ret = 0;
	if ((ret = OOCore::ENGINE::instance()->open(threads)) == 0)
	{
		if ((ret = OOCore::Impl::Connection_Manager::init()) == 0)
		{
		}

		if (ret!=0)
			OOCore::ENGINE::instance()->shutdown();
	}

	return ret;
}

OOCore_Export void 
OOObject::Term()
{
	if (OOCore::Impl::g_IsServer)
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")));
	else
	{
		OOCore::Impl::CONNECTION_MANAGER::instance()->close();
		OOCore::Impl::CONNECTION_MANAGER::instance()->shutdown();
		
		OOCore::ENGINE::instance()->shutdown();
	}
}

OOCore_Export OOObject::int32_t 
OOObject::CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (OOCore::Impl::g_IsServer)
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
