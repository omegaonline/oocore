#include <OTL/OTL.h>
#include "Test.h"

#include "TestDll/TestDll.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::DllTest> ptrTest(L"Test.Dll");
	wprintf(ptrTest->Hello());

	OTL::ObjectPtr<Test::DllTest> ptrTest2(L"Test.Dll");

	return false;
}
