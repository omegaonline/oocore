#ifndef TEST_TEST_IMPL_H_INCLUDED_
#define TEST_TEST_IMPL_H_INCLUDED_

#include "./Test.h"

class Test_Impl : 
	public OOCore::Object_Impl<Test::Test>
{
// Test members
public:
	int Array_Test_In(OOObject::uint32_t count, OOObject::uint16_t* pArray);
	int Array_Test_Out(OOObject::uint32_t* count, OOObject::uint16_t** pArray);
	int Array_Test_InOut(OOObject::uint32_t* count, OOObject::uint16_t** pArray);
	int Object_Test_In(const OOObject::guid_t& iid, OOObject::Object* pObj);
	int Object_Test_Out(const OOObject::guid_t& iid, OOObject::Object** pObj);
};

#endif // TEST_TEST_IMPL_H_INCLUDED_
