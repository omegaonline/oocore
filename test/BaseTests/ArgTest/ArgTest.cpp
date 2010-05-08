#include <OOBase/CmdArgs.h>

#include <stdio.h>
#include <stdlib.h>

#include "Test.h"

namespace OOBase
{
	void CriticalFailure(const char* msg)
	{
		printf(msg);
		abort();
	}
}

int main(int argc, char* argv[])
{
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("a",'a');
	cmd_args.add_option("b",'b');
	cmd_args.add_option("c",'c',"long-c",true);

	std::map<std::string,std::string> results;
	TEST(cmd_args.parse(argc,argv,results));

	for (std::map<std::string,std::string>::iterator i=results.begin(); i!=results.end(); ++i)
	{
		printf("%s = %s\n",i->first.c_str(),i->second.c_str());
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
	vsnprintf_s(szBuf,sizeof(szBuf),sz,argptr);

	printf(szBuf);
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
	output("[Failed]\n\nAssertion '%s' failed at %s:%u\n",pszExpr,pszSrc,nLine);
	return false;
}

void add_success()
{
	++pass_count;
}

void add_failure(const wchar_t* pszText)
{
	output("[Failed]\n\n%ls",pszText);
	++fail_count;
}

int test_summary()
{
	if (fail_count || exception_count)
	{
		output("\n%lu tests failed, %lu tests passed.\n",fail_count + exception_count,pass_count);
		return EXIT_FAILURE;
	}
	else
	{
		output("\nAll (%lu) tests passed.\n",pass_count);
		return EXIT_SUCCESS;
	}
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
	catch (std::exception& e)
	{
		++exception_count;
		output("[Unhandled std::exception]\n\nWhat:\t%s\n",e.what());
	}
	catch (...)
	{
		++exception_count;
		output("[Unhandled C++ exception!]\n");
	}

	return false;
}
