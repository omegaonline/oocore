#include "./PSMap.h"

void 
OOCore::Impl::PSMap::insert(const OOObject::cookie_t& key, const OOObject::guid_t& iid, OOObject::Object* obj)
{
	m_forward_map.insert(std::map<OOObject::cookie_t,std::pair<OOObject::Object*,OOObject::guid_t> >::value_type(key,std::pair<OOObject::Object*,OOObject::guid_t>(obj,iid)));
	m_reverse_map.insert(std::map<std::pair<OOObject::Object*,OOObject::guid_t>,OOObject::cookie_t>::value_type(std::pair<OOObject::Object*,OOObject::guid_t>(obj,iid),key));
}

bool 
OOCore::Impl::PSMap::find(const OOObject::cookie_t& key, OOObject::Object*& obj)
{
	std::map<OOObject::cookie_t,std::pair<OOObject::Object*,OOObject::guid_t> >::iterator i=m_forward_map.find(key);
	if (i!=m_forward_map.end())
	{
		obj = i->second.first;
		return true;
	}
	return false;
}

bool 
OOCore::Impl::PSMap::find(OOObject::Object* obj, const OOObject::guid_t& iid, OOObject::cookie_t& key)
{
	std::map<std::pair<OOObject::Object*,OOObject::guid_t>,OOObject::cookie_t>::iterator i=m_reverse_map.find(std::pair<OOObject::Object*,OOObject::guid_t>(obj,iid));
	if (i!=m_reverse_map.end())
	{
		key = i->second;
		return true;
	}
	return false;
}

bool 
OOCore::Impl::PSMap::remove(const OOObject::cookie_t& key)
{
	std::map<OOObject::cookie_t,std::pair<OOObject::Object*,OOObject::guid_t> >::iterator i=m_forward_map.find(key);
	if (i!=m_forward_map.end())
	{
		m_reverse_map.erase(i->second);
		m_forward_map.erase(i);
		return true;
	}
	return false;
}

void 
OOCore::Impl::PSMap::remove_all()
{
	m_forward_map.clear();
	m_reverse_map.clear();
}