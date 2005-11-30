#ifndef OOCORE_OOCORE_PS_H_INCLUDED_
#define OOCORE_OOCORE_PS_H_INCLUDED_

#include "./ProxyStub.h"

namespace OOCore
{
	BEGIN_AUTO_PROXY_STUB(RemoteObjectFactory)
		METHOD(CreateObject,3,((in),const OOObject::guid_t&,clsid,(in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,ppVal))
		METHOD(SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
	END_AUTO_PROXY_STUB()
};

#endif // OOCORE_OOCORE_PS_H_INCLUDED_
