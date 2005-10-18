#pragma once

#include "./Object.h"
#include "./Transport_Base.h"

class OOCore_Transport_Service :
	public OOObj::Object
{
public:
	virtual int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key) = 0;
	virtual int SetReverse(OOCore_Transport_Service* reverse) = 0;
		
	DECLARE_IID(OOCore_Export);
};

class OOCore_Transport_Service_Proxy : 
	public OOObj::Object_Proxy<OOCore_Transport_Service>
{
public:
	OOCore_Transport_Service_Proxy(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler) :
	  OOObj::Object_Proxy<OOCore_Transport_Service>(key,handler)
	{
	}

	int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key)
	{
		return (marshaller(3) << name << channel_key)();
	}

	int SetReverse(OOCore_Transport_Service* reverse)
	{
		return (marshaller(4) << OOCore_Object_Marshaller(reverse))();
	}
};

class OOCore_Transport_Service_Stub : 
	public OOObj::Object_Stub
{
public:
	OOCore_Transport_Service_Stub(OOObj::Object* obj) :
	  OOObj::Object_Stub(obj)
	{
		add_delegate(3,OpenChannel.bind<OOCore_Transport_Service,&OOCore_Transport_Service::OpenChannel>(obj));
		add_delegate(4,SetReverse.bind<OOCore_Transport_Service,&OOCore_Transport_Service::SetReverse>(obj));
	}

private:
	OOObj::Delegate::D2<const OOObj::char_t*,OOObj::cookie_t*> OpenChannel;
	OOObj::Delegate::D1<OOCore_Transport_Service*> SetReverse;
};
