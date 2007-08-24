#include <OTL/OTL.h>
#include "./interfaces.h"
#include "TestLibrary/TestLibrary.h"

#include "Test.h"

bool interface_tests()
{
	OTL::ObjectPtr<Test::Iface> ptrTestLib(L"Test.Library");
	
	TEST(ptrTestLib->Hello() == L"Hello!");

	return true;
}
