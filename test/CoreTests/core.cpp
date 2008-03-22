#include <OOCore/OOCore.h>
#include "Test.h"

bool init_tests()
{
	static struct AutoUninit
	{
		AutoUninit() : bInitCalled(false)
		{}

		~AutoUninit()
		{
			if (bInitCalled)
				Omega::Uninitialize();
		}

		bool bInitCalled;
	} auto_uninit;

	// Call Omega::Initialze and remember we have...
	Omega::IException* pE = Omega::Initialize();
	if (pE)
		throw pE;

	auto_uninit.bInitCalled = true;
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
			throw Omega::INoInterfaceException::Create(OMEGA_UUIDOF(Omega::IObject),szFile);
		}
		catch (Omega::INoInterfaceException* pE)
		{
			TEST(pE->ActualIID() == OMEGA_UUIDOF(Omega::INoInterfaceException));
			TEST(pE->GetUnsupportedIID() == OMEGA_UUIDOF(Omega::IObject));
			TEST(!pE->Cause());
			TEST(pE->Source() == szFile);

			Omega::IException* pE2 = Omega::IException::Create(szDesc,szFile,pE);
			pE->Release();
			throw pE2;
		}
	}
	catch (Omega::IException* pE)
	{
		TEST(pE->Description() == szDesc);
		TEST(pE->Source() == szFile);

		Omega::IException* pE2 = pE->Cause();
		TEST(pE2->ActualIID() == OMEGA_UUIDOF(Omega::INoInterfaceException));
		TEST(pE2->Source() == szFile);

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
		TEST(pE->Description() == szDesc);
		TEST(!pE->Cause());
		TEST(pE->Source() == szFile);
		pE->Release();
	}

	// Try another simple throw
	try
	{
		throw Omega::INoInterfaceException::Create(OMEGA_UUIDOF(Omega::IObject),szFile);
	}
	catch (Omega::INoInterfaceException* pE)
	{
		TEST(pE->ActualIID() == OMEGA_UUIDOF(Omega::INoInterfaceException));
		TEST(pE->GetUnsupportedIID() == OMEGA_UUIDOF(Omega::IObject));
		TEST(!pE->Cause());
		TEST(pE->Source() == szFile);
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
