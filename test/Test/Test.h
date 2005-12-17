#ifndef TEST_TEST_H_INCLUDED_
#define TEST_TEST_H_INCLUDED_

#include <OOCore/Object.h>
#include <OOCore/ProxyStub.h>

#include "./Test_export.h"

namespace Test
{
	class Test : public OOObject::Object
	{
	public:
		virtual int Array_Test_In(OOObject::uint32_t count, OOObject::uint16_t* pArray) = 0;
		virtual int Array_Test_Out(OOObject::uint32_t* count, OOObject::uint16_t** pArray) = 0;
		virtual int Array_Test_InOut(OOObject::uint32_t* count, OOObject::uint16_t** pArray) = 0;

		virtual int Object_Test_In(const OOObject::guid_t& iid, OOObject::Object* pObj) = 0;
		virtual int Object_Test_Out(const OOObject::guid_t& iid, OOObject::Object** pObj) = 0;
		
		DECLARE_IID(Test);
	};

	BEGIN_AUTO_PROXY_STUB(Test)
		METHOD(Array_Test_Out,2,((out),OOObject::uint32_t*,count,(out)(size_is(count)),OOObject::uint16_t**,pArray))
		METHOD(Array_Test_In,2,((in),OOObject::uint32_t,count,(in)(size_is(count)),OOObject::uint16_t*,pArray))	
		METHOD(Array_Test_InOut,2,((in)(out),OOObject::uint32_t*,count,(in)(out)(size_is(count)),OOObject::uint16_t**,pArray))

		METHOD(Object_Test_Out,2,((in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,pObj))
		METHOD(Object_Test_In,2,((in),const OOObject::guid_t&,iid,(in)(iid_is(iid)),OOObject::Object*,pObj))
	END_AUTO_PROXY_STUB()
};

DECLARE_CLSID(Test,Test);

#endif // TEST_TEST_H_INCLUDED_
