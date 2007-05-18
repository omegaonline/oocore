#include "Test.h"

struct AutoUninit
{
	AutoUninit() : bInitCalled(false)
	{}

	~AutoUninit()
	{
		if (bInitCalled)
			Omega::Uninitialize();
	}

	bool bInitCalled;
};

static AutoUninit s_auto_init;

static void call_init()
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		add_failure();
		printf("[Failed]\n\tOmega::Initialize failed: %s\n",(const char*)pE->Description());
		exit(test_summary());
	}

	s_auto_init.bInitCalled = true;
}

bool core_tests()
{
	call_init();
	
	return true;
}

static bool complex_throw()
{
	const Omega::char_t szDesc[] = "A test description";
	const Omega::char_t szFile[] = __FILE__;

	// try a more complex throw
	try
	{
		try
		{
			Omega::INoInterfaceException::Throw(OMEGA_UUIDOF(Omega::IObject),szFile);
		}
		catch (Omega::INoInterfaceException* pE)
		{
			TEST(pE->ActualIID() == OMEGA_UUIDOF(Omega::INoInterfaceException));
			TEST(pE->GetUnsupportedIID() == OMEGA_UUIDOF(Omega::IObject));
			TEST(!pE->Cause());
			TEST(pE->Source() == szFile);
			
			Omega::IException::Throw(szDesc,szFile,pE);
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
	}

	return true;
}

bool exception_tests()
{
	// Try a simple throw...
	const Omega::char_t szDesc[] = "A test description";
	const Omega::char_t szFile[] = __FILE__;
	try
	{
		Omega::IException::Throw(szDesc,szFile);
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
		Omega::INoInterfaceException::Throw(OMEGA_UUIDOF(Omega::IObject),szFile);
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
