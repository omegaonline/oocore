#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

// Link to the static lib version of ACE...
#define ACE_AS_STATIC_LIBS 1

#include <ace/OS_NS_stdio.h>
#include <ace/OS_NS_stdlib.h>

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
bool registry_tests_3();
bool interface_tests();

int ACE_TMAIN(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
	ACE_OS::fprintf(stdout,L"OOCore version info:\n%ls\n\n",Omega::System::GetVersion().c_str());

	//RUN_TEST(string_tests);
	//RUN_TEST(guid_tests);
	RUN_TEST(init_tests);
	//RUN_TEST(exception_tests);
	//RUN_TEST(otl_tests);
	//RUN_TEST(registry_tests);
	//RUN_TEST(registry_tests_2);
	//RUN_TEST(registry_tests_3);
	RUN_TEST(interface_tests);

	return test_summary();
}

#if defined(ACE_WIN32) && defined(__MINGW32__)
#include <shellapi.h>
int main(int argc, char* /*argv*/[])
{
	// MinGW doesn't understand wmain, so...
	wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(),&argc);

	ACE_Main m;
	return ace_os_wmain_i (m, argc, wargv);   /* what the user calls "main" */
}
#endif

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
	ACE_OS::fprintf(stdout,L"[Failed]\n\n%ls",pszText);
	++fail_count;
}

int test_summary()
{
	if (fail_count || exception_count)
	{
		ACE_OS::fprintf(stdout,"\n%lu tests failed, %lu tests passed.\n",fail_count + exception_count,pass_count);
		return -1;
	}
	else
	{
		ACE_OS::fprintf(stdout,"\nAll (%lu) tests passed.\n",pass_count);
		return 0;
	}
}

void run_test(pfnTest t, const char* pszName)
{
	ACE_OS::fprintf(stdout,"Running %-40s",pszName);

	try
	{
		if ((*t)())
			ACE_OS::fprintf(stdout,"[Ok]\n");
	}
	catch (Omega::IException* pE)
	{
		++exception_count;
		ACE_OS::fprintf(stdout,L"[Unhandled Omega::IException]\n\n%ls\n%ls\n",pE->Description().c_str(),pE->Source().c_str());
		pE->Release();
	}
	catch (std::exception& e)
	{
		++exception_count;
		ACE_OS::fprintf(stdout,"[Unhandled std::exception]\n\n%s\n",e.what());
	}
	catch (...)
	{
		++exception_count;
		ACE_OS::fprintf(stdout,"[Unhandled C++ exception!]\n");
	}
}

// This is here so I don't have to include ACE everywhere...
int test_system(const wchar_t* pszCommand)
{
	return ACE_OS::system(ACE_TEXT_WCHAR_TO_TCHAR(pszCommand));
}
