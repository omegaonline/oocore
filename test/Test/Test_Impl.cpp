#include "./Test_Impl.h"

OOObject::int32_t 
Test_Impl::Array_Test_In(OOObject::uint32_t count, OOObject::int16_t* pArray)
{
	ACE_OS::printf(ACE_TEXT("Array_Test_In: Received %u items.\n"),count);

	return 0;
}

OOObject::int32_t 
Test_Impl::Array_Test_Out(OOObject::uint32_t* count, OOObject::int16_t** pArray)
{
	*count = 11;
	*pArray = (OOObject::int16_t*)OOObject::Alloc(sizeof(OOObject::int16_t)* *count);

	ACE_OS::printf(ACE_TEXT("Array_Test_Out: Sending %u items.\n"),*count);

	return (*pArray ? 0 : -1);
}

OOObject::int32_t 
Test_Impl::Array_Test_InOut(OOObject::uint32_t* count, OOObject::int16_t** pArray)
{
	ACE_OS::printf(ACE_TEXT("Array_Test_InOut: Received %u items.\n"),*count);

#define RESIZE_TEST 1

#if (RESIZE_TEST==1)
	OOObject::Free(*pArray);
	*count = 13;
	*pArray = (OOObject::int16_t*)OOObject::Alloc(sizeof(OOObject::int16_t)* *count);
#else
	*count = 9;
#endif

	ACE_OS::printf(ACE_TEXT("Array_Test_InOut: Sending %u items.\n"),*count);

	return (*pArray ? 0 : -1);
}

OOObject::int32_t 
Test_Impl::String_Test_In(const OOObject::char_t* str)
{
	ACE_OS::printf(ACE_TEXT("String_Test_In: Received \"%s\".\n"),str);

	return 0;
}

OOObject::int32_t 
Test_Impl::String_Test_Out(OOObject::char_t** str)
{
	*str = "Mr. Watson -- Come here -- I want to see you.";

	ACE_OS::printf(ACE_TEXT("String_Test_Out: Sending \"%s\".\n"),*str);

	return 0;
}

OOObject::int32_t 
Test_Impl::String_Test_InOut(OOObject::char_t** str)
{
	ACE_OS::printf(ACE_TEXT("String_Test_InOut: Received \"%s\".\n"),*str);

	*str =	"Leave the beaten track occasionally and dive into the woods. "
			"Every time you do so you will be certain to find something that you have never seen before. "
			"Follow it up, explore all around it, and before you know it, "
			"you will have something worth thinking about to occupy your mind. "
			"All really big discoveries are the results of thought."
			"\n\n\t-- Alexander Graham Bell";

	ACE_OS::printf(ACE_TEXT("String_Test_InOut: Sending \"%s\".\n"),*str);

	return 0;
}
