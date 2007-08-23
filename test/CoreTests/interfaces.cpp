#include <OTL/OTL.h>
#include "TestLibrary/TestLibrary.h"

#include "Test.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::Library> ptrTest(L"Test.Library");
	
	TEST(ptrTest->Hello() == L"Hello!");

	return true;
}
