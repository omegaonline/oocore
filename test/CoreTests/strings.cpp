#include <OOCore/Remoting.h>
#include "Test.h"

bool string_tests_wchar()
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
	TEST(s1.CompareNoCase(sz1_2) == 0);

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
	TEST(s3.IsEmpty())
	
	s3 = sz1_2;
	TEST(s1.CompareNoCase(s3) == 0);
	TEST(s1 == s3.ToLower());
	TEST(s1.ToUpper() == s3);

	s1 = L"abcdefghijabcdefghij";
	TEST(s1.Find(L'a') == 0);
	TEST(s1.Find(L'a',1) == 10);
	TEST(s1.Find(L'A',0,true) == 0);
	TEST(s1.Find(L'A',1,true) == 10);

	TEST(s1.ReverseFind(L'a') == 10);
	TEST(s1.ReverseFind(L'a',10) == 0);
	TEST(s1.ReverseFind(L'A',Omega::string_t::npos,true) == 10);
	TEST(s1.ReverseFind(L'A',10,true) == 0);

	s2 = sz1;
	TEST(s1.Find(s2) == 0);
	TEST(s1.Find(s2,1) == 10);
	TEST(s1.Find(s2.ToUpper(),0,true) == 0);
	TEST(s1.Find(s2.ToUpper(),1,true) == 10);

	// Some more here maybe?
	TEST(Omega::string_t::Format(L"%ls:%d",L"hello",1) == L"hello:1");

	TEST(s1.Left(5) == L"abcde");
	TEST(s1.Mid(15) == L"fghij");
	TEST(s1.Mid(15,3) == L"fgh");
	TEST(s1.Right(3) == L"hij");

	return true;
}

bool string_tests_char()
{
	const char sz1[] = "abcdef";
	const char sz1_1[] = "abcdef";
	const char sz1_2[] = "ABCDEF";

	Omega::string_t s1(sz1,false);
	TEST(!s1.IsEmpty());
	TEST(s1.Length() == 6);
	TEST(s1 == sz1_1 && !(s1 != sz1_1));
	TEST(s1.Compare(sz1_1) == 0);
	TEST(s1.CompareNoCase(sz1_2) == 0);

	const char sz2[] = "ghijk";
	Omega::string_t s2(sz2,true);
	TEST(s2 == sz2);

	Omega::string_t s3(s1);
	TEST(s3 == sz1);
	
	s3 = sz1_2;
	TEST(s1.CompareNoCase(s3) == 0);
	TEST(s1 == s3.ToLower());
	TEST(s1.ToUpper() == s3);

	s1 = "abcdefghijabcdefghij";
	TEST(s1.Find('a') == 0);
	TEST(s1.Find('a',1) == 10);
	TEST(s1.Find('A',0,true) == 0);
	TEST(s1.Find('A',1,true) == 10);

	TEST(s1.ReverseFind('a') == 10);
	TEST(s1.ReverseFind('a',10) == 0);
	TEST(s1.ReverseFind('A',Omega::string_t::npos,true) == 10);
	TEST(s1.ReverseFind('A',10,true) == 0);

	s2 = sz1;
	TEST(s1.Find(s2) == 0);
	TEST(s1.Find(s2,1) == 10);
	TEST(s1.Find(s2.ToUpper(),0,true) == 0);
	TEST(s1.Find(s2.ToUpper(),1,true) == 10);

	// Some more here maybe?
	TEST(Omega::string_t::Format(L"%hs:%d","hello",1) == "hello:1");

	TEST(s1.Left(5) == "abcde");
	TEST(s1.Mid(15) == "fghij");
	TEST(s1.Mid(15,3) == "fgh");
	TEST(s1.Right(3) == "hij");

	return true;
}

bool string_tests()
{
	TEST(string_tests_wchar());
	TEST(string_tests_char());

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
	TEST(guid2 == sz);

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
	TEST(Omega::Remoting::OID_StdObjectManager == L"{63EB243E-6AE3-43bd-B073-764E096775F8}");

	// Check whether OMEGA_UUIDOF works...
	TEST(OMEGA_UUIDOF(Omega::IObject) == OMEGA_UUIDOF(Omega::IObject*));

	return true;
}
