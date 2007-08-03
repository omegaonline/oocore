#include <OOCore/OOCore.h>
#include "Test.h"

static bool test_values(Omega::Registry::IRegistryKey* pKey, const Omega::string_t& strKey)
{
	// Generate a unique value name
	Omega::string_t strTestValue = Omega::string_t::Format(L"TestValue_%lu",::GetCurrentProcessId());
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = "_" + strTestValue;
	}

	TEST_VOID(pKey->SetStringValue(strTestValue,L"Yes"));
	TEST(pKey->GetValueType(strTestValue) == Omega::Registry::String);
	TEST(pKey->IsValue(strTestValue));
	TEST(pKey->GetStringValue(strTestValue) ==  L"Yes");
	TEST_VOID(pKey->SetStringValue(strTestValue,L"No"));
	TEST(pKey->GetStringValue(strTestValue) ==  L"No");
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));
	TEST_VOID(pKey->SetUIntValue(strTestValue,100));
	TEST(pKey->IsValue(strTestValue));
	TEST(pKey->GetValueType(strTestValue) == Omega::Registry::UInt32);
	TEST(pKey->GetUIntValue(strTestValue) == 100);
	TEST_VOID(pKey->SetUIntValue(strTestValue,200));
	TEST(pKey->GetUIntValue(strTestValue) == 200);
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	const Omega::byte_t szBuf[] = "Welcome to the project site for Omega Online ¶"
							"Omega Online is a massively-multiplayer online roleplaying game, based on the successful live-roleplaying system from Omega LRP. "
							"The goals of the project are to produce a game that is: "
							"Free to play -- No fees and no subscriptions "
							"Cross-platform -- Initially targetting Windows, MacOSX, and Unix (including Linux and FreeBSD) "
							"Peer-to-peer -- Using serverless distributed processing "
							"Open Source -- All the source-code will be released under an applicable open source license "
							"The project is very much in its infancy, despite having been in development for more than two years, but progress is being made! "
							"Starting points ¶"
							"GameBackground? -- The background of the game "
							"GameDesign -- An overview of the design of the game "
							"TechOverview -- An overview of the technologies behind the game "
							"For a complete list of pages, see TitleIndex. "
							"Download in other formats:"
							"Plain Text"
							"--------------------------------------------------------------------------------"
							" Powered by Trac 0.10.3"
							"By Edgewall Software. ";

	const Omega::uint32_t orig_size = sizeof(szBuf);
	TEST_VOID(pKey->SetBinaryValue(strTestValue,orig_size,szBuf));
	TEST(pKey->GetValueType(strTestValue) == Omega::Registry::Binary);
	TEST(pKey->IsValue(strTestValue));

	const Omega::uint32_t new_size = orig_size * 3 / 2;
	Omega::byte_t szBuf2[new_size] = {0};
	Omega::uint32_t size = new_size;
	TEST_VOID(pKey->GetBinaryValue(strTestValue,size,szBuf2));
	TEST(size == orig_size);

	const Omega::uint32_t new_size2 = orig_size / 2;
	Omega::byte_t szBuf3[new_size2] = {0};
	size = new_size2;
	TEST_VOID(pKey->GetBinaryValue(strTestValue,size,szBuf3));
	TEST(size == new_size2);

	size = 0;
	TEST_VOID(pKey->GetBinaryValue(strTestValue,size,NULL));
	TEST(size == orig_size);
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	Omega::string_t strFQKeyName = strKey + "\\" + strTestValue;
	if (strFQKeyName.Left(1) == "\\")
		strFQKeyName = strFQKeyName.Mid(1);

	try
	{
		pKey->GetStringValue(strTestValue);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		TEST(pE->GetName() == strFQKeyName);
		pE->Release();
	}

	TEST_VOID(pKey->SetUIntValue(strTestValue,100));
	try
	{
		pKey->GetStringValue(strTestValue);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IWrongValueTypeException* pE)
	{
		TEST(pE->GetValueName() == strFQKeyName);
		TEST(pE->GetValueType() == Omega::Registry::UInt32);

		pE->Release();
	}

	TEST_VOID(pKey->DeleteValue(strTestValue));
	try
	{
		pKey->DeleteValue(strTestValue);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		TEST(pE->GetName() == strFQKeyName);
		pE->Release();
	}

	try
	{
		pKey->SetStringValue(L"\\",L"Invalid name");
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "\\");
		pE->Release();
	}
	try
	{
		pKey->SetStringValue(L"[",L"Invalid name");
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "[");
		pE->Release();
	}
	try
	{
		pKey->SetStringValue(L"]",L"Invalid name");
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "]");
		pE->Release();
	}

	TEST_VOID(pKey->SetStringValue(strTestValue,L"Yes"));

	Omega::IEnumString* pValues = pKey->EnumValues();
	TEST(pValues);

	Omega::uint32_t nFound = 0;
	Omega::bool_t bMore;
	do
	{
		Omega::string_t vals[10];
		Omega::uint32_t count = 10;

		bMore = pValues->Next(count,vals);

		for (Omega::uint32_t i=0;i<count;++i)
		{
			if (vals[i] == strTestValue)
				++nFound;
		}
	} while (bMore);
	pValues->Release();
	TEST(nFound == 1);

	TEST_VOID(pKey->DeleteValue(strTestValue));

	pValues = pKey->EnumValues();
	TEST(pValues);
	nFound = 0;
	do
	{
		Omega::string_t vals[10];
		Omega::uint32_t count = 10;

		bMore = pValues->Next(count,vals);

		for (Omega::uint32_t i=0;i<count;++i)
		{
			if (vals[i] == strTestValue)
				++nFound;
		}
	} while (bMore);
	pValues->Release();
	TEST(nFound == 0);

	return true;
}

static bool test_key2(Omega::Registry::IRegistryKey* pKey, const Omega::string_t& strKey)
{
	if (!test_values(pKey,strKey))
		return false;

	Omega::string_t strTestKey = Omega::string_t::Format(L"TestKey_%lu",::GetCurrentProcessId());
	while (pKey->IsSubKey(strTestKey))
	{
		strTestKey = "_" + strTestKey;
	}

	Omega::string_t strFQKeyName = strKey + "\\" + strTestKey;
	if (strFQKeyName.Left(1) == "\\")
		strFQKeyName = strFQKeyName.Mid(1);

	Omega::Registry::IRegistryKey* pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::Create);
	TEST(pSubKey);
	TEST(pKey->IsSubKey(strTestKey));

	if (!test_values(pSubKey,strFQKeyName))
		return false;

	pSubKey->Release();

	pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::OpenExisting);
	TEST(pSubKey);
	pSubKey->Release();

	Omega::IEnumString* pKeys = pKey->EnumSubKeys();
	TEST(pKeys);

	Omega::uint32_t nFound = 0;
	Omega::bool_t bMore;
	do
	{
		Omega::string_t keys[10];
		Omega::uint32_t count = 10;

		bMore = pKeys->Next(count,keys);

		for (Omega::uint32_t i=0;i<count;++i)
		{
			if (keys[i] == strTestKey)
				++nFound;
		}
	} while (bMore);
	pKeys->Release();
	TEST(nFound == 1);

	try
	{
		pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::Create | Omega::Registry::IRegistryKey::FailIfThere);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IAlreadyExistsException* pE)
	{
		TEST(pE->GetKeyName() == strFQKeyName);
		pE->Release();
	}

	TEST_VOID(pKey->DeleteKey(strTestKey));
	TEST(!pKey->IsSubKey(strTestKey));

	try
	{
		pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::OpenExisting);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		TEST(pE->GetName() == strFQKeyName);
		pE->Release();
	}

	pKeys = pKey->EnumSubKeys();
	TEST(pKeys);
	nFound = 0;
	do
	{
		Omega::string_t keys[10];
		Omega::uint32_t count = 10;

		bMore = pKeys->Next(count,keys);

		for (Omega::uint32_t i=0;i<count;++i)
		{
			if (keys[i] == strTestKey)
				++nFound;
		}
	} while (bMore);
	pKeys->Release();
	TEST(nFound == 0);

	return true;
}

static bool test_key(const Omega::string_t& strKey)
{
	Omega::Registry::IRegistryKey* pKey = Omega::Registry::IRegistryKey::OpenKey(strKey);
	TEST(pKey);

	bool bTest;
	try
	{
		bTest = test_key2(pKey,strKey);
	}
	catch(...)
	{
		pKey->Release();
		throw;
	}
	pKey->Release();
	if (!bTest)
		return false;

	return true;
}

static bool test_root_key(Omega::Registry::IRegistryKey* pKey)
{
	TEST(pKey->IsSubKey(L"All Users"));
	TEST(pKey->IsSubKey(L"Server"));
	TEST(pKey->IsSubKey(L"Server\\Sandbox"));
	TEST(pKey->IsSubKey(L"Current User"));

	Omega::string_t strTestValue = Omega::string_t::Format(L"TestValue_%lu",::GetCurrentProcessId());
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = L"_" + strTestValue;
	}

	bool bCanWriteToRoot = true;
	try
	{
		TEST_VOID(pKey->SetStringValue(strTestValue,L"Yes"));
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		// We don't have permission - skip this test...
		pE->Release();
		bCanWriteToRoot = false;
	}

	if (bCanWriteToRoot)
	{
		TEST_VOID(pKey->DeleteValue(strTestValue));

		if (!test_values(pKey,L""))
			return false;
	}

	return true;
}

bool registry_tests()
{
	Omega::Registry::IRegistryKey* pRootKey = Omega::Registry::IRegistryKey::OpenKey(L"\\");
	TEST(pRootKey);

	bool bTest;
	try
	{
		bTest = test_root_key(pRootKey);
	}
	catch(...)
	{
		pRootKey->Release();
		throw;
	}
	pRootKey->Release();
	if (!bTest)
		return false;

	if (!test_key(L"All Users"))
		return false;

	if (!test_key(L"Current User"))
		return false;

	return true;
}

#include <OTL/OTL.h>

bool registry_tests_2()
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey;
	TEST(!ptrKey);
	TEST(ptrKey == (Omega::IObject*)0);
	TEST(ptrKey == (Omega::Registry::IRegistryKey*)0);



	return true;
}
