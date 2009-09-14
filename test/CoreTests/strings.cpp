#include <OOCore/Remoting.h>
#include "Test.h"

static bool string_tests_wchar()
{
	const wchar_t sz1[] = L"abcdef";
	const wchar_t sz1_1[] = L"abcdef";
	const wchar_t sz1_2[] = L"ABCDEF";

	Omega::string_t s1;
	TEST(s1.IsEmpty());

	s1 = sz1;
	TEST(!s1.IsEmpty());
	TEST(s1.Length() == 6);
	TEST(s1 == sz1_1 && !(s1 != sz1_1));
	TEST(s1.Compare(sz1_1) == 0);
	TEST(s1.Compare(sz1_2,0,Omega::string_t::npos,true) == 0);

	const wchar_t sz2[] = L"ghijk";
	Omega::string_t s2(sz2);
	TEST(s2 == sz2);

	Omega::string_t s3(s1);
	TEST(s3 == sz1);

	TEST(s3 == s1 && !(s3 != s1));
	TEST(s3.Compare(s1) == 0);

	s3 = s2;
	TEST(s3 == s2);
	TEST(wcscmp(s3.c_str(),sz2) == 0);

	s3.Clear();
	TEST(s3.IsEmpty());

	s3 = sz1_2;
	TEST(s1.Compare(s3,0,Omega::string_t::npos,true) == 0);
	TEST(s1 == s3.ToLower());
	TEST(s1.ToUpper() == s3);

	s1 = L"abcdefghijabcdefghij";
	TEST(s1.Find(L'a') == 0);
	TEST(s1.Find(L'a',1) == 10);
	TEST(s1.Find(L'A',0,true) == 0);
	TEST(s1.Find(L'A',1,true) == 10);

	TEST(s1.FindNot(L'a') == 1);
	TEST(s1.FindNot(L'a',1) == 1);
	TEST(s1.FindNot(L'A',0,true) == 1);
	TEST(s1.FindNot(L'A',1,true) == 1);

	TEST(s1.FindOneOf(L"edf") == 3);
	
	TEST(s1.ReverseFind(L'a') == 10);
	TEST(s1.ReverseFind(L'a',9) == 0);
	TEST(s1.ReverseFind(L'A',Omega::string_t::npos,true) == 10);
	TEST(s1.ReverseFind(L'A',9,true) == 0);

	s2 = sz1;
	TEST(s1.Find(s2) == 0);
	TEST(s1.Find(s2,1) == 10);
	TEST(s1.Find(s2.ToUpper(),0,true) == 0);
	TEST(s1.Find(s2.ToUpper(),1,true) == 10);

	TEST(s1.Left(5) == L"abcde");
	TEST(s1.Mid(15) == L"fghij");
	TEST(s1.Mid(15,3) == L"fgh");
	TEST(s1.Right(3) == L"hij");

	Omega::string_t s4;
	TEST(s4 == L"");
	TEST(s4 == (wchar_t*)0);
	TEST(s4 == Omega::string_t(L""));
	TEST(s4 == Omega::string_t("",false));

	TEST(Omega::string_t(L"1111H").TrimLeft(L'1') == L"H");
	TEST(Omega::string_t(L"1111").TrimLeft(L'1').IsEmpty());
	TEST(Omega::string_t(L"123321H").TrimLeft(L"123") == L"H");
	TEST(Omega::string_t(L"123321").TrimLeft(L"123").IsEmpty());

	TEST(Omega::string_t(L"H1111").TrimRight(L'1') == L"H");
	TEST(Omega::string_t(L"1111").TrimRight(L'1').IsEmpty());
	TEST(Omega::string_t(L"H123321").TrimRight(L"123") == L"H");
	TEST(Omega::string_t(L"123321").TrimRight(L"123").IsEmpty());

	return true;
}

static bool string_tests_format()
{
	TEST(Omega::string_t::Format(L"%ls:%d",L"hello",1) == L"hello:1");
	TEST(Omega::string_t::Format(L"%hs:%d","hello",1) == L"hello:1");

	return true;
}

static bool string_tests_utf8()
{
/*	This test is automated...

	FILE* pIn = fopen("UTF-8-test.txt","rb");
	if (!pIn)
		return false;

	// Loop reading and converting...
	Omega::string_t str;
	for (;;)
	{
		char szBuf[129];
		if (!fgets(szBuf,128,pIn))
			break;

		szBuf[128] = '\0';
		str += Omega::string_t(szBuf,true);
	}

	fclose(pIn);

	FILE* pOut = fopen("UTF-8-results.txt","w+b");
	if (!pOut)
		return false;

	fwrite(L"\xFEFF",sizeof(wchar_t),1,pOut);
	fwrite(str.c_str(),sizeof(wchar_t),str.Length(),pOut);

	fclose(pOut);

	pOut = fopen("UTF-8-results2.txt","w+b");
	if (!pOut)
		return false;

	std::string str2 = str.ToUTF8();
	fwrite(str2.c_str(),sizeof(char),str2.length(),pOut);

	fclose(pOut); */

	return true;
}

bool string_tests()
{
	if (!string_tests_wchar())
		return false;

	if (!string_tests_format())
		return false;

	if (!string_tests_utf8())
		return false;

	return true;
}

bool guid_tests()
{
	Omega::guid_t guid(Omega::guid_t::Null());
	TEST(guid == Omega::guid_t::Null());

	const wchar_t sz[] = L"{BCB02DAE-998A-4fc1-AB91-39290C237A37}";

	Omega::guid_t guid2 = Omega::guid_t::FromString(sz);
	TEST(guid2 != guid);
	TEST(guid2 != Omega::guid_t::Null());
	
	TEST(Omega::guid_t::FromString(sz,guid2));

	// Create a load of unique guid_t's
	Omega::guid_t arr[100];
	for (size_t i=0;i<sizeof(arr)/sizeof(arr[0]);++i)
		arr[i] = Omega::guid_t::Create();

	// Make sure they are unique!
	bool bTest = true;
	for (size_t j=0;j<sizeof(arr)/sizeof(arr[0]) && bTest;++j)
	{
		for (size_t k=0;k<sizeof(arr)/sizeof(arr[0]) && bTest;++k)
		{
			if (j != k)
				bTest = (arr[j] != arr[k]);
		}
	}
	TEST(bTest);

	// Check to see if we can export OID's properly
	TEST(Omega::Remoting::OID_StdObjectManager == Omega::guid_t::FromString(L"{63EB243E-6AE3-43bd-B073-764E096775F8}"));

	// Check whether OMEGA_GUIDOF works...
	TEST(OMEGA_GUIDOF(Omega::IObject) == OMEGA_GUIDOF(Omega::IObject*));

	return true;
}
