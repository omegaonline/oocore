#include <stdio.h>
#include "Test.h"

static unsigned long exception_count = 0;
static unsigned long pass_count = 0;
static unsigned long fail_count = 0;

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine)
{
	++fail_count;
	printf("[Failed]\nAssertion '%s' failed at %s:%lu\n",pszExpr,pszSrc,nLine);
	return false;
}

void add_success()
{
	++pass_count;
}

int test_summary()
{
	if (fail_count || exception_count)
	{
		if (exception_count)
			printf("\n%lu tests failed, %lu tests passed, %lu unhandled exceptions occurred.\n",fail_count,pass_count,exception_count);
		else
			printf("\n%lu tests failed, %lu tests passed.\n",fail_count,pass_count);
		return -1;
	}
	else
	{
		printf("\nAll (%lu) tests passed.\n",pass_count);
		return 0;
	}
}

void run_test(pfnTest t, const char* pszName)
{
	printf("Running test '%s'...\t",pszName);
	try
	{
		if ((*t)())
			printf("[Ok]\n");
	}
	catch (...)
	{
		++exception_count;
		printf("[Unhandled exception!]\n");
	}
}

/*
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		printf_exception("Omega::Initialize",pE);
		return -1;
	}

	try
	{
		OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
		OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");

		printf(ptrTest->Hello());
	}
	catch(Omega::IException* pE)
	{
		printf_exception("Test.Dll tests",pE);
		Omega::Uninitialize();
		return -1;
	}

	Omega::Uninitialize();
*/
