#ifndef TEST_H_INCLUDED_
#define TEST_H_INCLUDED_

bool print_result(const char* pszExpr, const char* pszSrc, unsigned int nLine);
int test_summary();
void add_success();

typedef bool (*pfnTest)();
void run_test(pfnTest t, const char* pszName);

#define RUN_TEST(test)	run_test(test,#test)

#define TEST(expr) \
	if (!(expr)) \
	{ \
		print_result(#expr,__FILE__,__LINE__); \
		return false; \
	} \
	else \
		add_success(); \

#endif TEST_H_INCLUDED_