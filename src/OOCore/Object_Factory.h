#ifndef OOCORE_OBJECT_FACTORY_H_INCLUDED_
#define OOCORE_OBJECT_FACTORY_H_INCLUDED_

#include <map>

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./OOCore_Util.h"

namespace OOCore
{
namespace Impl
{
class Object_Factory
{
public:
	OOObject::int32_t add_object_factory(const OOObject::guid_t& clsid, ObjectFactory* pFactory);
	OOObject::int32_t remove_object_factory(const OOObject::guid_t& clsid);
	OOObject::int32_t create_object(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

private:
	Object_Factory(void) {}
	virtual ~Object_Factory(void) {}
	friend ACE_Singleton<Object_Factory, ACE_Thread_Mutex>;

	ACE_Thread_Mutex m_lock;
	std::map<OOObject::guid_t,OOCore::Object_Ptr<ObjectFactory> > m_factory_map;
};

typedef ACE_Singleton<Object_Factory, ACE_Thread_Mutex> OBJECT_FACTORY;

};
};

#endif // OOCORE_OBJECT_FACTORY_H_INCLUDED_
