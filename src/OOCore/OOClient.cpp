#include "./Object.h"

#include <ace/ACE.h>

#include "./Connection_Manager.h"
#include "./Object_Factory.h"
#include "./Protocol_Manager.h"

OOCore_Export int 
OOObject::Init(unsigned int threads)
{
	if (OOCore::Impl::g_IsServer)
	{
		errno = EACCES;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")),-1);
	}

	int ret = 0;
	if ((ret = OOCore::Impl::ENGINE::instance()->open(threads)) == 0)
	{
		if ((ret = OOCore::Impl::Connection_Manager::init()) == 0)
		{
		}

		if (ret!=0)
			OOCore::Impl::ENGINE::instance()->close();
	}

	return ret;
}

OOCore_Export void 
OOObject::Term()
{
	if (OOCore::Impl::g_IsServer)
	{
		errno = EISCONN;
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Already initialized as server!\n")));
	}
	else
	{
		OOCore::Impl::CONNECTION_MANAGER::instance()->RequestClose();
				
		OOCore::Impl::ENGINE::instance()->close();
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

OOCore_Export OOObject::int32_t 
OOObject::CreateObject(const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	return OOObject::CreateRemoteObject(0,clsid,pOuter,iid,ppVal);
}

OOCore_Export OOObject::int32_t 
OOObject::CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (!OOCore::Impl::g_IsServer)
		return OOCore::Impl::CONNECTION_MANAGER::instance()->CreateRemoteObject(remote_url,clsid,pOuter,iid,ppVal);

	if (!remote_url)
		return OOCore::Impl::OBJECT_FACTORY::instance()->create_object(clsid,OOCore::ObjectFactory::LOCAL_ONLY,pOuter,iid,ppVal);

	return OOCore::Impl::PROTOCOL_MANAGER::instance()->create_remote_object(remote_url,clsid,pOuter,iid,ppVal);
}
