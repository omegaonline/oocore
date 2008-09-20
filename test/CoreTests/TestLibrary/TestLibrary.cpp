#include <OTL/OTL.h>

#include "../SimpleTest.h"
#include "./TestLibrary.h"

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestLibrary, "{16C07AEA-242F-48f5-A10E-1DCA3FADB9A6}");

class TestLibraryImpl :
	public OTL::ObjectBase,
	public OTL::AutoObjectFactory<TestLibraryImpl,&Omega::TestSuite::OID_TestLibrary>,
	public OTL::IProvideObjectInfoImpl<TestLibraryImpl>,
	public SimpleTestImpl
{
public:
	TestLibraryImpl()
	{ }

	void Abort();

	BEGIN_INTERFACE_MAP(TestLibraryImpl)
		INTERFACE_ENTRY(Omega::TestSuite::ISimpleTest)
		INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
	END_INTERFACE_MAP()
};


void TestLibraryImpl::Abort()
{
	// Do nothing
}

BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestLibraryImpl,L"Test.Library")
END_LIBRARY_OBJECT_MAP()
