#include "../include/Omega/Omega.h"
#include "Test.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(_MSC_VER)
// Shutup VS leak
extern "C" int _setenvp() { return 0; }
#endif

////////////////////////////////////////////////////////////
// List the test entry points here rather than using header files...
// cos I'm lazy ;)

bool init_tests();
bool string_tests();
bool string_tests_format();
bool guid_tests();
bool any_tests();
bool exception_tests();
bool otl_tests();
bool registry_tests();
bool registry_tests_2();
bool interface_process_tests();
bool interface_dll_tests();
bool compartment_dll_tests();
bool compartment_process_tests();
bool net_tests();
bool interface_tests2();

int main(int /*argc*/, char* /*argv*/[])
{
	output("OOCore version: %s\n",OOCore::GetVersion());

	output("\nRunning general tests\n\n");

	RUN_TEST(string_tests);
	RUN_TEST(string_tests_format);
	RUN_TEST(guid_tests);
	RUN_TEST(any_tests);
	RUN_TEST(exception_tests);
	RUN_TEST(otl_tests);

	output("\nRunning %-40s","server tests");
	if (init_tests())
	{
		output("\n");

		RUN_TEST(registry_tests);
		RUN_TEST(interface_dll_tests);
		RUN_TEST(interface_process_tests);
		RUN_TEST(compartment_dll_tests);
		RUN_TEST(compartment_process_tests);

		//RUN_TEST(net_tests);
		//RUN_TEST(interface_tests2);

		Omega::Uninitialize();
	}

	return test_summary();
}

/////////////////////////////////////////////////////////////
// The following are the functions that actually do the tests

#if defined(_WIN32)

void output(const char* sz, ...)
{
	va_list argptr;
	va_start(argptr, sz);

	char szBuf[4096] = {0};
	vsnprintf(szBuf,sizeof(szBuf),sz,argptr);

	fputs(szBuf,stdout);

#if defined(_MSC_VER)
	OutputDebugStringA(szBuf);
#endif

	va_end(argptr);

	fflush(stdout);
}
#else
void output(const char* sz, ...)
{
	va_list argptr;
	va_start(argptr, sz);

	vprintf(sz,argptr);

	va_end(argptr);

	fflush(stdout);
}
#endif

static unsigned long exception_count = 0;
static unsigned long pass_count = 0;
static unsigned long fail_count = 0;

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine)
{
	Omega::string_t err(Omega::string_t("Assertion '{0}' failed at {1}:{2}\n") % pszExpr % pszSrc % nLine);
	add_failure(err.c_str());
	return false;
}

void add_success()
{
	++pass_count;
}

void add_failure(const char* pszText)
{
	output("[Failed]\n%ls",pszText);
	++fail_count;
}

int test_summary()
{
	if (fail_count || exception_count)
	{
		output("\n%lu tests failed, %lu tests passed.\n\n",fail_count + exception_count,pass_count);
		return EXIT_FAILURE;
	}
	else
	{
		output("\nAll (%lu) tests passed.\n\n",pass_count);
		return EXIT_SUCCESS;
	}
}

static void recurse_output_exception(Omega::IException* pE)
{
	Omega::IException* pCause = pE->GetCause();
	if (pCause)
	{
		try
		{
			output("Cause:\t%s\n",pCause->GetDescription().c_str());
			recurse_output_exception(pCause);
		}
		catch (Omega::IException* pE2)
		{
			output_exception(pE2);
			pE2->Release();
		}
		catch (...)
		{
			output("[C++ exception!]\n");
		}
		pCause->Release();
	}
}

void output_exception(Omega::IException* pE)
{
	output("Desc:\t%s\n",pE->GetDescription().c_str());

	Omega::IInternalException* pInt = static_cast<Omega::IInternalException*>(pE->QueryInterface(OMEGA_GUIDOF(Omega::IInternalException)));
	if (pInt)
	{
		output("Src:\t%s\n",pInt->GetSource().c_str());
		pInt->Release();
	}

	recurse_output_exception(pE);
}

bool run_test(pfnTest t, const char* pszName)
{
	output("Running %-40s",pszName);

	try
	{
		if ((*t)())
		{
			output("[Ok]\n");
			return true;
		}
	}
	catch (Omega::IException* pE)
	{
		++exception_count;
		output("[Omega::IException]\n");
		output_exception(pE);
		pE->Release();
	}
	catch (...)
	{
		++exception_count;
		output("[C++ exception!]\n");
	}

	return false;
}
