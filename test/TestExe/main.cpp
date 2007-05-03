#define OMEGA_GUID_LINK_HERE
#include <OOCore/OOCore.h>

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

#include <stdio.h>
#include "Test.h"

bool string_tests();

int main(int argc, char* argv[])
{
	RUN_TEST(string_tests);

	return test_summary();
}
