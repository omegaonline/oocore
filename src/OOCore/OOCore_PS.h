#ifndef OOCORE_OOCORE_PS_H_INCLUDED_
#define OOCORE_OOCORE_PS_H_INCLUDED_

#include "./ProxyStub.h"

namespace OOCore
{
	BEGIN_AUTO_PROXY_STUB(RemoteObjectFactory)
		METHOD(CreateObject,3,((in)(string),const OOObject::char_t*,class_name,(in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,ppVal))
		METHOD(SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
	END_AUTO_PROXY_STUB()

	BEGIN_AUTO_PROXY_STUB(Server)
		METHOD(Stop,2,((in),OOObject::bool_t,force,(out),OOObject::uint16_t*,remaining))
		METHOD(StopPending,1,((out),OOObject::bool_t*,pending))
		METHOD(StayAlive,0,())
	END_AUTO_PROXY_STUB()
};

#endif // OOCORE_OOCORE_PS_H_INCLUDED_
