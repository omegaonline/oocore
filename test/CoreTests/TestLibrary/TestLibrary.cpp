#include <OTL/OTL.h>

#include "../interfaces.h"
#include "./TestLibrary.h"

namespace
{
	extern const wchar_t TestLibraryName[] = L"Test.Library";

	class TestLibraryImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestLibraryImpl,&Test::OID_TestLibrary,TestLibraryName>,
		public Test::Iface
	{
	public:
		TestLibraryImpl()
		{ }

		Omega::string_t Hello();

		BEGIN_INTERFACE_MAP(TestLibraryImpl)
			INTERFACE_ENTRY(Test::Iface)
		END_INTERFACE_MAP()
	};
};

Omega::string_t
TestLibraryImpl::Hello()
{
	return L"Hello!";
}

BEGIN_LIBRARY_OBJECT_MAP(TestLibrary)
	OBJECT_MAP_ENTRY(TestLibraryImpl)
END_LIBRARY_OBJECT_MAP()

OMEGA_DEFINE_OID(Test, OID_TestLibrary, "{16C07AEA-242F-48F5-A10E-1DCA3FADB9A6}" );
