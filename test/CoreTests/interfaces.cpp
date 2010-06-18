#include "../../include/OTL/Registry.h"
#include "../../include/Omega/Remoting.h"
#include "interfaces.h"

void normalise_path(Omega::string_t& strPath);

namespace Omega
{
	namespace TestSuite
	{
		extern "C" const Omega::guid_t OID_TestLibrary;
		extern "C" const Omega::guid_t OID_TestProcess;
	}
}

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestLibrary, "{16C07AEA-242F-48f5-A10E-1DCA3FADB9A6}");
OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestProcess, "{4BC2E65B-CEE0-40c6-90F2-39C7C306FC69}");

#include "Test.h"

bool register_library(const wchar_t* pszLibName, bool& bSkipped)
{
	bSkipped = false;

#if defined(_WIN32)
	if (access(Omega::string_t(pszLibName,Omega::string_t::npos).ToUTF8().c_str(),0) != 0)
	{
		output("[Missing]\n");
		bSkipped = true;
		return true;
	}
#endif

	Omega::string_t strOid = Omega::TestSuite::OID_TestLibrary.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User\\Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey.OpenSubKey(L"Test.Library",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"OID",strOid);
	ptrSubKey = ptrKey.OpenSubKey(L"OIDs\\" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"Library",Omega::string_t(pszLibName,Omega::string_t::npos));

	return true;
}

bool unregister_library()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User\\Objects");

	if (ptrKey->IsSubKey(L"Test.Library"))
		ptrKey->DeleteKey(L"Test.Library");

	Omega::string_t strOid = Omega::TestSuite::OID_TestLibrary.ToString();

	if (ptrKey->IsSubKey(L"OIDs\\" + strOid))
		ptrKey->DeleteKey(L"OIDs\\" + strOid);
	
	return true;
}

bool register_process(const wchar_t* pszExeName, bool& bSkipped)
{
	bSkipped = false;

#if defined(_WIN32)
	if (access(Omega::string_t(pszExeName,Omega::string_t::npos).ToUTF8().c_str(),0) != 0)
	{
		output("[Missing]\n");
		bSkipped = true;
		return true;
	}
#endif

	Omega::string_t strOid = Omega::TestSuite::OID_TestProcess.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User\\Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey.OpenSubKey(L"Test.Process",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"OID",strOid);
	ptrSubKey = ptrKey.OpenSubKey(L"OIDs\\" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"Application",L"CoreTests.TestProcess");

	ptrKey = OTL::ObjectPtr<Omega::Registry::IKey>(L"\\Local User\\Applications",Omega::Registry::IKey::OpenCreate);
	ptrSubKey = ptrKey.OpenSubKey(L"CoreTests.TestProcess\\Activation",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"Path",Omega::string_t(pszExeName,Omega::string_t::npos));

	return true;
}

bool unregister_process()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User\\Objects");

	if (ptrKey->IsSubKey(L"Test.Process"))
		ptrKey->DeleteKey(L"Test.Process");

	Omega::string_t strOid = Omega::TestSuite::OID_TestProcess.ToString();

	if (ptrKey->IsSubKey(L"OIDs\\" + strOid))
		ptrKey->DeleteKey(L"OIDs\\" + strOid);

	ptrKey = OTL::ObjectPtr<Omega::Registry::IKey>(L"\\Local User\\Applications",Omega::Registry::IKey::OpenCreate);
	if (ptrKey->IsSubKey(L"CoreTests.TestProcess"))
		ptrKey->DeleteKey(L"CoreTests.TestProcess");

	return true;
}

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest)
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
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest*>(ptrSimpleTest);
	TEST(ptrSimpleTest2);

	OTL::ObjectPtr<Omega::IObject> ptrO1;
	ptrO1.Attach(ptrSimpleTest->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));
	OTL::ObjectPtr<Omega::IObject> ptrO2;
	ptrO2.Attach(ptrSimpleTest2->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));

	TEST(static_cast<Omega::IObject*>(ptrO1) == static_cast<Omega::IObject*>(ptrO2));

	ptrO1.Release();
	ptrO2.Release();

	// Test the type info
	OTL::ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrPOI = ptrSimpleTest;
	TEST(ptrPOI);

	// Try to get the first interface
	std::set<Omega::guid_t> interfaces = ptrPOI->EnumInterfaces();
	TEST(!interfaces.empty());
	TEST(*interfaces.begin() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));

	OTL::ObjectPtr<Omega::TypeInfo::IInterfaceInfo> ptrII;
	ptrII.Attach(Omega::TypeInfo::GetInterfaceInfo(*interfaces.begin(),ptrSimpleTest));
	TEST(ptrII);

	TEST(ptrII->GetName() == L"Omega::TestSuite::ISimpleTest");
	TEST(ptrII->GetIID() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));
	TEST(ptrII->GetMethodCount() == 25+3);

	return true;
}

namespace
{
	class Aggregator :
			public Omega::TestSuite::ISimpleTest2
	{
	public:
		Aggregator() : m_pInner(0)
		{
			AddRef();
		}

		void SetInner(Omega::IObject* pInner)
		{
			m_pInner = pInner;
		}

		void AddRef()
		{
			m_refcount.AddRef();
		}

		void Release()
		{
			assert(m_refcount.m_debug_value > 0);

			if (m_refcount.Release())
				delete this;
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			if (iid == OMEGA_GUIDOF(Omega::IObject) ||
					iid == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest2))
			{
				AddRef();
				return this;
			}

			if (m_pInner)
			{
				Omega::IObject* pObj = m_pInner->QueryInterface(iid);
				if (pObj)
					return pObj;
			}

			return 0;
		}

		Omega::string_t WhereAmI()
		{
			return Omega::string_t(L"Outer");
		}

	private:
		virtual ~Aggregator()
		{
			if (m_pInner)
				m_pInner->Release();
		}

		Omega::Threading::AtomicRefCount m_refcount;
		Omega::IObject*   m_pInner;
	};
}

static bool do_local_library_test(const wchar_t* pszLibName, bool& bSkipped)
{
	// Register the library
	output("  %-45ls ",pszLibName);

	// Register the library
	TEST(register_library(pszLibName,bSkipped));
	if (bSkipped)
		return true;
	
	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(Omega::TestSuite::OID_TestLibrary,Omega::Activation::InProcess);
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest*>(ptrSimpleTest);
	TEST(ptrSimpleTest2->WhereAmI() == L"Inner");

	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();

	// Test aggregation
	Aggregator* pAgg = 0;
	OMEGA_NEW(pAgg,Aggregator);

	pAgg->SetInner(Omega::CreateInstance(Omega::TestSuite::OID_TestLibrary,Omega::Activation::InProcess,pAgg,OMEGA_GUIDOF(Omega::IObject)));

	ptrSimpleTest2.Attach(static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg));
	TEST(ptrSimpleTest2->WhereAmI() == L"Outer");

	ptrSimpleTest.Attach(ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>());
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Check QI rules (again)
	OTL::ObjectPtr<Omega::IObject> ptrO1;
	ptrO1.Attach(ptrSimpleTest->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));
	OTL::ObjectPtr<Omega::IObject> ptrO2;
	ptrO2.Attach(ptrSimpleTest2->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));

	TEST(static_cast<Omega::IObject*>(ptrO1) == static_cast<Omega::IObject*>(ptrO2));

	ptrO1.Release();
	ptrO2.Release();

	ptrSimpleTest2.Release();
	ptrSimpleTest.Release();

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
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test for local activation with '@local'
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestLibrary.ToString() + L"@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test redirecting the registration
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User\\Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey.OpenSubKey(L"MyLittleTest",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue(L"CurrentVersion",L"Test.Library");
	
	ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest@local");
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test it has gone
	ptrKey->DeleteKey(L"MyLittleTest");
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"MyLittleTest");
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test unregistering
	TEST(unregister_library());

	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Library");
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
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

static bool do_local_process_test(const wchar_t* pszModulePath, bool& bSkipped)
{
	output("  %-45ls ",pszModulePath);

	// Register the exe
	TEST(register_process(pszModulePath,bSkipped));
	if (bSkipped)
		return true;

	// Test the simplest case
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(Omega::TestSuite::OID_TestProcess,Omega::Activation::OutOfProcess);
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest*>(ptrSimpleTest);
	TEST(ptrSimpleTest2->WhereAmI() == L"Inner");

	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();

	// Test aggregation
	Aggregator* pAgg = 0;
	OMEGA_NEW(pAgg,Aggregator);

	pAgg->SetInner(Omega::CreateInstance(Omega::TestSuite::OID_TestProcess,Omega::Activation::OutOfProcess,pAgg,OMEGA_GUIDOF(Omega::IObject)));

	ptrSimpleTest2.Attach(static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg));
	TEST(ptrSimpleTest2->WhereAmI() == L"Outer");

	ptrSimpleTest.Attach(ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>());
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Check QI rules (again)
	OTL::ObjectPtr<Omega::IObject> ptrO1;
	ptrO1.Attach(ptrSimpleTest->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));
	OTL::ObjectPtr<Omega::IObject> ptrO2;
	ptrO2.Attach(ptrSimpleTest2->QueryInterface(OMEGA_GUIDOF(Omega::IObject)));

	TEST(static_cast<Omega::IObject*>(ptrO1) == static_cast<Omega::IObject*>(ptrO2));

	ptrO1.Release();
	ptrO2.Release();

	ptrSimpleTest2.Release();
	ptrSimpleTest.Release();

	// Now check for activation rules
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(Omega::TestSuite::OID_TestProcess,Omega::Activation::OutOfProcess);
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
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::IException* pE)
	{
		add_success();
		pE->Release();
	}

	// Test unregistering
	TEST(unregister_process());
	
	// Check its gone
	try
	{
		ptrSimpleTest = OTL::ObjectPtr<Omega::TestSuite::ISimpleTest>(L"Test.Process");
	}
	catch (Omega::Activation::IOidNotFoundException* pE)
	{
		add_success();
		pE->Release();
	}

	return true;
}

const wchar_t** get_dlls()
{
	static const wchar_t* dlls[] =
	{
#if defined(_MSC_VER)
		L"TestLibrary_msvc.dll",
	#if defined(_DEBUG)
			L"..\\..\\build\\test\\CoreTests\\TestLibrary\\.libs\\TestLibrary.dll",
	#else
			L"..\\build\\test\\CoreTests\\TestLibrary\\.libs\\TestLibrary.dll",
	#endif
#elif defined(_WIN32)
		OMEGA_WIDEN_STRINGIZE(TOP_SRC_DIR) L"/bin/TestLibrary_msvc.dll",
		OMEGA_WIDEN_STRINGIZE(TOP_SRC_DIR) L"/bin/Debug/TestLibrary_msvc.dll",
		L"CoreTests/TestLibrary/.libs/TestLibrary.dll",
#else
		L"CoreTests/TestLibrary/testlibrary",
#endif
		0
	};
	return dlls;
}

bool interface_dll_tests()
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_local_library_test(*pszDlls,bSkipped);

		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

const wchar_t** get_exes()
{
	static const wchar_t* exes[] =
	{
#if defined(_MSC_VER)
		L"TestProcess_msvc.exe",
	#if defined(_DEBUG)
			L"..\\..\\build\\test\\CoreTests\\TestProcess\\testprocess.exe",
	#else
			L"..\\build\\test\\CoreTests\\TestProcess\\testprocess.exe",
	#endif
#elif defined(_WIN32)
		OMEGA_WIDEN_STRINGIZE(TOP_SRC_DIR) L"/bin/TestProcess_msvc.exe",
		OMEGA_WIDEN_STRINGIZE(TOP_SRC_DIR) L"/bin/Debug/TestProcess_msvc.exe",
		OMEGA_WIDEN_STRINGIZE(BUILD_DIR) L"/TestProcess/testprocess.exe",
#else
		OMEGA_WIDEN_STRINGIZE(BUILD_DIR) L"/TestProcess/testprocess",
#endif
		0
	};
	return exes;
}

bool interface_process_tests()
{
	output("\n");

	for (const wchar_t** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_local_process_test(*pszExes,bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	return true;
}

static bool do_library_test(const wchar_t* pszLibName, const wchar_t* pszEndpoint, bool& bSkipped)
{
	// Register the library ready for local loopback stuff
	output("  %-45ls ",pszLibName);

	// Register the library
	TEST(register_library(pszLibName,bSkipped));
	if (bSkipped)
		return true;
		
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(L"Test.Library@" + Omega::string_t(pszEndpoint,Omega::string_t::npos));
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	TEST(unregister_library());

	return true;
}

static bool do_process_test(const wchar_t* pszModulePath, const wchar_t* pszEndpoint, bool& bSkipped)
{
	output("  %-45ls ",pszModulePath);

	// Register the exe
	TEST(register_process(pszModulePath,bSkipped));
	if (bSkipped)
		return true;

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(L"Test.Process@" + Omega::string_t(pszEndpoint,Omega::string_t::npos));
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	TEST(unregister_process());

	return true;
}

static bool interface_tests_i(const wchar_t* pszHost)
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_library_test(*pszDlls,pszHost,bSkipped);

		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	for (const wchar_t** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_process_test(*pszExes,pszHost,bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

bool interface_tests2()
{
	//return interface_tests_i(L"http://TSS04:8901/");
	return interface_tests_i(L"http://localhost:8901");
}
