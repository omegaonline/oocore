#include <OTL/OTL.h>
#include "TestDll/TestDll.h"

#include "Test.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::DllTest> ptrTest(L"Test.Dll");
	
	TEST(ptrTest->Hello() == L"Hello");

	return true;
}
