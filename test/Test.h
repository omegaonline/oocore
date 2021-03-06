#ifndef TEST_H_INCLUDED_
#define TEST_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine);
int test_summary();
void add_success();
void add_failure(const char* pszText);
void output_exception(Omega::IException* pE);

typedef bool (*pfnTest)();
bool run_test(pfnTest t, const char* pszName);

#define RUN_TEST(test)      run_test(test,#test)

#define TEST(expr) \
	if (!(expr)) \
		return print_result(#expr,__FILE__,__LINE__); \
	else \
		add_success()

#define TEST_FAIL(expr) \
	return print_result(#expr,__FILE__,__LINE__); \
	 
#define TEST_VOID(expr) \
	(expr); add_success();

void output(const char* sz, ...);

#endif // TEST_H_INCLUDED_
