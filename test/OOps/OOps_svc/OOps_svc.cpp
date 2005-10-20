#include "../../../src/OOCore/Object.h"

#include "./Local_Service.h"
#include "./Remote_Service.h"

#include "./OOps_export.h"

extern "C" int OOps_Export CreateStub(const OOObj::GUID& iid, OOObj::Object* obj, OOCore_Object_Stub_Base** stub)
{
	if (iid==OOps_Local_Service::IID)
		ACE_NEW_RETURN(*stub,OOps_Local_Service_Stub(obj),-1);
	else if (iid==OOps_Remote_Service::IID)
		ACE_NEW_RETURN(*stub,OOps_Remote_Service_Stub(obj),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Stub IID\n")),-1);
	
	return 0;
}

extern "C" OOps_Export int CreateProxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler, OOObj::Object** proxy)
{
	if (iid==OOps_Local_Service::IID)
		ACE_NEW_RETURN(*proxy,OOps_Local_Service_Proxy(key,handler),-1);
	else if (iid==OOps_Remote_Service::IID)
		ACE_NEW_RETURN(*proxy,OOps_Remote_Service_Proxy(key,handler),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Proxy IID\n")),-1);
	
	(*proxy)->AddRef();
	return 0;
}