#ifndef TEST_TEST_SERVICE_H_INCLUDED_
#define TEST_TEST_SERVICE_H_INCLUDED_

#include <ace/Service_Object.h>

#include <OOCore/OOCore_Util.h>

class Test_Service : 
	public ACE_Service_Object,
	public OOCore::Object_Impl<OOCore::ObjectFactory>
{
public:
	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

// ObjectFactory members
public:
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
};

#endif // TEST_TEST_SERVICE_H_INCLUDED_
