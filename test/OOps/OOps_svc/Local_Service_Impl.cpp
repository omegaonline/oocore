#include "./Local_Service_Impl.h"

#include "../../src/OOSvc/Transport_Manager.h"

#include "./Remote_Service.h"

DEFINE_IID(OOps_Local_Service,5475B7AB-258A-4c06-86A2-F29091358D58);

REGISTER_PROXYSTUB_NO_NAMESPACE(OOps_Local_Service,OOps);

OOps_Local_Service_Impl::OOps_Local_Service_Impl(void)
{
}

OOps_Local_Service_Impl::~OOps_Local_Service_Impl(void)
{
}

int OOps_Local_Service_Impl::Create(OOps_Local_Service** pObj)
{
	ACE_NEW_RETURN(*pObj,OOps_Local_Service_Impl,-1);
	
	(*pObj)->AddRef();
	return 0;
}

int OOps_Local_Service_Impl::QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	if (iid == OOObj::Object::IID ||
		iid == OOps_Local_Service::IID)
	{
		*ppVal = this;
		AddRef();
		return 0;
	}

	return -1;
}

int OOps_Local_Service_Impl::Ping(const OOObj::char_t* remote_host)
{
	ACE_CString strURL(remote_host);

	strURL += "/OOps_Remote";

	OOObj::Object_Ptr<OOps_Remote_Service> ptrRemote;
	int ret = TRANSPORT_MANAGER::instance()->create_remote_object(strURL.c_str(),OOps_Remote_Service::IID,reinterpret_cast<OOObj::Object**>(&ptrRemote));
	if (ret == 0)
	{
		ret = ptrRemote->Ping();
	}

	return ret;
}