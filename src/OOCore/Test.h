#ifndef OOCORE_TEST_H_INCLUDED_
#define OOCORE_TEST_H_INCLUDED_

#include "./Object.h"
#include "./ProxyStub.h"

namespace OOCore
{
	// TODO: MOVE THIS ELSEWHERE!
	class Server : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t Stop(OOObject::bool_t force, OOObject::uint16_t* remaining) = 0;
		virtual OOObject::int32_t StopPending(OOObject::bool_t* pending) = 0;
		virtual OOObject::int32_t StayAlive() = 0;

		DECLARE_IID(OOCore);
	};

	BEGIN_META_INFO(Server)
		METHOD(Stop,2,((in),OOObject::bool_t,force,(out),OOObject::uint16_t*,remaining))
		METHOD(StopPending,1,((out),OOObject::bool_t*,pending))
		METHOD(StayAlive,0,())
	END_META_INFO()
};

#endif // OOCORE_TEST_H_INCLUDED_
