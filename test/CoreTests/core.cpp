#include <Omega/Omega.h>
#include "Test.h"

bool init_standalone_tests()
{
	std::map<Omega::string_t,Omega::string_t> args;
	Omega::IException* pE = Omega::InitStandalone(args);
	if (pE)
	{
		output("[Omega::IException]\n");
		output_exception(pE);
		pE->Release();
		return false;
	}

	return true;
}

bool init_server_tests(bool& bRun)
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		if (pE->GetDescription() == L"Failed to connect to network daemon")
		{
			pE->Release();
			output("[No server]\n");
			bRun = false;
			return true;
		}

		output("[Omega::IException]\n");
		output_exception(pE);
		pE->Release();
		return false;
	}

	return true;
}

bool exception_tests()
{
	// Try a simple throw...
	const wchar_t szDesc[] = L"A test description";
	const wchar_t szFile[] = OMEGA_WIDEN_STRING(__FILE__);
	try
	{
		throw Omega::ISystemException::Create(szDesc,szFile);
	}
	catch (Omega::IException* pE)
	{
		TEST(pE->GetDescription() == szDesc);
		TEST(!pE->GetCause());
		TEST(pE->GetSource() == szFile);
		pE->Release();
	}

	// Try another simple throw
	try
	{
		throw Omega::INoInterfaceException::Create(OMEGA_GUIDOF(Omega::IObject),szFile);
	}
	catch (Omega::INoInterfaceException* pE)
	{
		TEST(pE->GetThrownIID() == OMEGA_GUIDOF(Omega::INoInterfaceException));
		TEST(pE->GetUnsupportedIID() == OMEGA_GUIDOF(Omega::IObject));
		TEST(!pE->GetCause());
		TEST(pE->GetSource() == szFile);
		pE->Release();
	}

	return true;
}

#include <OTL/OTL.h>

bool otl_tests()
{
	OTL::ObjectPtr<Omega::IObject> ptrObj;
	TEST(!ptrObj);
	TEST(ptrObj == (Omega::IObject*)0);
	TEST(ptrObj == ptrObj);
	TEST(ptrObj == static_cast<Omega::IObject*>(ptrObj));

	return true;
}
