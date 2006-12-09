#include "./Testdll.h"

#include <OTL/OTL.h>

namespace 
{
	class TestDllImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestDllImpl,&Test::OID_TestDll>,
		public Test::DllTest
	{
	public:
		Omega::string_t Hello();
		
		BEGIN_INTERFACE_MAP(TestDllImpl)
			INTERFACE_ENTRY(Test::DllTest)
		END_INTERFACE_MAP()
	};
};

Omega::string_t 
TestDllImpl::Hello()
{
	return "Hello!";
}

BEGIN_LIBRARY_OBJECT_MAP(TestDll)
	OBJECT_MAP_ENTRY(TestDllImpl)
END_LIBRARY_OBJECT_MAP()

