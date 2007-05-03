#include <OOCore/OOCore.h>

#include "Test.h"

bool string_tests()
{
	const char sz1[] = "abcdef";
	const char sz1_1[] = "abcdef";
	const char sz1_2[] = "ABCDEF";
		
	Omega::string_t s1;
	TEST(s1.IsEmpty());
		
	s1 = sz1;
	TEST(!s1.IsEmpty());
	TEST(s1.Length() == sizeof(sz1)-1);
	TEST(s1 == sz1_1 && !(s1 != sz1_1));
	TEST(s1.Compare(sz1_1) == 0);
	
	const char sz2[] = "ghijk";
	Omega::string_t s2(sz2);
	TEST(s2 == sz2);
		
	Omega::string_t s3(s1);
	TEST(s3 == sz1);
		
	TEST(s3 == s1 && !(s3 != s1));
	TEST(s3.Compare(s1) == 0);

	throw 2;
		
	s3 = s2;
	TEST(s3 == s2);
	TEST(strcmp(s3,sz2) == 0);
		
	s3.Clear();
	TEST(s3.IsEmpty())
	TEST(s1.CompareNoCase(sz1_2) == 0);
		
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
	TEST(Omega::string_t::Format("%s:%d","hello",1) == "hello:1");

	TEST(s1.Left(5) == "abcde");
	TEST(s1.Mid(15) == "fghij");
	TEST(s1.Mid(15,3) == "fgh");
	TEST(s1.Right(3) == "hij");

	return true;
}