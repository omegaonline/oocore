#include <OTL/OTL.h>
//#include "Test.h"

//#include "TestDll/TestDll.h"

namespace Test
{
	interface ExeTest : public Omega::IObject
	{
		virtual Omega::string_t Hello() = 0;
	};
}

OMEGA_DEFINE_INTERFACE
(
	Test, ExeTest, "{8488359E-C953-4e99-B7E5-ECA150C92F49}",

	// Methods
	OMEGA_METHOD(Omega::string_t,Hello,0,())
)


namespace 
{
	class TestDllImpl :
		public OTL::ObjectBase,
		public Test::ExeTest
	{
	public:
		TestDllImpl()
		{
			wprintf(L"%s\n",(const wchar_t*)Omega::System::MetaInfo::lookup_iid(OMEGA_UUIDOF(Test::ExeTest)));
		}
		Omega::string_t Hello();
		
		BEGIN_INTERFACE_MAP(TestDllImpl)
			INTERFACE_ENTRY(Test::ExeTest)
		END_INTERFACE_MAP()
	};
};

Omega::string_t 
TestDllImpl::Hello()
{
	return L"Hello!";
}

// OTL requires this...
class TestExe : public OTL::ProcessModule
{
public:
};

BEGIN_PROCESS_OBJECT_MAP(TestExe)
END_PROCESS_OBJECT_MAP()

bool interface_tests()
{
	OTL::ObjectImpl<TestDllImpl>* imp = OTL::ObjectImpl<TestDllImpl>::CreateInstance();

	const Omega::System::MetaInfo::qi_rtti* p = 0; 
/*	Omega::System::MetaInfo::DllTest43 q=0; 
	Omega::System::MetaInfo::get_qi_rtti((int*)0,&p,q,OMEGA_UUIDOF(Test::DllTest));	

	wprintf(L"%s\n",(const wchar_t*)Omega::System::MetaInfo::lookup_iid(OMEGA_UUIDOF(Test::DllTest)));

	Omega::guid_t oid = Omega::Activation::NameToOid(L"Test.Dll");
	Omega::IObject* pObj = Omega::CreateInstance(oid,Omega::Activation::Any,0,OMEGA_UUIDOF(Omega::IObject));

	Test::DllTest* pT = (Test::DllTest*)pObj->QueryInterface(OMEGA_UUIDOF(Test::DllTest));

	//OTL::ObjectPtr<Test::DllTest> ptrTest();
	//wprintf(ptrTest->Hello());

	OTL::ObjectPtr<Test::DllTest> ptrTest2(L"Test.Dll");*/


	return false;
}
