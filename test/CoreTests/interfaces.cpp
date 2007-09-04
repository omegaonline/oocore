#include <OTL/OTL.h>
#include "./interfaces.h"
#include "TestLibrary/TestLibrary.h"

#include "Test.h"

static bool do_interface_tests(OTL::ObjectPtr<Test::Iface>& ptrTestLib)
{
	TEST(ptrTestLib->Hello() == L"Hello!");

	return true;
}

static bool do_library_test(const wchar_t* pszLibName, const wchar_t* pszObject)
{
	int err = test_system((Omega::string_t(L"OORegister -i -s ") + pszLibName).c_str());
	if (err != 0)
		return false;

	{
		OTL::ObjectPtr<Test::Iface> iface(pszObject);
		do_interface_tests(iface);
	}

	Omega::string_t strXML =
		L"<?xml version=\"1.0\" ?>"
		L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
			L"<key name=\"Objects\">"
				L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
					L"<value name=\"CurrentVersion\">%OBJECT%</value>"
				L"</key>"
			L"</key>"
		L"</root>";

	Omega::string_t strSubsts = L"OBJECT=";
	strSubsts += pszObject;

	Omega::Registry::AddXML(strXML,true,strSubsts);

	{
		OTL::ObjectPtr<Test::Iface> iface(L"MyLittleTest");
		do_interface_tests(iface);
	}

	Omega::Registry::AddXML(strXML,false,L"");

	{
		OTL::ObjectPtr<Test::Iface> iface(pszObject);
		do_interface_tests(iface);
	}

	TEST(test_system((Omega::string_t(L"OORegister -u -s ") + pszLibName).c_str()) == 0);

	return true;
}

static bool do_process_test(const wchar_t* pszModulePath, const wchar_t* pszObject)
{
	int err = test_system((Omega::string_t(L"TestProcess -i MODULE_PATH=") + pszModulePath).c_str());
	if (err != 0)
		return false;

	{
		OTL::ObjectPtr<Test::Iface> iface(pszObject);
		do_interface_tests(iface);
	}

	Omega::string_t strXML =
		L"<?xml version=\"1.0\" ?>"
		L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
			L"<key name=\"Objects\">"
				L"<key name=\"MyLittleTest\" uninstall=\"Remove\">"
					L"<value name=\"CurrentVersion\">%OBJECT%</value>"
				L"</key>"
			L"</key>"
		L"</root>";

	Omega::string_t strSubsts = L"OBJECT=";
	strSubsts += pszObject;

	Omega::Registry::AddXML(strXML,true,strSubsts);

	{
		OTL::ObjectPtr<Test::Iface> iface(L"MyLittleTest");
		do_interface_tests(iface);
	}

	Omega::Registry::AddXML(strXML,false,L"");

	{
		OTL::ObjectPtr<Test::Iface> iface(pszObject);
		do_interface_tests(iface);
	}

	TEST(test_system((Omega::string_t(L"TestProcess -u MODULE_PATH=") + pszModulePath).c_str()) == 0);

	return true;
}

bool interface_tests()
{
//#if defined(OMEGA_WIN32)
//    do_library_test(L"TestLibrary_msvc",L"Test.Library.msvc");
//	do_library_test(L"TestLibrary_mingw",L"Test.Library.mingw");
//#else
//	do_library_test(L"TestLibrary",L"Test.Library");
//#endif

	do_process_test(L"TestProcess",L"Test.Process");

	return true;
}
