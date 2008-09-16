//////////////////////////////////////////////
// Set up the export macros for TEST

#if defined(TEST_BUILD_LIBRARY)
#define TEST_DECLARE_OID(n) OMEGA_EXPORT_OID(n)
#else
#define TEST_DECLARE_OID(n) OMEGA_IMPORT_OID(n)
#endif

namespace Omega
{
	namespace TestSuite
	{
		TEST_DECLARE_OID(OID_TestLibrary)
	}
}