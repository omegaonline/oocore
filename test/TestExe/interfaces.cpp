#include <OTL/OTL.h>

#include "TestDll/TestDll.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::DllTest> ptrTest(L"Test.Dll");
	ptrTest->Hello();

	return false;
}
