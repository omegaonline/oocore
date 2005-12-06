#include "./Object_Factory.h"

OOObject::int32_t 
OOCore::Impl::Object_Factory::add_object_factory(const OOObject::guid_t& clsid, ObjectFactory* pFactory)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	m_factory_map.insert(std::map<OOObject::guid_t,OOCore::Object_Ptr<ObjectFactory> >::value_type(clsid,pFactory));
	
	return 0;
}

OOObject::int32_t 
OOCore::Impl::Object_Factory::remove_object_factory(const OOObject::guid_t& clsid)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	return (m_factory_map.erase(clsid)==1 ? 0 : -1);
}

OOObject::int32_t 
OOCore::Impl::Object_Factory::create_object(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<OOObject::guid_t,OOCore::Object_Ptr<ObjectFactory> >::iterator i=m_factory_map.find(clsid);

	guard.release();

	if (i==m_factory_map.end())
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No class factory\n")),-1);

	return i->second->CreateObject(clsid,iid,ppVal);
}
