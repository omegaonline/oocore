#ifndef TEST_TEST_H_INCLUDED_
#define TEST_TEST_H_INCLUDED_

#include <OOCore/OOObject.h>
#include <OOCore/ProxyStub.h>

#include "./Test_export.h"

namespace TestNS
{
	class Test : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t Array_Test_In(OOObject::uint32_t count, OOObject::int16_t* pArray) = 0;
		virtual OOObject::int32_t Array_Test_Out(OOObject::uint32_t* count, OOObject::int16_t** pArray) = 0;
		virtual OOObject::int32_t Array_Test_InOut(OOObject::uint32_t* count, OOObject::int16_t** pArray) = 0;

		virtual OOObject::int32_t String_Test_In(const OOObject::char_t* str) = 0;
		virtual OOObject::int32_t String_Test_Out(OOObject::char_t** str) = 0;
		virtual OOObject::int32_t String_Test_InOut(OOObject::char_t** str) = 0;

		DECLARE_IID(Test);
	};

	class Test2 : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t Test2_Hello(OOObject::char_t** str) = 0;

		DECLARE_IID(Test);
	};

	BEGIN_META_INFO(Test)
		METHOD(Array_Test_In,2,((in),OOObject::uint32_t,count,(in)(size_is(count)),OOObject::int16_t*,pArray))	
		METHOD(Array_Test_Out,2,((out),OOObject::uint32_t*,count,(out)(size_is(count)),OOObject::int16_t**,pArray))
		METHOD(Array_Test_InOut,2,((in)(out),OOObject::uint32_t*,count,(in)(out)(size_is(count)),OOObject::int16_t**,pArray))

		METHOD(String_Test_In,1,((in)(string),const OOObject::char_t*,str))
		METHOD(String_Test_Out,1,((out)(string),OOObject::char_t**,str));
		METHOD(String_Test_InOut,1,((in)(out)(string),OOObject::char_t**,str));
	END_META_INFO()

	BEGIN_META_INFO(Test2)
		METHOD(Test2_Hello,1,((out)(string),OOObject::char_t**,str));
	END_META_INFO()
};

DECLARE_OID(Test,Test);

#endif // TEST_TEST_H_INCLUDED_
