#include <OOCore/OOCore.h>
#include "Test.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

////////////////////////////////////////////////////////////
// List the test entry points here rather than using header files...
// cos I'm lazy ;)

bool init_standalone_tests();
bool init_server_tests(bool& bRun);
bool string_tests();
bool string_tests_format();
bool string_tests_utf8();
bool guid_tests();
bool exception_tests();
bool otl_tests();
bool registry_tests();
bool registry_tests_2();
bool interface_process_tests();
bool interface_dll_tests();
bool apartment_dll_tests();
bool net_tests();
bool interface_tests2();

static void tests(bool bStandalone)
{
	if (bStandalone)
	{
		RUN_TEST(string_tests);
		RUN_TEST(string_tests_format);
		RUN_TEST(string_tests_utf8);
		RUN_TEST(guid_tests);
		RUN_TEST(exception_tests);
		RUN_TEST(otl_tests);
	}

	RUN_TEST(registry_tests);
	RUN_TEST(registry_tests_2);
	RUN_TEST(interface_dll_tests);
	RUN_TEST(apartment_dll_tests);
	
	if (!bStandalone)
	{
		RUN_TEST(interface_process_tests);
	}

	return;

	RUN_TEST(net_tests);
	RUN_TEST(interface_tests2);
}

int main(int /*argc*/, char* /*argv*/[])
{
	output("OOCore version: %s\n",OOCore::GetVersion());

	output("\nRunning %-40s\n\n","standalone tests");
	if (init_standalone_tests())
	{
		tests(true);

		Omega::Uninitialize();
	}

	output("\nRunning %-40s","server tests");
	bool bRun = true;
	if (init_server_tests(bRun) && bRun)
	{
		output("\n\n");

		tests(false);

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
	OutputDebugStringA(szBuf);

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
	add_failure((L"Assertion '{0}' failed at {1}:{2}\n" % Omega::string_t(pszExpr,false) % Omega::string_t(pszSrc,false) % nLine).c_str());
	return false;
}

void add_success()
{
	++pass_count;
}

void add_failure(const wchar_t* pszText)
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
		output("Cause:\t%ls\nSource:\t%ls\n",pCause->GetDescription().c_str(),pCause->GetSource().c_str());
		recurse_output_exception(pCause);
		pCause->Release();
	}
}

void output_exception(Omega::IException* pE)
{
	output("Desc:\t%ls\nSource:\t%ls\n",pE->GetDescription().c_str(),pE->GetSource().c_str());
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
	catch (std::exception& e)
	{
		++exception_count;
		output("[std::exception]\nWhat:\t%s\n",e.what());
	}
	catch (...)
	{
		++exception_count;
		output("[C++ exception!]\n");
	}

    return false;
}
