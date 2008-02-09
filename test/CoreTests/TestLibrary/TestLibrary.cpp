#include <OTL/OTL.h>

#include "../interfaces.h"
#include "./TestLibrary.h"

#if defined(OMEGA_WIN32)
#if defined(__GNUC__)
#define TEST_LIBRARY_NAME L"Test.Library.mingw"
OMEGA_DEFINE_OID(Test, OID_TestLibrary, "{DA41C02A-B8E3-4597-8541-480F48A2E3CC}" );
#else
#define TEST_LIBRARY_NAME L"Test.Library.msvc"
OMEGA_DEFINE_OID(Test, OID_TestLibrary, "{9BA414E7-16C9-4e89-9439-A428CA9BB4E0}" );
#endif
#else
#define TEST_LIBRARY_NAME L"Test.Library"
OMEGA_DEFINE_OID(Test, OID_TestLibrary, "{16C07AEA-242F-48f5-A10E-1DCA3FADB9A6}" );
#endif

class TestLibraryImpl :
	public OTL::ObjectBase,
	public OTL::AutoObjectFactory<TestLibraryImpl,&Test::OID_TestLibrary>,
	public Test::Iface
{
public:
	TestLibraryImpl()
	{ }

	Omega::string_t Hello();
	void Abort();

	BEGIN_INTERFACE_MAP(TestLibraryImpl)
		INTERFACE_ENTRY(Test::Iface)
	END_INTERFACE_MAP()
};

Omega::string_t TestLibraryImpl::Hello()
{
	return L"Hello!";
}

void TestLibraryImpl::Abort()
{
	// Do nothing
}

BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestLibraryImpl,TEST_LIBRARY_NAME)
END_LIBRARY_OBJECT_MAP()
