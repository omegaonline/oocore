#ifndef OOCORE_PSMAP_H_INCLUDED_
#define OOCORE_PSMAP_H_INCLUDED_

#include <map>

#include "./OOCore_Impl.h"

namespace OOCore
{

namespace Impl
{

class PSMap
{
public:
	void insert(const OOCore::ProxyStubManager::cookie_t& key, const OOObject::guid_t& iid, OOObject::Object* obj);
	bool find(const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object*& obj);
	bool find(OOObject::Object* obj, const OOObject::guid_t& iid, OOCore::ProxyStubManager::cookie_t& key);
	bool remove(const OOCore::ProxyStubManager::cookie_t& key);
	void remove_all();

private:
	std::map<OOCore::ProxyStubManager::cookie_t,std::pair<OOObject::Object*,OOObject::guid_t> > m_forward_map;
	std::map<std::pair<OOObject::Object*,OOObject::guid_t>,OOCore::ProxyStubManager::cookie_t> m_reverse_map;
};

};
};

#endif // OOCORE_PSMAP_H_INCLUDED_
