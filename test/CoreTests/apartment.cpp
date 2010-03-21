#include <OTL/OTL.h>
#include <Omega/Apartment.h>
#include "interfaces.h"

#if defined(_WIN32)
#define OOREGISTER L"ooregister -s -c"
#else
#define OOREGISTER L"./ooregister -s -c"
#endif

#include "Test.h"

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest);

static bool do_apt_library_test(const wchar_t* pszLibName, bool& bSkipped)
{
	output("  %-45ls ",pszLibName);

	// Register the library
#if defined(_WIN32)
	if (access(Omega::string_t(pszLibName).ToUTF8().c_str(),0) != 0)
	{
		output("[Missing]\n");
		bSkipped = true;
		return true;
	}
#endif

	bSkipped = false;
	if (system((Omega::string_t(OOREGISTER L" -i ") + pszLibName).ToUTF8().c_str()) != 0)
	{
		add_failure(L"Registration failed\n");
		return false;
	}

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
	TEST(system((Omega::string_t(OOREGISTER L" -u ") + pszLibName).ToUTF8().c_str()) == 0);

	return true;
}

const wchar_t** get_dlls();

bool apartment_dll_tests()
{
	output("\n");

	for (const wchar_t** pszDlls = get_dlls();*pszDlls;++pszDlls)
	{
		bool bSkipped;
		if (!do_apt_library_test(*pszDlls,bSkipped))
			return false;
		if (!bSkipped)
			output("[Ok]\n");
	}

	output("  %-46s","Result");
	return true;
}
