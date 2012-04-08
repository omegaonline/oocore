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
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		if (pE->GetDescription() == "Failed to connect to network daemon")
		{
			pE->Release();
			output("[No server]\n");
		}
		else
		{
			output("[Omega::IException]\n");
			output_exception(pE);
			pE->Release();
		}
		return false;
	}

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
		throw Omega::INoInterfaceException::Create(OMEGA_GUIDOF(Omega::IObject));
	}
	catch (Omega::INoInterfaceException* pE)
	{
		TEST(pE->GetThrownIID() == OMEGA_GUIDOF(Omega::INoInterfaceException));
		TEST(pE->GetUnsupportedIID() == OMEGA_GUIDOF(Omega::IObject));
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
