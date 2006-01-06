#include "./Object_Factory.h"

OOObject::int32_t 
OOCore::Impl::Object_Factory::add_object_factory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, ObjectFactory* pFactory)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	m_factory_map.insert(map_type::value_type(clsid,std::pair<OOCore::Object_Ptr<ObjectFactory>,OOCore::ObjectFactory::Flags_t>(pFactory,flags)));
	
	return 0;
}

OOObject::int32_t 
OOCore::Impl::Object_Factory::remove_object_factory(const OOObject::guid_t& clsid)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	return (m_factory_map.erase(clsid)==1 ? 0 : -1);
}

OOObject::int32_t 
OOCore::Impl::Object_Factory::create_object(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	map_type::iterator i=m_factory_map.find(clsid);
	if (i==m_factory_map.end() || (flags & i->second.second)==0 || i->second.first==0)
	{
		errno = ENOENT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No class factory\n")),-1);
	}

	Object_Ptr<ObjectFactory> fact = i->second.first;

	guard.release();

	return fact->CreateObject(clsid,iid,ppVal);
}
