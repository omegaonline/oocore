#pragma once

#include "../../src/OOCore/Object.h"

#include "./OOps_export.h"

class OOps_Local_Service : public OOObj::Object
{
public:
	virtual int Ping(const OOObj::char_t* remote_host) = 0;
	
	DECLARE_IID(OOps_Export);
};

class OOps_Local_Service_Proxy : 
	public OOObj::Object_Proxy<OOps_Local_Service>
{
public:
	OOps_Local_Service_Proxy(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler) :
	  OOObj::Object_Proxy<OOps_Local_Service>(key,handler)
	{
	}

	int Ping(const OOObj::char_t* remote_host)
	{
		return (marshaller(3) << remote_host)();
	}
};

class OOps_Local_Service_Stub : 
	public OOObj::Object_Stub
{
public:
	OOps_Local_Service_Stub(OOObj::Object* obj) :
	  OOObj::Object_Stub(obj)
	{
		add_delegate(3,Ping.bind<OOps_Local_Service,&OOps_Local_Service::Ping>(obj));
	}

private:
	OOObj::Delegate::D1<const OOObj::char_t*> Ping;
};

