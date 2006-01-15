#include "./Test_Service.h"
#include "./Test_Impl.h"

ACE_FACTORY_DEFINE(Test,Test_Service)


DEFINE_IID(Test::Test,6AAE8C33-699A-4414-AF84-25E74E693207);
DEFINE_CLSID(Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

BEGIN_META_INFO_MAP_EX(Test,OOTest)
	META_INFO_ENTRY(Test::Test)
END_META_INFO_MAP()

int 
Test_Service::init(int argc, ACE_TCHAR *argv[])
{
	// Artifically increment our RefCount, the ACE_Svc_Config will delete us
    AddRef();

	// Register ourselves... Its just easier!
	RegisterLib(true);

	return OOCore::AddObjectFactory(OOCore::ObjectFactory::USAGE_ANY,CLSID_Test,this);
}

int 
Test_Service::fini(void)
{
	return OOCore::RemoveObjectFactory(CLSID_Test);
}

OOObject::int32_t 
Test_Service::CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (!ppVal)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer!\n")),-1);
	}

	ACE_NEW_RETURN(*ppVal,Test_Impl,-1);
	(*ppVal)->AddRef();

	return 0;
}
