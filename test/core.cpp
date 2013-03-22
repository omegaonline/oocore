#include "../include/Omega/Omega.h"
#include "Test.h"

void normalise_path(Omega::string_t& strPath)
{
#if defined(_WIN32)
	char from = '/';
	char to = '\\';
#else
	char from = '\\';
	char to = '/';
#endif

	for (;;)
	{
		size_t p=strPath.Find(from);
		if (p == Omega::string_t::npos)
			break;

		strPath = strPath.Left(p) + to + strPath.Mid(p+1);
	}
}

bool init_tests()
{
	Omega::Initialize();

	return true;
}

bool exception_tests()
{
	// Try a simple throw...
	try
	{
		throw Omega::ISystemException::Create(2);
	}
	catch (Omega::ISystemException* pE)
	{
		TEST(pE->GetErrorCode() == 2);
		TEST(!pE->GetCause());
		pE->Release();
	}

	// Try another simple throw
	try
	{
		throw Omega::INotFoundException::Create("Bang!");
	}
	catch (Omega::INotFoundException* pE)
	{
		TEST(pE->GetThrownIID() == OMEGA_GUIDOF(Omega::INotFoundException));
		TEST(!pE->GetCause());
		pE->Release();
	}

	return true;
}

#include "../include/OTL/OTL.h"

bool otl_tests()
{
	OTL::ObjectPtr<Omega::IObject> ptrObj;
	TEST(!ptrObj);
	TEST(ptrObj == (Omega::IObject*)0);
	TEST(ptrObj == ptrObj);
	TEST(ptrObj == static_cast<Omega::IObject*>(ptrObj));

	return true;
}
