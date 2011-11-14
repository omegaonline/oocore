#include "../include/Omega/Omega.h"
#include "Test.h"

void normalise_path(Omega::string_t& strPath)
{
#if defined(_WIN32)
	wchar_t from = L'/';
	wchar_t to = L'\\';
#else
	wchar_t from = L'\\';
	wchar_t to = L'/';
#endif

	for (;;)
	{
		size_t p=strPath.Find(from);
		if (p == Omega::string_t::npos)
			break;

		strPath = strPath.Left(p) + to + strPath.Mid(p+1);
	}
}

bool init_standalone_tests()
{
#if defined(_MSC_VER)
	Omega::string_t regdb_path = L"..\\..\\..\\build\\data\\";
#else
	Omega::string_t regdb_path = OMEGA_WIDEN_STRINGIZE(BUILD_DIR) L"/../data/";
#endif

	normalise_path(regdb_path);
	
	Omega::string_t args(L"standalone=always");
	args += L" , regdb_path=" + regdb_path;
	args += L"\t,user_regdb = " + regdb_path + L"default_user.regdb";
	
	Omega::IException* pE = Omega::Initialize(args);
	if (pE)
	{
		output("[Omega::IException]\n");
		output_exception(pE);
		pE->Release();
		return false;
	}

	return true;
}

bool init_server_tests()
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		if (pE->GetDescription() == L"Failed to connect to network daemon")
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
