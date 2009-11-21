#include <OTL/OTL.h>
#include <OOCore/Apartment.h>
#include "interfaces.h"

#if defined(_WIN32)
#define OOREGISTER L"ooregister -s"
#else
#define OOREGISTER L"./ooregister -s"
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
	ptrApartment->CreateInstance(L"Test.Library",Omega::Activation::InProcess,NULL,OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo),pObject);
	TEST(pObject);

	OTL::ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrPOI;
	ptrPOI.Attach(static_cast<Omega::TypeInfo::IProvideObjectInfo*>(pObject));
	pObject = 0;
	TEST(ptrPOI);

	// Try to get the first interface
	std::list<Omega::guid_t> interfaces = ptrPOI->EnumInterfaces();
	TEST(!interfaces.empty());
	TEST(interfaces.front() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));

	// Confirm we can QI for all the interfaces we need...
	OTL::ObjectPtr<Omega::Remoting::IProxy> ptrProxy(ptrPOI);
	TEST(ptrProxy);

	OTL::ObjectPtr<Omega::Remoting::IMarshaller> ptrMarshaller;
	ptrMarshaller.Attach(ptrProxy->GetMarshaller());
	TEST(ptrMarshaller);

	OTL::ObjectPtr<Omega::TypeInfo::ITypeInfo> ptrTI;
	ptrTI.Attach(ptrMarshaller->GetTypeInfo(*interfaces.begin()));
	TEST(ptrTI);

	TEST(ptrTI->GetName() == L"Omega::TestSuite::ISimpleTest");
	TEST(ptrTI->GetIID() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));
	TEST(ptrTI->GetMethodCount() == 25+3);

	// Test the interface
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(ptrPOI);
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
