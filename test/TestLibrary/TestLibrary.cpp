#include "../../include/Omega/Omega.h"
#include "../../include/Omega/TypeInfo.h"
#include "../../include/OTL/OTL.h"

#include "../SimpleTest.h"

namespace Omega
{
	namespace TestSuite
	{
		extern const Omega::guid_t OID_TestLibrary;
	}
}

const Omega::guid_t Omega::TestSuite::OID_TestLibrary("{16C07AEA-242F-48f5-A10E-1DCA3FADB9A6}");

class TestLibraryImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestLibraryImpl,&Omega::TestSuite::OID_TestLibrary>,
		public OTL::IProvideObjectInfoImpl<TestLibraryImpl>,
		public SimpleTestImpl
{
public:
	TestLibraryImpl()
	{
	}

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
	OBJECT_MAP_ENTRY(TestLibraryImpl)
END_LIBRARY_OBJECT_MAP()
