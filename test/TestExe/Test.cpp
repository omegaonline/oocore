#include <OOCore/OOCore.h>
#include <stdio.h>
#include "Test.h"

static unsigned long exception_count = 0;
static unsigned long pass_count = 0;
static unsigned long fail_count = 0;

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine)
{
	add_failure();
	printf("[Failed]\n\nAssertion '%s' failed at %s:%u\n",pszExpr,pszSrc,nLine);
	return false;
}

void add_success()
{
	++pass_count;
}

void add_failure()
{
	++pass_count;
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

void run_test(pfnTest t, const char* pszName)
{
	printf("Running %-40s",pszName);

	try
	{
		if ((*t)())
			printf("[Ok]\n");
	}
	catch (Omega::IException* pE)
	{
		++exception_count;
		printf("[Unhandled Omega::IException]\n\n%s\n%s\n",(const char*)pE->Description(),(const char*)pE->Source());
		pE->Release();
	}
	catch (std::exception& e)
	{
		++exception_count;
		printf("[Unhandled std::exception]\n\n%s\n",e.what());
	}
	catch (...)
	{
		++exception_count;
		printf("[Unhandled exception!]\n");
	}
}
