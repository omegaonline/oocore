#include <OOCore/OOCore.h>

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

#include <stdio.h>
#include "Test.h"

bool string_tests();
bool guid_tests();

int main(int argc, char* argv[])
{
	printf("%s\n\n",(const char*)Omega::System::GetVersion());

	RUN_TEST(string_tests);
	RUN_TEST(guid_tests);

	return test_summary();
}
