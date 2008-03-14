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
	TEST_VOID(pKey->SetIntegerValue(strTestValue,100));
	TEST(pKey->IsValue(strTestValue));
	TEST(pKey->GetValueType(strTestValue) == Omega::Registry::Integer);
	TEST(pKey->GetIntegerValue(strTestValue) == 100);
	TEST_VOID(pKey->SetIntegerValue(strTestValue,200));
	TEST(pKey->GetIntegerValue(strTestValue) == 200);
	TEST_VOID(pKey->SetValueDescription(strTestValue,L"A test description"));
	TEST(pKey->GetValueDescription(strTestValue) == L"A test description");
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
							"--------------------------------------------------------------------------------";

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

	try
	{
		pKey->GetStringValue(strTestValue);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		TEST(pE->GetName() == strTestValue);
		pE->Release();
	}

	TEST_VOID(pKey->SetIntegerValue(strTestValue,100));
	try
	{
		pKey->GetStringValue(strTestValue);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IWrongValueTypeException* pE)
	{
		TEST(pE->GetValueName() == strTestValue);
		TEST(pE->GetValueType() == Omega::Registry::Integer);

		pE->Release();
	}

	TEST_VOID(pKey->DeleteValue(strTestValue));
	
	try
	{
		pKey->SetStringValue(L"",L"Invalid name");
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "");
		pE->Release();
	}
	try
	{
		pKey->SetIntegerValue(L"\\",0);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "\\");
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

	Omega::Registry::IRegistryKey* pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::Create);
	TEST(pSubKey);
	TEST(pKey->IsSubKey(strTestKey));

	if (!test_values(pSubKey,strKey + "\\" + strTestKey))
		return false;

	TEST_VOID(pSubKey->SetDescription(L"A test description"));
	TEST(pSubKey->GetDescription() == L"A test description");

	pSubKey->Release();

	pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IRegistryKey::OpenExisting);
	TEST(pSubKey);
	pSubKey->Release();

	try
	{
		pSubKey = pKey->OpenSubKey(L"",Omega::Registry::IRegistryKey::Create);
		pSubKey->Release();
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "");
		pE->Release();
	}
	try
	{
		pSubKey = pKey->OpenSubKey(L"\\",Omega::Registry::IRegistryKey::Create);
		pSubKey->Release();
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "\\");
		pE->Release();
	}
	
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
		TEST(pE->GetKeyName() == strKey + "\\" + strTestKey);
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
		TEST(pE->GetName() == strKey + "\\" + strTestKey);
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

static bool test_privates(Omega::Registry::IRegistryKey* pKey, const Omega::string_t& strSubKey)
{
	try
	{
		pKey->DeleteKey(strSubKey);
		TEST(!"No exception thrown!");
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		TEST(pE->GetKeyName() == L"\\" + strSubKey);
		pE->Release();
	}

	return true;
}

static bool test_root_key(Omega::Registry::IRegistryKey* pKey)
{
	TEST(pKey->IsSubKey(L"All Users"));
	TEST(pKey->IsSubKey(L"Server"));
	TEST(pKey->IsSubKey(L"Server\\Sandbox"));
	TEST(pKey->IsSubKey(L"Local User"));

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

	// Test the private root keys
	test_privates(pKey,L"All Users");
	test_privates(pKey,L"Objects");
	test_privates(pKey,L"Objects\\OIDs");
	test_privates(pKey,L"Server");
	test_privates(pKey,L"Server\\Sandbox");

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

	if (!test_key(L"\\All Users"))
		return false;

	if (!test_key(L"\\Local User"))
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

	ptrKey = OTL::ObjectPtr<Omega::Registry::IRegistryKey>(L"\\");

	// Generate a unique value name
	Omega::string_t strTestKey = Omega::string_t::Format(L"TestKey_%lu",::GetCurrentProcessId());
	while (ptrKey->IsSubKey(strTestKey))
	{
		strTestKey = "_" + strTestKey;
	}

	Omega::string_t strXML =
		L"<?xml version=\"1.0\" ?>\r\n"
		L"<oo:root xmlns:oo=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">\r\n"
			L"<oo:key name=\"%TESTKEY%\">\r\n"
				L"<oo:key name=\"Testkey\" uninstall=\"Remove\"/>\r\n"
				L"<oo:key name=\"%MODULE%\">\r\n"
					L"<oo:value name=\"TestVal1\">Testing testing 1,2,3</oo:value>\r\n"
					L"<oo:value name=\"TestVal2\">%MODULE%</oo:value>\r\n"
					L"<oo:value name=\"TestVal3\" type=\"Integer\">  0x12345  </oo:value>\r\n"
					L"<oo:value name=\"TestVal4\" type=\"Integer\">  -12345  </oo:value>\r\n"
					L"<oo:value name=\"TestVal5\" type=\"Integer\" uninstall=\"Keep\">12345</oo:value>\r\n"
				L"</oo:key>\r\n"
			L"</oo:key>\r\n"
		L"</oo:root>\r\n";

	Omega::string_t strSubsts = L"  MODULE  =My Module;  TESTKEY=\\" + strTestKey;
	
	try
	{
		Omega::Registry::AddXML(strXML,true,strSubsts);
		TEST(ptrKey->IsSubKey(strTestKey + L"\\Testkey"));
		TEST(ptrKey->IsSubKey(strTestKey + L"\\My Module"));
		
		Omega::Registry::AddXML(strXML,false,strSubsts);
		TEST(!ptrKey->IsSubKey(strTestKey + L"\\Testkey"));
		TEST(ptrKey->IsSubKey(strTestKey + L"\\My Module"));

		if (ptrKey->IsSubKey(strTestKey))
			ptrKey->DeleteKey(strTestKey);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		// This is okay...
		pE->Release();
	}

	ptrKey = OTL::ObjectPtr<Omega::Registry::IRegistryKey>(L"\\Local User");
	// Generate a unique value name
	strTestKey = Omega::string_t::Format(L"TestKey_%lu",::GetCurrentProcessId());
	while (ptrKey->IsSubKey(strTestKey))
	{
		strTestKey = "_" + strTestKey;
	}

	strSubsts = L"  MODULE  =My Module;  TESTKEY=\\Local User\\" + strTestKey;

	Omega::Registry::AddXML(strXML,true,strSubsts);
	TEST(ptrKey->IsSubKey(strTestKey + L"\\Testkey"));
	TEST(ptrKey->IsSubKey(strTestKey + L"\\My Module"));

	Omega::Registry::AddXML(strXML,false,strSubsts);
	TEST(!ptrKey->IsSubKey(strTestKey + L"\\Testkey"));
	TEST(ptrKey->IsSubKey(strTestKey + L"\\My Module"));
	
	if (ptrKey->IsSubKey(strTestKey))
		ptrKey->DeleteKey(strTestKey);

	return true;
}
