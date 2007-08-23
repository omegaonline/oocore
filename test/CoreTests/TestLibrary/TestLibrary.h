//////////////////////////////////////////////
// Set up the export macros for TEST
#if defined(TEST_BUILD_LIBRARY)

#define TEST_DECLARE_OID(n) \
	OMEGA_EXPORT_OID(n)

#else

#define TEST_DECLARE_OID(n) \
	OMEGA_IMPORT_OID(n)

#endif

namespace Test
{
	interface Library : public Omega::IObject
	{
		virtual Omega::string_t Hello() = 0;
	};

	TEST_DECLARE_OID(OID_TestLibrary)
}

OMEGA_DEFINE_INTERFACE
(
	Test, Library, "{8488359E-C953-4e99-B7E5-ECA150C92F48}",

	// Methods
	OMEGA_METHOD(Omega::string_t,Hello,0,())
)
