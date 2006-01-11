#ifndef TEST_TEST_IMPL_H_INCLUDED_
#define TEST_TEST_IMPL_H_INCLUDED_

#include "./Test.h"

class Test_Impl : 
	public OOCore::Object_Impl<Test::Test>
{
// Test members
public:
	OOObject::int32_t Array_Test_In(OOObject::uint32_t count, OOObject::int16_t* pArray);
	OOObject::int32_t Array_Test_Out(OOObject::uint32_t* count, OOObject::int16_t** pArray);
	OOObject::int32_t Array_Test_InOut(OOObject::uint32_t* count, OOObject::int16_t** pArray);
};

#endif // TEST_TEST_IMPL_H_INCLUDED_
