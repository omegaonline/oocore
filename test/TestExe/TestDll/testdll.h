//////////////////////////////////////////////
// Set up the export macros for TEST
#if defined(TEST_BUILD_LIBRARY)

#define TEST_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define TEST_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define TEST_DECLARE_OID(n) \
	OMEGA_EXPORT_OID(n)

#else

#define TEST_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params)

#define TEST_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params)

#define TEST_DECLARE_OID(n) \
	OMEGA_IMPORT_OID(n)

#endif

#include <OOCore/OOCore.h>

namespace Test
{
	interface DllTest : public Omega::IObject
	{
	public:
		virtual Omega::string_t Hello() = 0;
	};

	TEST_DECLARE_OID(OID_TestDll)
}

OMEGA_EXPORT_INTERFACE
(
	Test, DllTest, "{8488359E-C953-4e99-B7E5-ECA150C92F48}",

	// Methods
	OMEGA_METHOD(Omega::string_t,Hello,0,())
)
