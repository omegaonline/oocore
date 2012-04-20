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
	catch (Omega::IAccessDeniedException* pE)
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
	TEST_VOID(pKey->DeleteValue(strTestValue));
	TEST(!pKey->IsValue(strTestValue));

	try
	{
		pKey->GetValue(strTestValue);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::INotFoundException* pE)
	{
		pE->Release();
	}

	TEST_VOID(pKey->DeleteValue(strTestValue));

	try
	{
		pKey->SetValue("","Invalid name");
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::IAccessDeniedException* pE)
	{
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
	while (pKey->IsKey(strTestKey))
	{
		strTestKey = "_" + strTestKey;
	}

	Omega::Registry::IKey* pSubKey;
	try
	{
		pSubKey = pKey->OpenKey(strTestKey,Omega::Registry::IKey::OpenCreate);
	}
	catch (Omega::IAccessDeniedException* pE)
	{
		// We have insufficient permissions to write here
		pE->Release();
		return true;
	}

	TEST(pSubKey);
	TEST(pKey->IsKey(strTestKey));

	if (!test_values(pSubKey))
		return false;

	pSubKey->Release();

	pSubKey = pKey->OpenKey(strTestKey,Omega::Registry::IKey::OpenExisting);
	TEST(pSubKey);
	pSubKey->Release();

	std::set<Omega::string_t> keys = pKey->EnumSubKeys();
	TEST(!keys.empty());
	TEST(keys.find(strTestKey) != keys.end());

	try
	{
		pKey->OpenKey(strTestKey,Omega::Registry::IKey::CreateNew);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::IAlreadyExistsException* pE)
	{
		pE->Release();
	}
	catch (Omega::IException* pE)
	{
		output_exception(pE);
		pE->Release();
	}

	TEST_VOID(pKey->DeleteSubKey(strTestKey));
	TEST(!pKey->IsKey(strTestKey));

	try
	{
		pKey->OpenKey(strTestKey,Omega::Registry::IKey::OpenExisting);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::INotFoundException* pE)
	{
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
	
	Omega::Registry::IKey* pKey = pRootKey->OpenKey(strKey);
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
		pKey->DeleteSubKey(strSubKey);
		TEST_FAIL("No exception thrown!");
	}
	catch (Omega::IAccessDeniedException* pE)
	{
		pE->Release();
	}

	return true;
}

static bool test_root_key(Omega::Registry::IKey* pKey)
{
	TEST(pKey->IsKey("System"));
	TEST(pKey->IsKey("Local User"));
	TEST(pKey->IsKey("All Users"));

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
	catch (Omega::IAccessDeniedException* pE)
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
