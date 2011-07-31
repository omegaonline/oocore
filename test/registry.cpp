#include "../include/Omega/Omega.h"
#include "Test.h"

#if !defined(_WIN32)
#define GetCurrentProcessId getpid
#endif

static bool test_values(Omega::Registry::IKey* pKey)
{
	// Generate a unique value name
	Omega::string_t strTestValue(L"TestValue_{0}");
	strTestValue %= GetCurrentProcessId();
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = L"_" + strTestValue;
	}

	try
	{
		pKey->SetValue(strTestValue,L"Yes");
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		// We have insufficient permissions to write here
		pE->Release();
		return true;
	}

	TEST_VOID(pKey->SetValue(strTestValue,L"Yes"));
	TEST(pKey->IsValue(strTestValue));
	TEST(pKey->GetValue(strTestValue) ==  L"Yes");
	TEST_VOID(pKey->SetValue(strTestValue,L"No"));
	TEST(pKey->GetValue(strTestValue) ==  L"No");
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	TEST_VOID(pKey->SetValue(strTestValue,L"Yes"));
	TEST_VOID(pKey->SetValueDescription(strTestValue,L"A test description"));
	TEST(pKey->GetValueDescription(strTestValue) == L"A test description");
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	try
	{
		pKey->GetValue(strTestValue);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		TEST(pE->GetName() == strTestValue);
		pE->Release();
	}

	TEST_VOID(pKey->DeleteValue(strTestValue));

	try
	{
		pKey->SetValue(L"",L"Invalid name");
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == L"");
		pE->Release();
	}
	try
	{
		pKey->SetValue(L"/",0);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == L"/");
		pE->Release();
	}

	TEST_VOID(pKey->SetValue(strTestValue,L"Yes"));

	std::set<Omega::string_t> values = pKey->EnumValues();

	TEST(!values.empty());
	TEST(values.find(strTestValue) != values.end());

	TEST_VOID(pKey->DeleteValue(strTestValue));

	values = pKey->EnumValues();
	TEST(values.find(strTestValue) == values.end());

	return true;
}

static bool test_key2(Omega::Registry::IKey* pKey, const Omega::string_t& strKey)
{
	if (!test_values(pKey))
		return false;

	Omega::string_t strTestKey(L"TestKey_{0}");
	strTestKey %= GetCurrentProcessId();
	while (pKey->IsSubKey(strTestKey))
	{
		strTestKey = L"_" + strTestKey;
	}

	Omega::Registry::IKey* pSubKey;
	try
	{
		pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IKey::OpenCreate);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		// We have insufficient permissions to write here
		pE->Release();
		return true;
	}

	TEST(pSubKey);
	TEST(pKey->IsSubKey(strTestKey));

	if (!test_values(pSubKey))
		return false;

	TEST_VOID(pSubKey->SetDescription(L"A test description"));
	TEST(pSubKey->GetDescription() == L"A test description");

	pSubKey->Release();

	pSubKey = pKey->OpenSubKey(strTestKey,Omega::Registry::IKey::OpenExisting);
	TEST(pSubKey);
	pSubKey->Release();

	try
	{
		pSubKey = pKey->OpenSubKey(Omega::string_t(),Omega::Registry::IKey::OpenCreate);
		pSubKey->Release();
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName().IsEmpty());
		pE->Release();
	}
	try
	{
		pSubKey = pKey->OpenSubKey(L"/",Omega::Registry::IKey::OpenCreate);
		pSubKey->Release();
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == L"/");
		pE->Release();
	}

	std::set<Omega::string_t> keys = pKey->EnumSubKeys();
	TEST(!keys.empty());
	TEST(keys.find(strTestKey) != keys.end());

	try
	{
		pKey->OpenSubKey(strTestKey,Omega::Registry::IKey::CreateNew);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IAlreadyExistsException* pE)
	{
		if (!strKey.IsEmpty())
			TEST(pE->GetKeyName() == strKey + L"/" + strTestKey);
		else
			TEST(pE->GetKeyName() == strTestKey);
		
		pE->Release();
	}

	TEST_VOID(pKey->DeleteKey(strTestKey));
	TEST(!pKey->IsSubKey(strTestKey));

	try
	{
		pKey->OpenSubKey(strTestKey,Omega::Registry::IKey::OpenExisting);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::INotFoundException* pE)
	{
		if (!strKey.IsEmpty())
			TEST(pE->GetName() == strKey + L"/" + strTestKey);
		else
			TEST(pE->GetName() == strTestKey);
		
		pE->Release();
	}

	keys = pKey->EnumSubKeys();
	TEST(keys.find(strTestKey) == keys.end());

	return true;
}

static bool test_key(const Omega::string_t& strKey)
{
	Omega::IObject* pObj = Omega::CreateInstance(Omega::Registry::OID_Registry,Omega::Activation::Default,NULL,OMEGA_GUIDOF(Omega::Registry::IKey));
	TEST(pObj);
	
	Omega::Registry::IKey* pRootKey = static_cast<Omega::Registry::IKey*>(pObj);
	pObj = NULL;
	
	Omega::Registry::IKey* pKey = pRootKey->OpenSubKey(strKey);
	pRootKey->Release();
	TEST(pKey);

	bool bTest;
	try
	{
		bTest = test_key2(pKey,strKey);
	}
	catch (...)
	{
		pKey->Release();
		throw;
	}
	pKey->Release();
	if (!bTest)
		return false;

	return true;
}

static bool test_privates(Omega::Registry::IKey* pKey, const Omega::string_t& strSubKey)
{
	try
	{
		pKey->DeleteKey(strSubKey);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		TEST(pE->GetKeyName() == strSubKey);
		pE->Release();
	}

	return true;
}

static bool test_root_key(Omega::Registry::IKey* pKey)
{
	TEST(pKey->IsSubKey(L"System"));
	TEST(pKey->IsSubKey(L"Local User"));
	TEST(pKey->IsSubKey(L"All Users"));

	Omega::string_t strTestValue(L"TestValue_{0}");
	strTestValue %= GetCurrentProcessId();
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = L"_" + strTestValue;
	}

	bool bCanWriteToRoot = true;
	try
	{
		TEST_VOID(pKey->SetValue(strTestValue,L"Yes"));
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

		if (!test_key2(pKey,L""))
			return false;
	}

	// Test the private root keys
	test_privates(pKey,L"System");
	test_privates(pKey,L"System/Sandbox");
	test_privates(pKey,L"All Users/Objects");
	test_privates(pKey,L"All Users/Objects/OIDs");
	test_privates(pKey,L"All Users/Applications");
	test_privates(pKey,L"Local User");

	if (bCanWriteToRoot)
		test_privates(pKey,L"System");

	return true;
}

bool registry_tests()
{
	Omega::IObject* pObj = Omega::CreateInstance(Omega::Registry::OID_Registry,Omega::Activation::Default,NULL,OMEGA_GUIDOF(Omega::Registry::IKey));
	TEST(pObj);
	
	Omega::Registry::IKey* pRootKey = static_cast<Omega::Registry::IKey*>(pObj);
	pObj = NULL;
	
	bool bTest;
	try
	{
		bTest = test_root_key(pRootKey);
	}
	catch (...)
	{
		pRootKey->Release();
		throw;
	}
	pRootKey->Release();
	if (!bTest)
		return false;
		
	// Check we can use the textual OID
	pObj = Omega::CreateInstance(L"Omega.Registry",Omega::Activation::Default,NULL,OMEGA_GUIDOF(Omega::Registry::IKey));
	TEST(pObj);
	pObj->Release();

	if (!test_key(L"System"))
		return false;

	if (!test_key(L"Local User"))
		return false;

	return true;
}
