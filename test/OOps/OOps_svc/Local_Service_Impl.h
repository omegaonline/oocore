#pragma once

#include "./Local_Service.h"

class OOps_Local_Service_Impl : public OOObj::Object_Impl<OOps_Local_Service>
{
public:
	virtual int QueryInterface(const OOObj::GUID& iid, Object** ppVal);
	virtual int Ping(const OOObj::char_t* remote_host);

	static int Create(OOps_Local_Service** pObj);

private:
	OOps_Local_Service_Impl(void);
	virtual ~OOps_Local_Service_Impl(void);
};
