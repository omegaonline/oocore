#include <OTL/OTL.h>
#include "Test.h"

#include "TestDll/TestDll.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
	printf(ptrTest->Hello());

	OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");

	return false;
}
