#include <OTL/OTL.h>

#include "./Testdll.h"

namespace 
{
	class TestDllImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestDllImpl,&Test::OID_TestDll>,
		public Test::DllTest
	{
	public:
		TestDllImpl()
		{
			wprintf(L"%s\n",(const wchar_t*)Omega::System::MetaInfo::lookup_iid(OMEGA_UUIDOF(Test::DllTest)));
		}
		Omega::string_t Hello();
		
		BEGIN_INTERFACE_MAP(TestDllImpl)
			INTERFACE_ENTRY(Test::DllTest)
		END_INTERFACE_MAP()
	};
};

Omega::string_t 
TestDllImpl::Hello()
{
	return L"Hello!";
}

BEGIN_LIBRARY_OBJECT_MAP(TestDll)
	OBJECT_MAP_ENTRY(TestDllImpl)
END_LIBRARY_OBJECT_MAP()

OMEGA_DEFINE_OID(Test, OID_TestDll, "{16C07AEA-242F-48F5-A10E-1DCA3FADB9A6}" );
