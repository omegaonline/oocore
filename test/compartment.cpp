#include "../include/Omega/Compartment.h"
#include "../include/OTL/OTL.h"
#include "../include/Omega/Remoting.h"
#include "interfaces.h"
#include "aggregator.h"

#include "Test.h"

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest);
bool register_library(const Omega::string_t& strLibName, bool& bSkipped);
bool unregister_library();
bool register_process(const Omega::string_t& strExeName, bool& bSkipped);
bool unregister_process();

static bool do_cmpt_library_test(const Omega::string_t& strLibName, bool& bSkipped)
{
	output("  %-45ls ",strLibName.c_wstr());

	// Register the library
	TEST(register_library(strLibName,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an compartment
	OTL::ObjectPtr<Omega::Compartment::ICompartment> ptrCompartment;
	ptrCompartment.Attach(Omega::Compartment::ICompartment::Create());
	TEST(ptrCompartment);

	Omega::IObject* pObj = 0;
	ptrCompartment->CreateInstance(L"Test.Library",Omega::Activation::InProcess,NULL,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObj);
	TEST(pObj);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObj));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);
	
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest*>(ptrSimpleTest);
	TEST(ptrSimpleTest2->WhereAmI() == L"Inner");
	
	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();
	
	// Test aggregation
	Aggregator* pAgg = new Aggregator();
	
	pObj = 0;
	ptrCompartment->CreateInstance(L"Test.Library",Omega::Activation::InProcess,pAgg,OMEGA_GUIDOF(Omega::IObject),pObj);
	TEST(pObj);

	pAgg->SetInner(pObj);

	ptrSimpleTest2.Attach(static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg));
	TEST(ptrSimpleTest2->WhereAmI() == L"Outer");

	ptrSimpleTest.Attach(ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>());
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest); 

	return true;
}

const wchar_t** get_dlls();
Omega::string_t make_absolute(const wchar_t* wsz);

bool compartment_dll_tests()
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_cmpt_library_test(make_absolute(*pszDlls),bSkipped);
		
		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

static bool do_cmpt_process_test(const Omega::string_t& strModulePath, bool& bSkipped)
{
	output("  %-45ls ",strModulePath.c_wstr());

	// Register the process
	TEST(register_process(strModulePath,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an compartment
	OTL::ObjectPtr<Omega::Compartment::ICompartment> ptrCompartment;
	ptrCompartment.Attach(Omega::Compartment::ICompartment::Create());
	TEST(ptrCompartment);

	Omega::IObject* pObj = 0;
	ptrCompartment->CreateInstance(L"Test.Process",Omega::Activation::OutOfProcess,NULL,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObj);
	TEST(pObj);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObj));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);
	
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest2> ptrSimpleTest2 = static_cast<Omega::TestSuite::ISimpleTest*>(ptrSimpleTest);
	TEST(ptrSimpleTest2->WhereAmI() == L"Inner");
	
	ptrSimpleTest.Release();
	ptrSimpleTest2.Release();
	
	// Test aggregation
	Aggregator* pAgg = new Aggregator();
	
	pObj = NULL;
	ptrCompartment->CreateInstance(L"Test.Process",Omega::Activation::OutOfProcess,pAgg,OMEGA_GUIDOF(Omega::IObject),pObj);
	TEST(pObj);

	pAgg->SetInner(pObj);

	ptrSimpleTest2.Attach(static_cast<Omega::TestSuite::ISimpleTest2*>(pAgg));
	TEST(ptrSimpleTest2->WhereAmI() == L"Outer");

	ptrSimpleTest.Attach(ptrSimpleTest2.QueryInterface<Omega::TestSuite::ISimpleTest>());
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	ptrSimpleTest2.Release(); 
	
	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		add_success();
	}

	return true;
}

const wchar_t** get_exes();

bool compartment_process_tests()
{
	output("\n");

	for (const wchar_t** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_cmpt_process_test(make_absolute(*pszExes),bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	return true;
}
