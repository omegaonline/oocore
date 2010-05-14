#include <OTL/OTL.h>
#include <Omega/Apartment.h>
#include "interfaces.h"

#include "Test.h"

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest);
bool register_library(const wchar_t* pszLibName, bool& bSkipped);
bool unregister_library();
bool register_process(const wchar_t* pszExeName, bool& bSkipped);
bool unregister_process();

static bool do_apt_library_test(const wchar_t* pszLibName, bool& bSkipped)
{
	output("  %-45ls ",pszLibName);

	// Register the library
	TEST(register_library(pszLibName,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an apartment
	OTL::ObjectPtr<Omega::Apartment::IApartment> ptrApartment;
	ptrApartment.Attach(Omega::Apartment::IApartment::Create());
	TEST(ptrApartment);

	// try to create the object asking for TypeInfo::IProvideObjectInfo
	Omega::IObject* pObject = 0;
	ptrApartment->CreateInstance(L"Test.Library",Omega::Activation::InProcess,0,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObject);
	TEST(pObject);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObject));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Test unregistering
	TEST(unregister_library());

	return true;
}

const wchar_t** get_dlls();

bool apartment_dll_tests()
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls(); *pszDlls; ++pszDlls)
	{
		bool bSkipped;
		bool res = do_apt_library_test(*pszDlls,bSkipped);
		
		unregister_library();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}

static bool do_apt_process_test(const wchar_t* pszModulePath, bool& bSkipped)
{
	output("  %-45ls ",pszModulePath);

	// Register the process
	TEST(register_process(pszModulePath,bSkipped));
	if (bSkipped)
		return true;
		
	// Create an apartment
	OTL::ObjectPtr<Omega::Apartment::IApartment> ptrApartment;
	ptrApartment.Attach(Omega::Apartment::IApartment::Create());
	TEST(ptrApartment);

	// try to create the object asking for TypeInfo::IProvideObjectInfo
	Omega::IObject* pObject = 0;
	ptrApartment->CreateInstance(L"Test.Process",Omega::Activation::OutOfProcess,0,OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest),pObject);
	TEST(pObject);

	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest;
	ptrSimpleTest.Attach(static_cast<Omega::TestSuite::ISimpleTest*>(pObject));

	// Test the interface
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);

	// Kill the running version
	try
	{
		ptrSimpleTest->Abort();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}

	// Test unregistering
	TEST(unregister_process());

	return true;
}

const wchar_t** get_exes();

bool apartment_process_tests()
{
	output("\n");

	for (const wchar_t** pszExes = get_exes(); *pszExes; ++pszExes)
	{
		bool bSkipped;
		bool res = do_apt_process_test(*pszExes,bSkipped);

		unregister_process();

		if (res && !bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");

	return true;
}
