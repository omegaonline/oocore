#include "../include/Omega/Compartment.h"
#include "../include/OTL/OTL.h"
#include "../include/Omega/Remoting.h"
#include "interfaces.h"

#include "Test.h"

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest);
bool register_library(const wchar_t* pszLibName, bool& bSkipped);
bool unregister_library();
bool register_process(const wchar_t* pszExeName, bool& bSkipped);
bool unregister_process();

static bool do_cmpt_library_test(const wchar_t* pszLibName, bool& bSkipped)
{
	output("  %-45ls ",pszLibName);

	// Register the library
	TEST(register_library(pszLibName,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an compartment
	OTL::ObjectPtr<Omega::Compartment::ICompartment> ptrCompartment;
	ptrCompartment.Attach(Omega::Compartment::ICompartment::Create());
	TEST(ptrCompartment);

	Omega::IObject* pObj = 0;
	ptrCompartment->CreateInstance(L"Test.Library",Omega::Activation::InProcess,0,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObj);
	TEST(pObj);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObj));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	return true;
}

const wchar_t** get_dlls();

bool compartment_dll_tests()
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_cmpt_library_test(*pszDlls,bSkipped);
		
		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

static bool do_cmpt_process_test(const wchar_t* pszModulePath, bool& bSkipped)
{
	output("  %-45ls ",pszModulePath);

	// Register the process
	TEST(register_process(pszModulePath,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an compartment
	OTL::ObjectPtr<Omega::Compartment::ICompartment> ptrCompartment;
	ptrCompartment.Attach(Omega::Compartment::ICompartment::Create());
	TEST(ptrCompartment);

	Omega::IObject* pObj = 0;
	ptrCompartment->CreateInstance(L"Test.Process",Omega::Activation::OutOfProcess,0,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObj);
	TEST(pObj);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObj));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

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
		bool res = do_cmpt_process_test(*pszExes,bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	return true;
}
