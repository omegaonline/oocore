#include <OTL/OTL.h>
#include "./interfaces.h"
#include "TestLibrary/TestLibrary.h"

#include "Test.h"

BEGIN_PROCESS_OBJECT_MAP(L"CoreTests")
END_PROCESS_OBJECT_MAP()

static bool do_interface_tests(OTL::ObjectPtr<Test::ISimpleTest>& ptrSimpleTest)
{
	{
		TEST(ptrSimpleTest->BoolNot1(true) == false);
		TEST(ptrSimpleTest->BoolNot1(false) == true);

		Omega::bool_t r;
		ptrSimpleTest->BoolNot2(true,r);
		TEST(r == false);
		ptrSimpleTest->BoolNot2(false,r);
		TEST(r == true);

		ptrSimpleTest->BoolNot3(true,r);
		TEST(r == false);
		ptrSimpleTest->BoolNot3(false,r);
		TEST(r == true);

		r = true;
		ptrSimpleTest->BoolNot4(r);
		TEST(r == false);
		r = false;
		ptrSimpleTest->BoolNot4(r);
		TEST(r == true);
	}

	{
		TEST(ptrSimpleTest->ByteInc1(23) == 24);
		Omega::byte_t r;
		ptrSimpleTest->ByteInc2(24,r);
		TEST(r == 25);
		ptrSimpleTest->ByteInc3(255,r);
		TEST(r == 0);
		r = 24;
		ptrSimpleTest->ByteInc4(r);
		TEST(r == 25);
	}

	{
		TEST(ptrSimpleTest->Int16Inc1(1) == 2);
		Omega::int16_t r;
		ptrSimpleTest->Int16Inc2(-1,r);
		TEST(r == 0);
		ptrSimpleTest->Int16Inc3(0,r);
		TEST(r == 1);
		r = 32767;
		ptrSimpleTest->Int16Inc4(r);
		TEST(r == -32768);
	}

	{
		TEST(ptrSimpleTest->Float4Mul31(23.0f) == 23.0f * 3);
		Omega::float4_t r;
		ptrSimpleTest->Float4Mul32(24.0f,r);
		TEST(r == 24.0f * 3);
		ptrSimpleTest->Float4Mul33(25.5f,r);
		TEST(r == 25.5f * 3);
		r = 26.8f;
		ptrSimpleTest->Float4Mul34(r);
		TEST(r == 26.8f * 3);
	}

	{
		TEST(ptrSimpleTest->Float8Mul31(23.0) == 23.0 * 3);
		Omega::float8_t r;
		ptrSimpleTest->Float8Mul32(24.0,r);
		TEST(r == 24.0 * 3);
		ptrSimpleTest->Float8Mul33(25.5,r);
		TEST(r == 25.5 * 3);
		r = 26.8;
		ptrSimpleTest->Float8Mul34(r);
		TEST(r == 26.8 * 3);
	}

	TEST(ptrSimpleTest->Hello() == L"Hello!");

	/*try
	{
		ptrSimpleTest->Throw(EACCES);
	}
	catch (Omega::ISystemException* pE)
	{
		TEST(pE->ErrorCode() == EACCES);
		pE->Release();
	}*/

	// This is a test for channel closing
	/*try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}*/

	return true;
}

static bool do_library_test(const wchar_t* pszLibName, const wchar_t* pszObject, const wchar_t* pszEndpoint)
{
	if (!pszEndpoint)
	{
		int err = system((Omega::string_t(L"OORegister -i -s ") + pszLibName).ToUTF8().c_str());
		if (err != 0)
			return false;
	}

	OTL::ObjectPtr<Test::ISimpleTest> ptrSimpleTest;

	if (!pszEndpoint)
	{
		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
	
		Omega::string_t strXML =
			L"<?xml version=\"1.0\" ?>"
			L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				L"<key name=\"\\Objects\">"
					L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
						L"<value name=\"CurrentVersion\">%OBJECT%</value>"
					L"</key>"
				L"</key>"
			L"</root>";

		Omega::string_t strSubsts = L"OBJECT=";
		strSubsts += pszObject;

		Omega::Registry::AddXML(strXML,true,strSubsts);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
		
		Omega::Registry::AddXML(strXML,false);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);

		strXML =
			L"<?xml version=\"1.0\" ?>"
			L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				L"<key name=\"\\Local User\\Objects\">"
					L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
						L"<value name=\"CurrentVersion\">%OBJECT%</value>"
					L"</key>"
				L"</key>"
			L"</root>";

		Omega::Registry::AddXML(strXML,true,strSubsts);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
		
		Omega::Registry::AddXML(strXML,false);
	}

	ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(ptrSimpleTest);
	
	//TEST(system((Omega::string_t(L"OORegister -u -s ") + pszLibName).ToUTF8().c_str()) == 0);
	
	return true;
}

static bool do_process_test(const wchar_t* pszModulePath, const wchar_t* pszObject, const wchar_t* pszEndpoint)
{
	if (!pszEndpoint)
	{
		int err = system((Omega::string_t(L"TestProcess -i MODULE_PATH=") + pszModulePath).ToUTF8().c_str());
		if (err != 0)
			return false;
	}

	OTL::ObjectPtr<Test::ISimpleTest> ptrSimpleTest;

	if (!pszEndpoint)
	{
		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
		
		Omega::string_t strXML =
			L"<?xml version=\"1.0\" ?>"
			L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				L"<key name=\"\\Objects\">"
					L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
						L"<value name=\"CurrentVersion\">%OBJECT%</value>"
					L"</key>"
				L"</key>"
			L"</root>";

		Omega::string_t strSubsts = L"OBJECT=";
		strSubsts += pszObject;

		Omega::Registry::AddXML(strXML,true,strSubsts);
		
		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
		
		Omega::Registry::AddXML(strXML,false);
		
		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);

		strXML =
			L"<?xml version=\"1.0\" ?>"
			L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				L"<key name=\"\\Local User\\Objects\">"
					L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
						L"<value name=\"CurrentVersion\">%OBJECT%</value>"
					L"</key>"
				L"</key>"
			L"</root>";

		Omega::Registry::AddXML(strXML,true,strSubsts);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
		
		Omega::Registry::AddXML(strXML,false);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);

		OTL::ObjectPtr<Omega::Registry::IKey> ptrReg(L"\\Applications\\TestProcess");
		ptrReg->SetIntegerValue(L"Public",0);

		ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(ptrSimpleTest);
	
		ptrReg->SetIntegerValue(L"Public",1);
	}

	ptrSimpleTest = OTL::ObjectPtr<Test::ISimpleTest>(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(ptrSimpleTest);

	//TEST(system((Omega::string_t(L"TestProcess -u MODULE_PATH=") + pszModulePath).ToUTF8().c_str()) == 0);
	
	return true;
}

static bool interface_tests_i(const wchar_t* host)
{
#if defined(OMEGA_WIN32)
	do_library_test(L"TestLibrary_msvc",L"Test.Library.msvc",host);

	try
	{
		do_library_test(L"TestLibrary_mingw",L"Test.Library.mingw",host);
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		pE->Release();
	}
#else
	do_library_test(L"TestLibrary",L"Test.Library",host);
#endif

	do_process_test(L"TestProcess",L"Test.Process",host);

	return true;
}

bool interface_tests()
{
	return interface_tests_i(0);
}

bool interface_tests2()
{
	//return interface_tests_i(L"http://TSS04:8901");
	return interface_tests_i(L"http://localhost:8901");
}
