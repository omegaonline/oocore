#include <OTL/OTL.h>
#include <OOCore/Apartment.h>
#include "interfaces.h"

#if defined(_WIN32)
#define OOREGISTER L"OORegister"
#else
#define OOREGISTER L"./ooregister"
#endif

#include "Test.h"

bool interface_tests(OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest);

static bool do_local_library_test(const wchar_t* pszLibName)
{
	// Register the library
	if (system((Omega::string_t(OOREGISTER L" -i -s ") + pszLibName).ToUTF8().c_str()) != 0)
    	return true;

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
	Omega::IEnumGuid* pEG = ptrPOI->EnumInterfaces();
	OTL::ObjectPtr<Omega::IEnumGuid> ptrEG;
	ptrEG.Attach(static_cast<Omega::IEnumGuid*>(pEG));
	TEST(ptrEG);

	Omega::uint32_t count = 1;
	Omega::guid_t iid;
	ptrEG->Next(count,&iid);
	TEST(count!=0);
	TEST(iid == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));

	// Confirm we can QI for all the interfaces we need...
	OTL::ObjectPtr<Omega::System::IProxy> ptrProxy(ptrPOI);
	TEST(ptrProxy);

	OTL::ObjectPtr<Omega::System::IMarshaller> ptrMarshaller;
	ptrMarshaller.Attach(ptrProxy->GetMarshaller());
	TEST(ptrMarshaller);

	OTL::ObjectPtr<Omega::TypeInfo::ITypeInfo> ptrTI;
	ptrTI.Attach(ptrMarshaller->GetTypeInfo(iid));
	TEST(ptrTI);

	TEST(ptrTI->GetName() == L"Omega::TestSuite::ISimpleTest");
	TEST(ptrTI->GetIID() == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest));
	TEST(ptrTI->GetMethodCount() == 23+3);

	// Test the interface
	OTL::ObjectPtr<Omega::TestSuite::ISimpleTest> ptrSimpleTest(ptrPOI);
	TEST(ptrSimpleTest);
	interface_tests(ptrSimpleTest);
		
	// Test unregistering
	TEST(system((Omega::string_t(OOREGISTER L" -u -s ") + pszLibName).ToUTF8().c_str()) == 0);

	return true;
}

static bool do_local_process_test(const wchar_t* pszModulePath)
{
	TEST(system((Omega::string_t(pszModulePath) + L" -i MODULE_PATH=" + pszModulePath).ToUTF8().c_str()) == 0);

	TEST(system((Omega::string_t(pszModulePath) +  L" -u MODULE_PATH=" + pszModulePath).ToUTF8().c_str()) == 0);

	return true;
}

bool apartment_dll_tests()
{
#if defined(_WIN32)
	do_local_library_test(L"TestLibrary_msvc");
	do_local_library_test(L"TestLibrary_mingw");
#else
	do_local_library_test(L"./libTestLibrary.so");
#endif

	return true;
}

bool apartment_tests()
{
#if defined(_WIN32)
	do_local_process_test(L"TestProcess");
#else
    do_local_process_test(L"./testprocess");
#endif

	return true;
}
