#include ".\test_impl.h"

int 
Test_Impl::Array_Test_In(OOObject::uint32_t count, OOObject::uint16_t* pArray)
{
	ACE_OS::printf(ACE_TEXT("Array_Test_In: Received %u items.\n"),count);

	return 0;
}

int 
Test_Impl::Array_Test_Out(OOObject::uint32_t* count, OOObject::uint16_t** pArray)
{
	*count = 14;
	*pArray = (OOObject::uint16_t*)OOObject::Alloc(sizeof(OOObject::uint16_t)* *count);

	ACE_OS::printf(ACE_TEXT("Array_Test_Out: Sending %u items.\n"),*count);

	return (*pArray ? 0 : -1);
}

int 
Test_Impl::Array_Test_InOut(OOObject::uint32_t* count, OOObject::uint16_t** pArray)
{
	ACE_OS::printf(ACE_TEXT("Array_Test_InOut: Received %u items.\n"),*count);

#define RESIZE_TEST 0

#ifdef RESIZE_TEST
	OOObject::Free(*pArray);
	*count = 14;
	*pArray = (OOObject::uint16_t*)OOObject::Alloc(sizeof(OOObject::uint16_t)* *count);
#else
	*count = 10;
#endif

	ACE_OS::printf(ACE_TEXT("Array_Test_InOut: Sending %u items.\n"),*count);

	return (*pArray ? 0 : -1);
}

int 
Test_Impl::Object_Test_In(const OOObject::guid_t& iid, OOObject::Object* pObj)
{
	return 0;
}

int 
Test_Impl::Object_Test_Out(const OOObject::guid_t& iid, OOObject::Object** pObj)
{
	return -1;
}
