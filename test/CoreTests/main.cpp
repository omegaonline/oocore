#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <OOCore/OOCore.h>
#include "Test.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

////////////////////////////////////////////////////////////
// List the test entry points here rather than using header files...
// cos I'm lazy ;)

bool string_tests();
bool guid_tests();
bool init_tests();
bool exception_tests();
bool otl_tests();
bool registry_tests();
bool registry_tests_2();
bool interface_tests();
bool net_tests();
bool interface_tests2();

int main(int /*argc*/, char* /*argv*/[])
{
	printf("OOCore version info:\n%ls\n\n",Omega::System::GetVersion().c_str());
	fflush(stdout);

	if (RUN_TEST(init_tests))
	{
		RUN_TEST(string_tests);
		RUN_TEST(guid_tests);
		RUN_TEST(exception_tests);
		RUN_TEST(otl_tests);
		RUN_TEST(registry_tests);
		RUN_TEST(registry_tests_2);
		RUN_TEST(interface_tests);
		//RUN_TEST(net_tests);
		//RUN_TEST(interface_tests2);
	}

	return test_summary();
}

/////////////////////////////////////////////////////////////
// The following are the functions that actually do the tests

static unsigned long exception_count = 0;
static unsigned long pass_count = 0;
static unsigned long fail_count = 0;

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine)
{
	add_failure(Omega::string_t::Format(L"Assertion '%hs' failed at %hs:%u\n",pszExpr,pszSrc,nLine).c_str());
	return false;
}

void add_success()
{
	++pass_count;
}

void add_failure(const wchar_t* pszText)
{
	printf("[Failed]\n\n%ls",pszText);
	fflush(stdout);
	++fail_count;
}

int test_summary()
{
	if (fail_count || exception_count)
	{
		printf("\n%lu tests failed, %lu tests passed.\n",fail_count + exception_count,pass_count);
		return -1;
	}
	else
	{
		printf("\nAll (%lu) tests passed.\n",pass_count);
		return 0;
	}
}

static void recurse_printf_exception(Omega::IException* pE)
{
	Omega::IException* pCause = pE->Cause();
	if (pCause)
	{
		printf("Cause:\t%ls\nSource:\t%ls\n",pCause->Description().c_str(),pCause->Source().c_str());
		recurse_printf_exception(pCause);
		pCause->Release();
	}
}

void printf_exception(Omega::IException* pE)
{
	printf("Desc:\t%ls\nSource:\t%ls\n",pE->Description().c_str(),pE->Source().c_str());
	recurse_printf_exception(pE);
}

bool run_test(pfnTest t, const char* pszName)
{
	printf("Running %-40s",pszName);

	try
	{
		if ((*t)())
		{
			printf("[Ok]\n");
			fflush(stdout);
			return true;
		}
	}
	catch (Omega::IException* pE)
	{
		++exception_count;
		printf("[Unhandled Omega::IException]\n\n");
		printf_exception(pE);
		pE->Release();
	}
	catch (std::exception& e)
	{
		++exception_count;
		printf("[Unhandled std::exception]\n\nWhat:\t%s\n",e.what());
	}
	catch (...)
	{
		++exception_count;
		printf("[Unhandled C++ exception!]\n");
	}

    fflush(stdout);
	return false;
}

#if !defined(WIN32)
pid_t GetCurrentProcessId()
{
    return getpid();
}
#endif
