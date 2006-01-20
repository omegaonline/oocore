#ifndef OOCORE_PSMAP_H_INCLUDED_
#define OOCORE_PSMAP_H_INCLUDED_

#include "./OOCore_Impl.h"

#include <map>

namespace OOCore
{

namespace Impl
{

class PSMap
{
public:
	void insert(const OOObject::uint32_t& key, const OOObject::guid_t& iid, OOObject::Object* obj);
	bool find(const OOObject::uint32_t& key, OOObject::Object*& obj);
	bool find(OOObject::Object* obj, const OOObject::guid_t& iid, OOObject::uint32_t& key);
	bool remove(const OOObject::uint32_t& key);
	void remove_all();

private:
	std::map<OOObject::uint32_t,std::pair<OOObject::Object*,OOObject::guid_t> > m_forward_map;
	std::map<std::pair<OOObject::Object*,OOObject::guid_t>,OOObject::uint32_t> m_reverse_map;
};

};
};

#endif // OOCORE_PSMAP_H_INCLUDED_
