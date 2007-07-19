#include <OOCore/OOCore.h>
#include "Test.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

// List the test entry points here rather than using header files...
bool string_tests();
bool guid_tests();
bool core_tests();
bool exception_tests();
bool otl_tests();
bool registry_tests();
bool registry_tests_2();
bool interface_tests();

int main(int /*argc*/, char* /*argv*/[])
{
	printf("%s\n\n",(const char*)Omega::System::GetVersion());

	RUN_TEST(string_tests);
	RUN_TEST(guid_tests);
	RUN_TEST(core_tests);
	RUN_TEST(exception_tests);
	RUN_TEST(otl_tests);
	RUN_TEST(registry_tests);
	RUN_TEST(registry_tests_2);
	RUN_TEST(interface_tests);

	return test_summary();
}
