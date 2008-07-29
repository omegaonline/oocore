#include <OTL/OTL.h>
#include "./interfaces.h"
#include "TestLibrary/TestLibrary.h"

#include "Test.h"

BEGIN_PROCESS_OBJECT_MAP(L"CoreTests")
END_PROCESS_OBJECT_MAP()

static bool do_interface_tests(OTL::ObjectPtr<Test::Iface>& ptrTestLib)
{
	TEST(ptrTestLib->Hello() == L"Hello!");

	try
	{
		ptrTestLib->Throw(EACCES);
	}
	catch (Omega::ISystemException* pE)
	{
		TEST(pE->ErrorCode() == EACCES);
		pE->Release();
	}

	// This is a test for channel closing
	/*try
	{
		ptrTestLib->Abort();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}*/

	return true;
}

static bool do_library_test(const wchar_t* pszLibName, const wchar_t* pszObject, const wchar_t* pszEndpoint)
{
	int err = system((Omega::string_t(L"OORegister -i -s ") + pszLibName).ToUTF8().c_str());
	if (err != 0)
		return false;

	OTL::ObjectPtr<Test::Iface> iface(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);

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

	iface = OTL::ObjectPtr<Test::Iface>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);
	
	Omega::Registry::AddXML(strXML,false);

	iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);

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

	iface = OTL::ObjectPtr<Test::Iface>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);
	
	Omega::Registry::AddXML(strXML,false);

	iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);
	
	TEST(system((Omega::string_t(L"OORegister -u -s ") + pszLibName).ToUTF8().c_str()) == 0);

	return true;
}

static bool do_process_test(const wchar_t* pszModulePath, const wchar_t* pszObject, const wchar_t* pszEndpoint)
{
	int err = system((Omega::string_t(L"TestProcess -i MODULE_PATH=") + pszModulePath).ToUTF8().c_str());
	if (err != 0)
		return false;

	OTL::ObjectPtr<Test::Iface> iface;

	if (!pszEndpoint)
	{
		iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);
		
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
		
		iface = OTL::ObjectPtr<Test::Iface>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);
		
		Omega::Registry::AddXML(strXML,false);
		
		iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);

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

		iface = OTL::ObjectPtr<Test::Iface>(L"MyLittleTest",Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);
		
		Omega::Registry::AddXML(strXML,false);

		iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);

		OTL::ObjectPtr<Omega::Registry::IKey> ptrReg(L"\\Applications\\TestProcess");
		ptrReg->SetIntegerValue(L"Public",0);

		iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
		do_interface_tests(iface);
	}

	OTL::ObjectPtr<Omega::Registry::IKey> ptrReg(L"\\Applications\\TestProcess");
	ptrReg->SetIntegerValue(L"Public",1);

	iface = OTL::ObjectPtr<Test::Iface>(pszObject,Omega::Activation::Any,0,pszEndpoint);
	do_interface_tests(iface);

	TEST(system((Omega::string_t(L"TestProcess -u MODULE_PATH=") + pszModulePath).ToUTF8().c_str()) == 0);

	return true;
}

bool interface_tests()
{
#if defined(OMEGA_WIN32)
	//do_library_test(L"TestLibrary_msvc",L"Test.Library.msvc",0);
	//do_library_test(L"TestLibrary_mingw",L"Test.Library.mingw",0);
#else
	do_library_test(L"TestLibrary",L"Test.Library",0);
#endif

	do_process_test(L"TestProcess",L"Test.Process",0);

	return true;
}

bool interface_tests2()
{
	const wchar_t host[] = L"http://localhost:8901";

#if defined(OMEGA_WIN32)
	do_library_test(L"TestLibrary_msvc",L"Test.Library.msvc",host);
	//do_library_test(L"TestLibrary_mingw",L"Test.Library.mingw",host);
#else
	do_library_test(L"TestLibrary",L"Test.Library",host);
#endif

	do_process_test(L"TestProcess",L"Test.Process",host);

	return true;
}
