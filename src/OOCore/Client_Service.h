#ifndef _OOCORE_CLIENT_SERVICE_H_INCLUDED_
#define _OOCORE_CLIENT_SERVICE_H_INCLUDED_

#include "./Object.h"

namespace OOObj
{
class Client_Service :
	public Object
{
public:
	virtual int Stop(OOObj::bool_t force, OOObj::uint16_t* remaining) = 0;
	virtual int StopPending(OOObj::bool_t* pending) = 0;
	virtual int StayAlive() = 0;

	// Test members
	virtual int Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray) = 0;
	virtual int Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray) = 0;
	virtual int Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray) = 0;

	DECLARE_IID(OOCore_Export);
};

class Client_Service_Proxy : 
	public Object_Proxy<OOObj::Client_Service>
{
public:
	Client_Service_Proxy(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler) :
	  Object_Proxy<OOObj::Client_Service>(key,handler)
	{ }

	int Stop(OOObj::bool_t force, OOObj::uint16_t* remaining)
	{
		int ret = (marshaller(3) << force << remaining)();
		if (force && handler()->closed())
			ret = 0;
		
		return ret;
	}

	int StopPending(OOObj::bool_t* pending)
	{
		return (marshaller(4) << pending)();
	}

	int StayAlive()
	{
		return marshaller(5)();
	}

	int Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray)
	{
		return (marshaller(6) << count << OOCore_Array_Marshaller<OOObj::uint16_t>(0,pArray))();
	}

	int Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray)
	{
		return (marshaller(7) << count << OOCore_Array_Marshaller<OOObj::uint16_t>(0,pArray))();
	}

	int Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray)
	{
		return (marshaller(8) << count << OOCore_Array_Marshaller<OOObj::uint16_t>(0,pArray,true))();
	}
};

class Client_Service_Stub : 
	public Object_Stub
{
public:
	Client_Service_Stub(Object* obj) :
	  Object_Stub(obj)
	{
		add_delegate(3,Stop.bind<Client_Service,&Client_Service::Stop>(obj));
		add_delegate(4,StopPending.bind<Client_Service,&Client_Service::StopPending>(obj));
		add_delegate(5,StayAlive.bind<Client_Service,&Client_Service::StayAlive>(obj));
		add_delegate(6,Array_Test_In.bind<Client_Service,&Client_Service::Array_Test_In>(obj));
		add_delegate(7,Array_Test_Out.bind<Client_Service,&Client_Service::Array_Test_Out>(obj));
		add_delegate(8,Array_Test_InOut.bind<Client_Service,&Client_Service::Array_Test_InOut>(obj));
	}

private:
	Delegate::D2<OOObj::bool_t,OOObj::uint16_t*> Stop;
	Delegate::D1<OOObj::bool_t*> StopPending;
	Delegate::D0 StayAlive;

	Delegate::D2<OOObj::uint32_t,OOObj::uint16_t*> Array_Test_In;
	Delegate::D2<OOObj::uint32_t*,OOObj::uint16_t**> Array_Test_Out;
	Delegate::D2<OOObj::uint32_t*,OOObj::uint16_t**> Array_Test_InOut;
};

};

#endif // _OOCORE_CLIENT_SERVICE_H_INCLUDED_
