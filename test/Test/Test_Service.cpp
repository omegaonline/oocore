#include "./Test_Service.h"
#include "./Test_Impl.h"

ACE_FACTORY_DEFINE(Test,Test_Service)

int 
Test_Service::init(int argc, ACE_TCHAR *argv[])
{
	AddRef();
	return OOCore::AddObjectFactory(CLSID_Test,this);
}

int 
Test_Service::fini(void)
{
	return 0;//OOCore::RemoveObjectFactory(CLSID_Test);
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