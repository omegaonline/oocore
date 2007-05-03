#define OMEGA_GUID_LINK_HERE

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

#include "../TestDll/testdll.h"
#include <OTL/OTL.h>

#include <stdio.h>
#include <conio.h>

static void printf_exception(const char* pszFunction, Omega::IException* pE)
{
	printf("Exception in %s: %s\nAt: %s.\n\nPress any key to continue...\n",pszFunction,(const char*)pE->Description(),(const char*)pE->Source());
	_getch();
	pE->Release();
}

static void output(const char* p)
{
	printf(p);
}

static void string_tests()
{
	const char sz1[] = "abcdef";
	const char sz1_1[] = "abcdef";
	const char sz1_2[] = "ABCDEF";
	const char sz2[] = "ghijk";
	
	Omega::string_t s1;
	if (!s1.IsEmpty())
		return output("string_t::string_t() or string_t::IsEmpty() failed.\n");

	s1 = sz1;
	if (s1.IsEmpty())
		return output("string_t::operator = (const char*) failed.\n");

	if (s1.Length() != sizeof(sz1)-1)
		return output("string_t::Length() failed.\n");

	if (s1 != sz1_1 || !(s1 == sz1_1))
		return output("string_t::operator == (const char*) failed.\n");

	if (s1.Compare(sz1_1) != 0)
		return output("string_t::Compare(const char*) failed.\n");

	Omega::string_t s2(sz2);
	if (s2 != sz2)
		return output("string_t::string_t(const char*) failed.\n");

	Omega::string_t s3(s1);
	if (s3 != sz1)
		return output("string_t::string_t(const string_t&) failed.\n");

	if (s3 != s1 || !(s3 == s1))
		return output("string_t::operator == (const string_t&) failed.\n");

	if (s3.Compare(s1) != 0)
		return output("string_t::Compare(const string_t&) failed.\n");

	s3 = s2;
	if (s3 != s2)
		return output("string_t::operator = (const string_t&) failed.\n");

	if (strcmp(s3,sz2) != 0)
		return output("string_t::operator (const char*) failed.\n");

	s3.Clear();
	if (!s3.IsEmpty())
		return output("string_t::Clear() failed.\n");

	if (s1.CompareNoCase(sz1_2) != 0)
		return output("string_t::CompareNoCase(const char*) failed.\n");

	s3 = sz1_2;
	if (s1.CompareNoCase(s3) != 0)
		return output("string_t::CompareNoCase(const string_t&) failed.\n");

	if (s1 != s3.ToLower())
		return output("string_t::ToLower() failed.\n");

	if (s1.ToUpper() != s3)
		return output("string_t::ToUpper() failed.\n");

	s1 = "abcdefghijabcdefghij";
	if (s1.Find('a') != 0)
		return output("string_t::Find(char,0,false) failed.\n");
	if (s1.Find('a',1) != 10)
		return output("string_t::Find(char,1,false) failed.\n");
	if (s1.Find('A',0,true) != 0)
		return output("string_t::Find(char,0,true) failed.\n");
	if (s1.Find('A',1,true) != 10)
		return output("string_t::Find(char,1,true) failed.\n");

	if (s1.ReverseFind('a') != 10)
		return output("string_t::ReverseFind(char,npos,false) failed.\n");
	if (s1.ReverseFind('a',10) != 0)
		return output("string_t::ReverseFind(char,10,false) failed.\n");
	if (s1.ReverseFind('A',Omega::string_t::npos,true) != 10)
		return output("string_t::ReverseFind(char,npos,true) failed.\n");
	if (s1.ReverseFind('A',10,true) != 0)
		return output("string_t::ReverseFind(char,10,true) failed.\n");

	s2 = sz1;
	if (s1.Find(s2) != 0)
		return output("string_t::Find(const string_t&,0,false) failed.\n");
	if (s1.Find(s2,1) != 10)
		return output("string_t::Find(const string_t&,1,false) failed.\n");
	if (s1.Find(s2.ToUpper(),0,true) != 0)
		return output("string_t::Find(const string_t&,0,true) failed.\n");
	if (s1.Find(s2.ToUpper(),1,true) != 10)
		return output("string_t::Find(const string_t&,1,true) failed.\n");

	if (Omega::string_t::Format("%s:%d","hello",1) != "hello:1")
		return output("string_t::Format() failed.\n");

	if (s1.Left(5) != "abcde")
		return output("string_t::Left() failed.\n");
	if (s1.Mid(15) != "fghij")
		return output("string_t::Mid(npos) failed.\n");
	if (s1.Mid(15,3) != "fgh")
		return output("string_t::Mid(a,b) failed.\n");
	if (s1.Right(3) != "hij")
		return output("string_t::Right() failed.\n");

	output("string_t tests passed!\n");
}

int main(int argc, char* argv[])
{
	try
	{
		string_tests();
	}
	catch(Omega::IException* pE)
	{
		printf_exception("string_tests",pE);
		return -1;
	}

	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		printf_exception("Omega::Initialize",pE);
		return -1;
	}
	
	try
	{
		OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
		OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");
	
		printf(ptrTest->Hello());
	}
	catch(Omega::IException* pE)
	{
		printf_exception("Test.Dll tests",pE);
		Omega::Uninitialize();
		return -1;
	}

	printf("Everything is cool - Press any key to continue...\n");
	_getch();

	Omega::Uninitialize();
	return 0;
}
