#include "../include/Omega/Omega.h"
#include "Test.h"

#if !defined(_WIN32)
#define GetCurrentProcessId getpid
#endif

static bool test_values(Omega::Registry::IKey* pKey)
{
	// Generate a unique value name
	Omega::string_t strTestValue("TestValue_{0}");
	strTestValue %= GetCurrentProcessId();
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = "_" + strTestValue;
	}

	try
	{
		pKey->SetValue(strTestValue,"Yes");
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		// We have insufficient permissions to write here
		pE->Release();
		return true;
	}

	TEST_VOID(pKey->SetValue(strTestValue,"Yes"));
	TEST(pKey->IsValue(strTestValue));
	TEST(pKey->GetValue(strTestValue) ==  "Yes");
	TEST_VOID(pKey->SetValue(strTestValue,"No"));
	TEST(pKey->GetValue(strTestValue) ==  "No");
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	TEST_VOID(pKey->SetValue(strTestValue,"Yes"));
	TEST_VOID(pKey->SetValueDescription(strTestValue,"A test description"));
	TEST(pKey->GetValueDescription(strTestValue) == "A test description");
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
		pKey->SetValue("","Invalid name");
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "");
		pE->Release();
	}
	try
	{
		pKey->SetValue("/",0);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "/");
		pE->Release();
	}

	TEST_VOID(pKey->SetValue(strTestValue,"Yes"));

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

	Omega::string_t strTestKey("TestKey_{0}");
	strTestKey %= GetCurrentProcessId();
	while (pKey->IsSubKey(strTestKey))
	{
		strTestKey = "_" + strTestKey;
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

	TEST_VOID(pSubKey->SetDescription("A test description"));
	TEST(pSubKey->GetDescription() == "A test description");

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
		pSubKey = pKey->OpenSubKey("/",Omega::Registry::IKey::OpenCreate);
		pSubKey->Release();
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		TEST(pE->GetName() == "/");
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
			TEST(pE->GetKeyName() == strKey + "/" + strTestKey);
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
			TEST(pE->GetName() == strKey + "/" + strTestKey);
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
	TEST(pKey->IsSubKey("System"));
	TEST(pKey->IsSubKey("Local User"));
	TEST(pKey->IsSubKey("All Users"));

	Omega::string_t strTestValue("TestValue_{0}");
	strTestValue %= GetCurrentProcessId();
	while (pKey->IsValue(strTestValue))
	{
		strTestValue = "_" + strTestValue;
	}

	bool bCanWriteToRoot = true;
	try
	{
		TEST_VOID(pKey->SetValue(strTestValue,"Yes"));
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

		if (!test_key2(pKey,""))
			return false;
	}

	// Test the private root keys
	test_privates(pKey,"System");
	test_privates(pKey,"System/Sandbox");
	test_privates(pKey,"All Users/Objects");
	test_privates(pKey,"All Users/Objects/OIDs");
	test_privates(pKey,"All Users/Applications");
	test_privates(pKey,"Local User");

	if (bCanWriteToRoot)
		test_privates(pKey,"System");

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
	pObj = Omega::CreateInstance("Omega.Registry",Omega::Activation::Default,NULL,OMEGA_GUIDOF(Omega::Registry::IKey));
	TEST(pObj);
	pObj->Release();

	if (!test_key("System"))
		return false;

	if (!test_key("Local User"))
		return false;

	return true;
}
