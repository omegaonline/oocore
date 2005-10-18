#pragma once

#include "./Remote_Service.h"

class OOps_Remote_Service_Impl : public OOObj::Object_Impl<OOps_Remote_Service>
{
public:
	virtual int QueryInterface(const OOObj::GUID& iid, Object** ppVal);
	virtual int Ping();

	static int Create(OOps_Remote_Service** pObj);

private:
	OOps_Remote_Service_Impl(void);
	virtual ~OOps_Remote_Service_Impl(void);
};
