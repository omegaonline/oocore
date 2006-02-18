#include "./Test_Service.h"
#include "./Test_Impl.h"

ACE_FACTORY_DEFINE(Test,Test_Service)

DEFINE_IID(TestNS::Test,6AAE8C33-699A-4414-AF84-25E74E693207);
DEFINE_IID(TestNS::Test2,911D26B6-8539-4b86-A0EE-C978342F0C7B);
DEFINE_CLSID(Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

BEGIN_META_INFO_MAP_EX(Test,OOTest)
	META_INFO_ENTRY(TestNS::Test)
	META_INFO_ENTRY(TestNS::Test2)
END_META_INFO_MAP()

int 
Test_Service::init(int argc, ACE_TCHAR *argv[])
{
	// Artifically increment our RefCount, the ACE_Svc_Config will delete us
    AddRef();

	// Register ourselves... Its just easier!
	RegisterLib(true);

	if (OOCore::RegisterObjectFactory(OOCore::ObjectFactory::USAGE_ANY,CLSID_Test,this) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to register Test object factory\n")),-1);

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Test-Service started.  Test away!\n")));

	return 0;
}

int 
Test_Service::fini(void)
{
	return OOCore::UnregisterObjectFactory(CLSID_Test);
}

OOObject::int32_t 
Test_Service::CreateObject(const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (!ppVal)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer!\n")),-1);
	}

	Test_Impl* pTest;
	ACE_NEW_RETURN(pTest,Test_Impl,-1);

	OOObject::int32_t res = pTest->QueryInterface(iid,ppVal);
	if (res != 0)
		delete pTest;

	return res;
}
