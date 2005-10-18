#include "./Remote_Service_Impl.h"

DEFINE_IID(OOps_Remote_Service,6CBD7088-5D2C-49d8-889A-AF99BA63B084);

REGISTER_PROXYSTUB_NO_NAMESPACE(OOps_Remote_Service,OOps);

OOps_Remote_Service_Impl::OOps_Remote_Service_Impl(void)
{
}

OOps_Remote_Service_Impl::~OOps_Remote_Service_Impl(void)
{
}

int OOps_Remote_Service_Impl::Create(OOps_Remote_Service** pObj)
{
	ACE_NEW_RETURN(*pObj,OOps_Remote_Service_Impl,-1);
	
	(*pObj)->AddRef();
	return 0;
}

int OOps_Remote_Service_Impl::QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	if (iid == OOObj::Object::IID ||
		iid == OOps_Remote_Service::IID)
	{
		*ppVal = this;
		AddRef();
		return 0;
	}

	return -1;
}

int OOps_Remote_Service_Impl::Ping()
{
	// If it got here, its a success!
	return 0;
}
