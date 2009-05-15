#include <OTL/OTL.h>
#include <OOCore/Remoting.h>
#include "interfaces.h"

namespace Omega {
namespace TestSuite
{
	extern "C" const Omega::guid_t OID_TestLibrary;
	extern "C" const Omega::guid_t OID_TestProcess;
} }

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestLibrary, "{16C07AEA-242F-48f5-A10E-1DCA3FADB9A6}");
OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestProcess, "{4BC2E65B-CEE0-40c6-90F2-39C7C306FC69}" );

#include "Test.h"

#if defined(_WIN32)
#define OOREGISTER L"OORegister"
#else
#define OOREGISTER L"./ooregister"
#endif

static bool do_interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>& ptrSimpleTest)
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

	try
	{
		ptrSimpleTest->Throw(2);
	}
	catch (Omega::ISystemException* pE)
	{
		TEST(pE->GetErrorCode() == 2);
		pE->Release();
	}

	return true;
}

static bool do_local_library_test(const wchar_t* pszLibName)
{
	// Register the library
	if (system((Omega::string_t(OOREGISTER L" -i -s ") + pszLibName).ToUTF8().c_str()) != 0)
    	return true;

	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(Omega::TestSuite::OID_TestLibrary,Omega::Activation::InProcess);
	do_interface_tests(ptrSimpleTest);

	// Now check for activation rules
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary,Omega::Activation::OutOfProcess);
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test for local activation
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary.ToString());
	do_interface_tests(ptrSimpleTest);

	// Test for local activation
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library");
	do_interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library@local");
	do_interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary.ToString() + L"@local");
	do_interface_tests(ptrSimpleTest);

	// Test redirecting the registration
	Omega::string_t strXML =
		L"<?xml version=\"1.0\" ?>"
		L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
			L"<key name=\"\\All Users\\Objects\">"
				L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
					L"<value name=\"CurrentVersion\">%OBJECT%</value>"
				L"</key>"
			L"</key>"
		L"</root>";

	Omega::string_t strSubsts = L"OBJECT=";
	strSubsts += L"Test.Library";

	Omega::Registry::AddXML(strXML,true,strSubsts);

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest");
	do_interface_tests(ptrSimpleTest);

	// Test it has gone
	Omega::Registry::AddXML(strXML,false);
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Try overloading the local only
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

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest@local");
	do_interface_tests(ptrSimpleTest);

	// Test it has gone
	Omega::Registry::AddXML(strXML,false);
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test unregistering
	TEST(system((Omega::string_t(OOREGISTER L" -u -s ") + pszLibName).ToUTF8().c_str()) == 0);

	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary);
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	return true;
}

static bool do_local_process_test(const wchar_t* pszModulePath)
{
	TEST(system((Omega::string_t(pszModulePath) + L" -i MODULE_PATH=" + pszModulePath).ToUTF8().c_str()) == 0);

	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestProcess,Omega::Activation::OutOfProcess);
	
	do_interface_tests(ptrSimpleTest);

	// Now check for activation rules
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestProcess,Omega::Activation::InProcess);
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Process");
	do_interface_tests(ptrSimpleTest);

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}

	TEST(system((Omega::string_t(pszModulePath) +  L" -u MODULE_PATH=" + pszModulePath).ToUTF8().c_str()) == 0);

	// Check its gone
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Process");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	return true;
}

bool interface_dll_tests()
{
#if defined(_WIN32)
	do_local_library_test(L"TestLibrary_msvc");
	do_local_library_test(L"TestLibrary_mingw");
#else
	do_local_library_test(L"./libTestLibrary.so");
#endif

	return true;
}

bool interface_tests()
{
#if defined(_WIN32)
	do_local_process_test(L"TestProcess");
#else
    do_local_process_test(L"./testprocess");
#endif

	return true;
}

static bool do_library_test(const wchar_t* pszLibName, const wchar_t* pszEndpoint)
{
	// Register the library ready for local loopback stuff
	system((Omega::string_t(OOREGISTER L" -i -s ") + pszLibName).ToUTF8().c_str());

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(L"Test.Library@" + Omega::string_t(pszEndpoint));
	do_interface_tests(ptrSimpleTest);

	return true;
}

static bool do_process_test(const wchar_t* pszModulePath, const wchar_t* pszEndpoint)
{
	system((Omega::string_t(pszModulePath) + L" -i MODULE_PATH=" + pszModulePath).ToUTF8().c_str());

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(L"Test.Process@" + Omega::string_t(pszEndpoint));
	do_interface_tests(ptrSimpleTest);

	return true;
}

static bool interface_tests_i(const wchar_t* pszHost)
{
#if defined(_WIN32)
	int c = 0;
	try
	{
		do_library_test(L"TestLibrary_msvc",pszHost);
		++c;
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	try
	{
		do_library_test(L"TestLibrary_mingw",pszHost);
		++c;
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	// One must pass...
	TEST(c != 0);
#else
	do_library_test(L"TestLibrary",pszHost);
#endif

	do_process_test(L"TestProcess",pszHost);

	return true;
}

bool interface_tests2()
{
	//return interface_tests_i(L"http://TSS04:8901/");
	return interface_tests_i(L"http://localhost:8901");
}
