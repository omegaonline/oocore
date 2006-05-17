#ifndef TEST_TEST_IMPL_H_INCLUDED_
#define TEST_TEST_IMPL_H_INCLUDED_

#include "./Test.h"

class Test_Impl : 
	public OOUtil::Object_Root<Test_Impl>,
	public TestNS::Test,
	public TestNS::Test2
{
public:

BEGIN_INTERFACE_MAP(Test_Impl)
	INTERFACE_ENTRY(TestNS::Test)
	INTERFACE_ENTRY(TestNS::Test2)
END_INTERFACE_MAP()

// Test members
public:
	OOObject::int32_t Array_Test_In(OOObject::uint32_t count, OOObject::int16_t* pArray);
	OOObject::int32_t Array_Test_Out(OOObject::uint32_t* count, OOObject::int16_t** pArray);
	OOObject::int32_t Array_Test_InOut(OOObject::uint32_t* count, OOObject::int16_t** pArray);

	OOObject::int32_t String_Test_In(const OOObject::char_t* str);
	OOObject::int32_t String_Test_Out(OOObject::char_t** str);
	OOObject::int32_t String_Test_InOut(OOObject::char_t** str);

// Test2 members
public:
	OOObject::int32_t Test2_Hello(OOObject::char_t** str);
};

#endif // TEST_TEST_IMPL_H_INCLUDED_
