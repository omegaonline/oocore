#include <OOCore/OOCore.h>
#include "Test.h"

bool init_tests()
{
	// Call Omega::Initialze and remember we have...
	Omega::IException* pE = Omega::Initialize();
	if (pE)
		throw pE;

	return true;
}

static bool complex_throw()
{
	/*const wchar_t szDesc[] = L"A test description";
	const wchar_t szFile[] = OMEGA_WIDEN_STRING(__FILE__);

	// try a more complex throw
	try
	{
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

			Omega::IException* pE2 = Omega::IException::Create(szDesc,szFile,pE);
			pE->Release();
			throw pE2;
		}
	}
	catch (Omega::IException* pE)
	{
		TEST(pE->GetDescription() == szDesc);
		TEST(pE->GetSource() == szFile);

		Omega::IException* pE2 = pE->GetCause();
		TEST(pE2->GetThrownIID() == OMEGA_GUIDOF(Omega::INoInterfaceException));
		TEST(pE2->GetSource() == szFile);

		pE2->Release();
		pE->Release();
	}*/

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

	TEST(complex_throw());

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
