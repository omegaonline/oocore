#ifndef _OOCORE_TRANSPORT_SERVICE_H_INCLUDED_
#define _OOCORE_TRANSPORT_SERVICE_H_INCLUDED_

#include "./Object.h"

class OOCore_Transport_Service :
	public OOObj::Object
{
public:
	virtual int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key) = 0;
	virtual int CloseChannel(OOObj::cookie_t channel_key) = 0;
	virtual int Connect(OOCore_Transport_Service* reverse) = 0;
	virtual int Disconnect() = 0;
		
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

	int CloseChannel(OOObj::cookie_t channel_key)
	{
		return (marshaller(4) << channel_key)();
	}

	int Connect(OOCore_Transport_Service* reverse)
	{
		return (marshaller(5) << OOCore_Object_Marshaller(reverse))();
	}

	int Disconnect()
	{
		return (marshaller(6))();
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
		add_delegate(4,CloseChannel.bind<OOCore_Transport_Service,&OOCore_Transport_Service::CloseChannel>(obj));
		add_delegate(5,Connect.bind<OOCore_Transport_Service,&OOCore_Transport_Service::Connect>(obj));
		add_delegate(6,Disconnect.bind<OOCore_Transport_Service,&OOCore_Transport_Service::Disconnect>(obj));
	}

private:
	OOObj::Delegate::D2<const OOObj::char_t*,OOObj::cookie_t*> OpenChannel;
	OOObj::Delegate::D1<OOObj::cookie_t> CloseChannel;
	OOObj::Delegate::D1<OOCore_Transport_Service*> Connect;
	OOObj::Delegate::D0 Disconnect;
};

#endif // _OOCORE_TRANSPORT_SERVICE_H_INCLUDED_
