#include <OOBase/Thread.h>
#include <OOBase/Singleton.h>
#include <OOBase/Queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Test.h"

struct Module
{
	int unused;
};

typedef OOBase::Singleton<OOBase::BoundedQueue<std::string>,Module > QUEUE;

static int producer_func(void* param)
{
	size_t id = (size_t)param;

	for (int i=0;i<1000;++i)
	{
		// Sleep a bit
		OOBase::sleep(OOBase::timeval_t(0,rand()));

		char szBuf[64];
		sprintf_s(szBuf,sizeof(szBuf),"[P%d]",id);
		output((std::string("Pushed ") + szBuf + "\n").c_str());
		QUEUE::instance()->push(szBuf);
	}

	return 0;
}

static int consumer_func(void* param)
{
	size_t id = (size_t)param;

	for (int i=0;i<1000;++i)
	{
		std::string val;
		QUEUE::instance()->pop(val);

		output("\t\t[C%d] Popped %s\n\n",id,val.c_str());
	}

	return 0;
}

namespace OOBase
{
	void CriticalFailure(const char* msg)
	{
		printf(msg);
		abort();
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	// Seed RNG
	srand((unsigned int)time(0));

	// Start some producer threads
	OOBase::Thread producers[25];
	for (size_t i=0;i<sizeof(producers)/sizeof(producers[0]);++i)
		producers[i].run(producer_func,(void*)i);

	// Start some consumer threads
	OOBase::Thread consumers[25];
	for (size_t i=0;i<sizeof(consumers)/sizeof(consumers[0]);++i)
		consumers[i].run(consumer_func,(void*)i);

	// Wait for everyone
	for (size_t i=0;i<sizeof(consumers)/sizeof(consumers[0]);++i)
		consumers[i].join();

	for (size_t i=0;i<sizeof(producers)/sizeof(producers[0]);++i)
		producers[i].join();

	return test_summary();
}

/////////////////////////////////////////////////////////////
// The following are the functions that actually do the tests

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
