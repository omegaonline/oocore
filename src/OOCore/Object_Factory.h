#ifndef OOCORE_OBJECT_FACTORY_H_INCLUDED_
#define OOCORE_OBJECT_FACTORY_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include <map>

#include "./OOCore_Util.h"

namespace OOCore
{
namespace Impl
{
class Object_Factory
{
public:
	OOObject::int32_t add_object_factory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, ObjectFactory* pFactory);
	OOObject::int32_t remove_object_factory(const OOObject::guid_t& clsid);
	OOObject::int32_t create_object(const OOObject::guid_t& clsid, ObjectFactory::Flags_t flags, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal);

private:
	Object_Factory(void) {}
	virtual ~Object_Factory(void) {}
	friend class ACE_Singleton<Object_Factory, ACE_Thread_Mutex>;

	ACE_Thread_Mutex m_lock;

	typedef std::map<OOObject::guid_t,std::pair<OOCore::Object_Ptr<ObjectFactory>,OOCore::ObjectFactory::Flags_t> > map_type;
	map_type m_factory_map;
};

typedef ACE_Singleton<Object_Factory, ACE_Thread_Mutex> OBJECT_FACTORY;

};
};

#endif // OOCORE_OBJECT_FACTORY_H_INCLUDED_
