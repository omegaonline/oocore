#ifndef OONET_KADEMLIA_H_INCLUDED_
#define OONET_KADEMLIA_H_INCLUDED_

#include "../OOCore/Object.h"
#include "../OOCore/ProxyStub.h"

#include "./OONet_export.h"

namespace Kademlia
{
	class RPC_Request : public OOObject::Object
	{
	public:
		
		DECLARE_IID(OONet);
	};

	BEGIN_META_INFO(RPC_Request)
		
	END_META_INFO()

	class RPC_Response : public OOObject::Object
	{
	public:
		
		DECLARE_IID(OONet);
	};

	BEGIN_META_INFO(RPC_Response)
		
	END_META_INFO()

};

#endif // OONET_KADEMLIA_H_INCLUDED_
