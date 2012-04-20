#include "../include/Omega/Remoting.h"
#include "../include/OTL/Registry.h"
#include "interfaces.h"
#include "aggregator.h"

#include <limits.h>

void normalise_path(Omega::string_t& strPath);

namespace Omega
{
	namespace TestSuite
	{
		extern "C" const Omega::guid_t OID_TestLibrary;
		extern "C" const Omega::guid_t OID_TestProcess;
	}
}

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestLibrary, "{16C07AEA-242F-48F5-A10E-1DCA3FADB9A6}");
OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestProcess, "{4BC2E65B-CEE0-40C6-90F2-39C7C306FC69}");

#include "Test.h"

bool register_library(const Omega::string_t& strLibName, bool& bSkipped)
{
	bSkipped = false;

#if defined(_WIN32)
	if (access(strLibName.c_str(),0) != 0)
	{
		output("[Missing]\n");
		bSkipped = true;
		return true;
	}
#endif

	Omega::string_t strOid = Omega::TestSuite::OID_TestLibrary.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey->OpenKey("Test.Library",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("OID",strOid);
	ptrSubKey = ptrKey->OpenKey("OIDs/" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("Library",strLibName);

	return true;
}

bool unregister_library()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects");

	if (ptrKey->IsKey("Test.Library"))
		ptrKey->DeleteSubKey("Test.Library");

	Omega::string_t strOid = Omega::TestSuite::OID_TestLibrary.ToString();

	if (ptrKey->IsKey("OIDs/" + strOid))
		ptrKey->DeleteSubKey("OIDs/" + strOid);

	return true;
}

bool register_process(const Omega::string_t& strExeName, bool& bSkipped)
{
	bSkipped = false;

#if defined(_WIN32)
	if (access(strExeName.c_str(),0) != 0)
	{
		output("[Missing]\n");
		bSkipped = true;
		return true;
	}
#endif

	Omega::string_t strOid = Omega::TestSuite::OID_TestProcess.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey->OpenKey("Test.Process",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("OID",strOid);
	ptrSubKey = ptrKey->OpenKey("OIDs/" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("Application","CoreTests.TestProcess");

	ptrKey = OTL::ObjectPtr<Omega::Registry::IKey>("Local User/Applications",Omega::Registry::IKey::OpenCreate);
	ptrSubKey = ptrKey->OpenKey("CoreTests.TestProcess/Activation",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("Path",strExeName);

	return true;
}

bool unregister_process()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects");

	if (ptrKey->IsKey("Test.Process"))
		ptrKey->DeleteSubKey("Test.Process");

	Omega::string_t strOid = Omega::TestSuite::OID_TestProcess.ToString();

	if (ptrKey->IsKey("OIDs/" + strOid))
		ptrKey->DeleteSubKey("OIDs/" + strOid);

	ptrKey = OTL::ObjectPtr<Omega::Registry::IKey>("Local User/Applications",Omega::Registry::IKey::OpenCreate);
	if (ptrKey->IsKey("CoreTests.TestProcess"))
		ptrKey->DeleteSubKey("CoreTests.TestProcess");

	return true;
}

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest)
{
	{
		TEST(ptrSimpleTest->BoolNot1(true) == false);
		TEST(ptrSimpleTest->BoolNot1(false) == true);

		Omega::bool_t r = true;
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

	TEST(ptrSimpleTest->Hello() == "Hello!");

	std::list<Omega::uint32_t> list1;
	list1.push_back(0);

	TEST(ptrSimpleTest->ListUInt32_Count(list1) == 1);

	std::list<Omega::uint32_t> list = ptrSimpleTest->ListUInt32_Fill();

	TEST(list.size() == 2);
	TEST(list.front() == 1);
	TEST(list.back() == 2);

	try
	{
		ptrSimpleTest->Throw(2);
	}
	catch (Omega::ISystemException* pE)
	{
		TEST(pE->GetErrorCode() == 2);
		pE->Release();
	}

	// Check the QI rules
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = ptrSimpleTest.QueryInterface<Omega::TestSuite::ISimpleTest2>();
	TEST(ptrSimpleTest2);

	OTL::ObjectPtr<Omega::IObject> ptrO1 = ptrSimpleTest->QueryInterface(OMEGA_GUIDOF(Omega::IObject));
	OTL::ObjectPtr<Omega::IObject> ptrO2 = ptrSimpleTest2->QueryInterface(OMEGA_GUIDOF(Omega::IObject));

	TEST(static_cast<Omega::IObject*>(ptrO1) == static_cast<Omega::IObject*>(ptrO2));

	ptrO1.Release();
	ptrO2.Release();

	// Test the type info
	OTL::ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrPOI = ptrSimpleTest.QueryInterface<Omega::TypeInfo::IProvideObjectInfo>();
	TEST(ptrPOI);

	// Try to get the first interface
	Omega::TypeInfo::IProvideObjectInfo::iid_list_t interfaces = ptrPOI->EnumInterfaces();
	TEST(!interfaces.empty());
	TEST(interfaces.front() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));

	OTL::ObjectPtr<Omega::TypeInfo::IInterfaceInfo> ptrII = Omega::TypeInfo::GetInterfaceInfo(interfaces.front(),ptrSimpleTest);
	TEST(ptrII);

	TEST(ptrII->GetName() == "Omega::TestSuite::ISimpleTest");
	TEST(ptrII->GetIID() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));
	TEST(ptrII->GetMethodCount() == 25+3);

	return true;
}

static bool do_local_library_test(const Omega::string_t& strLibName, bool& bSkipped)
{
	// Register the library
	output("  %-45s ",strLibName.c_str());

	// Register the library
	TEST(register_library(strLibName,bSkipped));
	if (bSkipped)
		return true;

	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(Omega::TestSuite::OID_TestLibrary,Omega::Activation::Library);
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = ptrSimpleTest.QueryInterface<Omega::TestSuite::ISimpleTest2>();
	TEST(ptrSimpleTest2->WhereAmI() == "Inner");

	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();

	// Test aggregation
	Aggregator* pAgg = new Aggregator();

	pAgg->SetInner(Omega::CreateInstance(Omega::TestSuite::OID_TestLibrary,Omega::Activation::Library,pAgg,OMEGA_GUIDOF(Omega::IObject)));

	ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg);
	TEST(ptrSimpleTest2->WhereAmI() == "Outer");

	ptrSimpleTest = ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>();
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	ptrSimpleTest2.Release();
	ptrSimpleTest.Release();

	// Test for local activation
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary.ToString());
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("Test.Library");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("Test.Library@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary.ToString() + "@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test redirecting the registration
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey->OpenKey("MyLittleTest",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("CurrentVersion","Test.Library");

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("MyLittleTest@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test it has gone
	ptrKey->DeleteSubKey("MyLittleTest");
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("MyLittleTest");
	}
	catch (Omega::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test unregistering
	TEST(unregister_library());

	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("Test.Library");
	}
	catch (Omega::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary);
	}
	catch (Omega::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	return true;
}

static bool do_local_process_test(const Omega::string_t& strModulePath, bool& bSkipped)
{
	output("  %-45s ",strModulePath.c_str());

	// Register the exe
	TEST(register_process(strModulePath,bSkipped));
	if (bSkipped)
		return true;

	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(Omega::TestSuite::OID_TestProcess,Omega::Activation::Process);
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = ptrSimpleTest.QueryInterface<Omega::TestSuite::ISimpleTest2>();
	TEST(ptrSimpleTest2->WhereAmI() == "Inner");

	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();

	// Test aggregation
	Aggregator* pAgg = new Aggregator();

	pAgg->SetInner(Omega::CreateInstance(Omega::TestSuite::OID_TestProcess,Omega::Activation::Process,pAgg,OMEGA_GUIDOF(Omega::IObject)));

	ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg);
	TEST(ptrSimpleTest2->WhereAmI() == "Outer");

	ptrSimpleTest = ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>();
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	ptrSimpleTest2.Release();

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestProcess,Omega::Activation::Process);

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::Remoting::IChannelClosedException* pE)
	{
		add_success();
		pE->Release();
	}

	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("Test.Process");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::Remoting::IChannelClosedException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test unregistering
	TEST(unregister_process());

	// Check its gone
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>("Test.Process");
	}
	catch (Omega::INotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	return true;
}

const char** get_dlls()
{
	static const char* dlls[] =
	{
#if defined(_MSC_VER)
		"TestLibrary_msvc.dll",
		"..\\Release\\TestLibrary_msvc.dll",
		"..\\..\\..\\debug\\test\\TestLibrary\\.libs\\TestLibrary.dll",
#elif defined(_WIN32)
		OMEGA_STRINGIZE(TOP_SRC_DIR) "/bin/Win32/Release.TestLibrary_msvc.dll",
		OMEGA_STRINGIZE(TOP_SRC_DIR) "/bin/Win32/Debug/TestLibrary_msvc.dll",
		"TestLibrary/.libs/TestLibrary.dll",
#else
		"TestLibrary/testlibrary.la",
#endif
		0
	};
	return dlls;
}

Omega::string_t make_absolute(const char* sz)
{
#if defined(_WIN32)
	char szBuf[MAX_PATH] = {0};
	GetFullPathNameA(sz,MAX_PATH,szBuf,NULL);
	return Omega::string_t(szBuf);
#else
	char* ret = realpath(sz,NULL);
	if (!ret)
		return Omega::string_t(sz);

	Omega::string_t r(ret);

	free(ret);

	return r;
#endif
}

bool interface_dll_tests()
{
	output("\n");

	for (const char** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_local_library_test(make_absolute(*pszDlls),bSkipped);

		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

const char** get_exes()
{
	static const char* exes[] =
	{
#if defined(_MSC_VER)
		"TestProcess_msvc.exe",
		"..\\Release\\TestProcess_msvc.exe",
		"..\\..\\..\\debug\\test\\TestProcess\\.libs\\testprocess.exe",
#elif defined(_WIN32)
		OMEGA_STRINGIZE(TOP_SRC_DIR) "/bin/Win32/Release/TestProcess_msvc.exe",
		OMEGA_STRINGIZE(TOP_SRC_DIR) "/bin/Win32/Debug/TestProcess_msvc.exe",
		OMEGA_STRINGIZE(BUILD_DIR) "/TestProcess/testprocess.exe",
#else
		OMEGA_STRINGIZE(BUILD_DIR) "/TestProcess/testprocess",
#endif
		0
	};
	return exes;
}

bool interface_process_tests()
{
	output("\n");

	for (const char** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_local_process_test(make_absolute(*pszExes),bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	return true;
}

static bool do_library_test(const Omega::string_t& strLibName, const char* pszEndpoint, bool& bSkipped)
{
	// Register the library ready for local loopback stuff
	output("  %-45s ",strLibName.c_str());

	// Register the library
	TEST(register_library(strLibName,bSkipped));
	if (bSkipped)
		return true;

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest("Test.Library@" + Omega::string_t(pszEndpoint));
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	return true;
}

static bool do_process_test(const Omega::string_t& strModulePath, const char* pszEndpoint, bool& bSkipped)
{
	output("  %-45s ",strModulePath.c_str());

	// Register the exe
	TEST(register_process(strModulePath,bSkipped));
	if (bSkipped)
		return true;

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest("Test.Process@" + Omega::string_t(pszEndpoint));
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	TEST(unregister_process());

	return true;
}

static bool interface_tests_i(const char* pszHost)
{
	output("\n");

	for (const char** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_library_test(make_absolute(*pszDlls),pszHost,bSkipped);

		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	for (const char** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_process_test(make_absolute(*pszExes),pszHost,bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

bool interface_tests2()
{
	//return interface_tests_i("http://TSS04:8901/");
	return interface_tests_i("http://localhost:8901");
}
