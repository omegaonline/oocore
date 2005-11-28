#ifndef OOCORE_OOCORE_PS_H_INCLUDED_
#define OOCORE_OOCORE_PS_H_INCLUDED_

#include "./ProxyStub.h"

namespace OOCore
{

	BEGIN_AUTO_PROXY_STUB(RemoteObjectFactory)
		METHOD(CreateObject,3,((in)(string),const OOObj::char_t*,class_name,(in),const OOObj::guid_t&,iid,(out)(iid_is(iid)),OOObj::Object**,ppVal))
		METHOD(SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
	END_AUTO_PROXY_STUB()


/*	BEGIN_DECLARE_AUTO_PROXY_STUB(RemoteObjectFactory)
		BEGIN_PROXY_MAP()
			
		END_PROXY_MAP()
		
		BEGIN_STUB_MAP()
			STUB_ENTRY_3(CreateObject,const OOObj::char_t*, const OOObj::guid_t&, OOObj::Object**)
			STUB_ENTRY_1(SetReverse,RemoteObjectFactory*)
		END_STUB_MAP()
	END_DECLARE_AUTO_PROXY_STUB()

	BEGIN_DECLARE_AUTO_PROXY_STUB(Server)
		BEGIN_PROXY_MAP()
			PROXY_ENTRY_2(Stop,OOObj::bool_t force, OOObj::uint16_t* remaining)
				PROXY_PARAMS_2(force,remaining)
			PROXY_ENTRY_1(StopPending,OOObj::bool_t* pending)
				PROXY_PARAMS_1(pending)
			PROXY_ENTRY_0(StayAlive)
		END_PROXY_MAP()
		
		BEGIN_STUB_MAP()
			STUB_ENTRY_2(Stop,OOObj::bool_t,OOObj::uint16_t*)
			STUB_ENTRY_1(StopPending,OOObj::bool_t*)
			STUB_ENTRY_0(StayAlive)
		END_STUB_MAP()
	END_DECLARE_AUTO_PROXY_STUB()*/
};

#endif // OOCORE_OOCORE_PS_H_INCLUDED_
