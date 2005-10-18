#pragma once

#include "../../src/OOCore/Object.h"

#include "./OOps_export.h"

class OOps_Remote_Service : public OOObj::Object
{
public:
	virtual int Ping() = 0;
	
	DECLARE_IID(OOps_Export);
};

class OOps_Remote_Service_Proxy : 
	public OOObj::Object_Proxy<OOps_Remote_Service>
{
public:
	OOps_Remote_Service_Proxy(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler) :
	  OOObj::Object_Proxy<OOps_Remote_Service>(key,handler)
	{
	}

	int Ping()
	{
		return (marshaller(3))();
	}
};

class OOps_Remote_Service_Stub : 
	public OOObj::Object_Stub
{
public:
	OOps_Remote_Service_Stub(OOObj::Object* obj) :
	  OOObj::Object_Stub(obj)
	{
		add_delegate(3,Ping.bind<OOps_Remote_Service,&OOps_Remote_Service::Ping>(obj));
	}

private:
	OOObj::Delegate::D0 Ping;
};


