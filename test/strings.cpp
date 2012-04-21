#include "../include/Omega/Remoting.h"
#include "Test.h"

#include <string>

bool string_tests()
{
	const char sz1[] = "abcdef";
	const char sz1_1[] = "abcdef";
	
	Omega::string_t s1;
	TEST(s1.IsEmpty());
	TEST(!s1);

	s1 = sz1;
	TEST(!s1.IsEmpty());
	TEST(s1.Length() == 6);

	TEST(s1 == sz1_1 && !(s1 != sz1_1));
	TEST(s1.Compare(sz1_1) == 0);
	TEST(s1.Compare("ABCDEF") != 0);

	const char sz2[] = "ghijk";
	Omega::string_t s2("ghijk");
	TEST(s2 == sz2);
	TEST(s2 != "ghi");
	TEST(s2 != "ghijklmno");

	TEST(s2.Length() == 5);
	TEST((s1 + s2).Length() == 11);

	Omega::string_t s3(s1);
	TEST(s3 == sz1);

	TEST(s3 == s1 && !(s3 != s1));
	TEST(s3.Compare(s1) == 0);

	s3 = s2;
	TEST(s3 == s2);
	TEST(strcmp(s3.c_str(),sz2) == 0);

	s3.Clear();
	TEST(s3.IsEmpty());

	s1 = "abcdefghijabcdefghij";
	TEST(s1.Find('a') == 0);
	TEST(s1.Find('a',1) == 10);

	TEST(s1.FindNot('a') == 1);
	TEST(s1.FindNot('a',10) == 11);

	TEST(s1.FindOneOf("edf") == 3);
	TEST(s1.FindOneOf("edf",10) == 13);

	TEST(s1.ReverseFind('a') == 10);
	TEST(s1.ReverseFind('a',9) == 0);

	TEST(s1.FindNotOf("abcde",0) == 5);
	TEST(s1.FindNotOf("abcde",10) == 15);
	
	s2 = sz1;
	TEST(s1.Find(s2) == 0);
	TEST(s1.Find(s2,1) == 10);

	TEST(s1.Left(5) == "abcde");
	TEST(s1.Mid(15) == "fghij");
	TEST(s1.Mid(15,3) == "fgh");
	TEST(s1.Right(3) == "hij");

	Omega::string_t s4;
	TEST(s4 == "");
	TEST(s4 == (char*)NULL);
	TEST(s4 == "");
	TEST(s4 == Omega::string_t());

	TEST(Omega::string_t("1111H").TrimLeft("1") == "H");
	TEST(Omega::string_t("1111").TrimLeft("1").IsEmpty());
	TEST(Omega::string_t("123321H").TrimLeft("123") == "H");
	TEST(Omega::string_t("123321").TrimLeft("123").IsEmpty());

	TEST(Omega::string_t("H1111").TrimRight("1") == "H");
	TEST(Omega::string_t("1111").TrimRight("1").IsEmpty());
	TEST(Omega::string_t("H123321").TrimRight("123") == "H");
	TEST(Omega::string_t("123321").TrimRight("123").IsEmpty());

	TEST(Omega::string_t("Hell") + 'o' == "Hello");

	return true;
}

struct loc_holder
{
#if defined(_WIN32)
	loc_holder()
	{
		lc = GetThreadLocale();
	}

	~loc_holder()
	{
		SetThreadLocale(lc);
	}

	LCID lc;

#else
	loc_holder()
	{
		m_loc = setlocale(LC_ALL,NULL);
	}

	~loc_holder()
	{
		setlocale(LC_ALL,m_loc.c_str());
	}

	std::string m_loc;
#endif
};

#if defined(_WIN32)
static bool set_locale_helper_win32(unsigned long win_loc)
{
	return (SetThreadLocale(win_loc) == TRUE);
}
#define set_locale_helper(w,c) set_locale_helper_win32(w)
#else
static bool set_locale_helper_crt(const char* posix_loc)
{
	return (setlocale(LC_ALL,posix_loc) != 0);
}
#define set_locale_helper(w,c) set_locale_helper_crt(c)
#endif

bool string_tests_format()
{
	TEST(Omega::string_t("1st:{0} 2nd:{1}") % 1 % 2 == "1st:1 2nd:2");
	TEST(Omega::string_t("2nd:{1} 1st:{0}") % 1 % 2 == "2nd:2 1st:1");
	TEST(Omega::string_t("1st:{0,10} 2nd:{1}") % 1 % 2 ==  "1st:         1 2nd:2");
	TEST(Omega::string_t("1st:{0,-10} 2nd:{1}") % 1 % 2 == "1st:1          2nd:2");

	{
		loc_holder lh;

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t("{0:C}") % 12345.6789 == "$12,345.68");
		}

		if (set_locale_helper(1031,"de_DE.utf8"))
		{
			Omega::string_t s = "12.345,68";
			char c[20] = "\xe2\x82\xac";
#if defined(_WIN32)
			wchar_t b[10] = {0};
			MultiByteToWideChar(CP_UTF8,0,c,-1,b,sizeof(b)/sizeof(b[0]));
			WideCharToMultiByte(CP_THREAD_ACP,0,b,-1,c,sizeof(c),NULL,NULL);
			s += " ";
#endif
			s += c;
			TEST(Omega::string_t("{0:C}") % 12345.678 == s);
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t("{0:D}") % 12345 == "12345");
			TEST(Omega::string_t("{0:D8}") % 12345 == "00012345");
			TEST(Omega::string_t("{0:E}") % 12345.6789 == "1.234568E+004");
			TEST(Omega::string_t("{0:E10}") % 12345.6789 == "1.2345678900E+004");
		}

		if (set_locale_helper(1036,"fr_FR.utf8"))
		{
			TEST(Omega::string_t("{0:E}") % 12345.6789 == "1,234568E+004");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t("{0:e4}") % 12345.6789 == "1.2346e+004");
			TEST(Omega::string_t("{0:F}") % 12345.6789 == "12345.68");
		}

		if (set_locale_helper(1034,"es_ES.utf8"))
		{
			TEST(Omega::string_t("{0:F}") % 12345.6789 == "12345,68");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t("{0:F0}") % 12345.6789 == "12346");
			TEST(Omega::string_t("{0:F6}") % 12345.6789 == "12345.678900");
			TEST(Omega::string_t("{0:G}") % 12345.6789 == "12345.6789");
			TEST(Omega::string_t("{0:G7}") % 12345.6789 == "12345.68");
			TEST(Omega::string_t("{0:G}") % 0.0000023 == "2.3E-06");
			TEST(Omega::string_t("{0:G}") % 0.0023 == "0.0023");
			TEST(Omega::string_t("{0:G2}") % 1234.0 == "1.2E+03");
			TEST(Omega::string_t("{0:N}") % 12345.6789 == "12,345.68");
		}

		if (set_locale_helper(1053,"sv_SE.utf8"))
		{
			TEST(Omega::string_t("{0:N}") % 12345.6789 == "12\xa0" "345,68");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t("{0:N4}") % 123456789 == "123,456,789.0000");
			TEST(Omega::string_t("{0:N4}") % 123456789.0 == "123,456,789.0000");
		}
	}

	TEST(Omega::string_t("{0:x}") % 0x2c45e == "2c45e");
	TEST(Omega::string_t("{0:X}") % 0x2c45e == "2C45E");
	TEST(Omega::string_t("{0:X8}") % 0x2c45e == "0002C45E");
	TEST(Omega::string_t("{0:x}") % 123456789 == "75bcd15");

	TEST(Omega::string_t("{0:R}") % 1.23456789 == "1.23456789");

	TEST(Omega::string_t("{0}") % true == "true");
	TEST(Omega::string_t("{0}") % false == "false");

	TEST(Omega::string_t("{0:#####}") % 123 == "123");
	TEST(Omega::string_t("{0:00000}") % 123 == "00123");
	TEST(Omega::string_t("{0:(###) ### - ####}") % 1234567890 == "(123) 456 - 7890");

	{
		loc_holder lh;

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			Omega::string_t t = Omega::string_t("{0:#.##}") % 1.2;

			TEST(Omega::string_t("{0:#.##}") % 1.2 == "1.2");
			TEST(Omega::string_t("{0:0.00}") % 1.2 == "1.20");
			TEST(Omega::string_t("{0:00.00}") % 1.2 == "01.20");
			TEST(Omega::string_t("{0:#,#}") % 1234567890 == "1,234,567,890");

			TEST(Omega::string_t("{0:0.###E+0}") % 86000 == "8.6E+4");
			TEST(Omega::string_t("{0:0.###E+000}") % 86000 == "8.6E+004");
			TEST(Omega::string_t("{0:0.###E-000}") % 86000 == "8.6E004");
		}
	}

	TEST(Omega::string_t("{0:[##-##-##]}") % 123456 == "[12-34-56]");
	TEST(Omega::string_t("{0:##;(##)}") % 12 == "12");
	TEST(Omega::string_t("{0:##;(##)}") % -12 == "(12)");

	TEST(Omega::string_t("{0:yes;no}") % true == "yes");
	TEST(Omega::string_t("{0:yes;no}") % false == "no");
	TEST(Omega::string_t("{0:'yes;';'no;'}") % true == "'yes;'");
	TEST(Omega::string_t("{0:\"yes;\";\"no;\"}") % false == "\"no;\"");

	return true;
}

bool guid_tests()
{
	Omega::guid_t guid(Omega::guid_t::Null());
	TEST(guid == Omega::guid_t::Null());
	TEST(guid.ToString() == "{00000000-0000-0000-0000-000000000000}");

	const char sz[] = "{BCB02DAE-998A-4FC1-AB91-39290C237A37}";

	Omega::guid_t guid2(sz);
	TEST(guid2 != guid);
	TEST(guid2 != Omega::guid_t::Null());
	
	TEST(guid2.ToString().Compare(sz)==0);
	TEST(Omega::guid_t::FromString(sz,guid2));

	// Create a load of unique guid_t's
	Omega::guid_t arr[100];
	for (size_t i=0; i<sizeof(arr)/sizeof(arr[0]); ++i)
		arr[i] = Omega::guid_t::Create();
	
	// Make sure they are unique! (This might catch time based uuids )
	bool bTest = true;
	for (size_t j=0; j<sizeof(arr)/sizeof(arr[0]) && bTest; ++j)
	{
		for (size_t k=0; k<sizeof(arr)/sizeof(arr[0]) && bTest; ++k)
		{
			if (j != k)
				bTest = (arr[j] != arr[k]);
		}
	}
	TEST(bTest);

	// Check to see if we can export OID's properly
	TEST(Omega::Remoting::OID_StdObjectManager == Omega::guid_t("{63EB243E-6AE3-43bd-B073-764E096775F8}"));

	// Check whether OMEGA_GUIDOF works...
	TEST(OMEGA_GUIDOF(Omega::IObject) == OMEGA_GUIDOF(Omega::IObject*));

	return true;
}
