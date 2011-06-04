#include "../include/Omega/Remoting.h"
#include "Test.h"

bool string_tests()
{
	const wchar_t sz1[] = L"abcdef";
	const wchar_t sz1_1[] = L"abcdef";
	const wchar_t sz1_2[] = L"ABCDEF";

	Omega::string_t s1;
	TEST(s1.IsEmpty());
	TEST(!s1);

	s1 = sz1;
	TEST(!s1.IsEmpty());
	TEST(s1.Length() == 6);

	TEST(s1 == sz1_1 && !(s1 != sz1_1));
	TEST(s1.Compare(sz1_1) == 0);
	TEST(s1.Compare(sz1_2,0,Omega::string_t::npos,true) == 0);

	const wchar_t sz2[] = L"ghijk";
	Omega::string_t s2(L"ghijk");
	TEST(s2 == sz2);

	Omega::string_t s3(s1);
	TEST(s3 == sz1);

	TEST(s3 == s1 && !(s3 != s1));
	TEST(s3.Compare(s1) == 0);

	s3 = s2;
	TEST(s3 == s2);
	TEST(wcscmp(s3.c_wstr(),sz2) == 0);

	s3.Clear();
	TEST(s3.IsEmpty());

	TEST(s2 != L"ghijklmno");

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
	TEST(s4 == L"");
	TEST(s4 == Omega::string_t());

	TEST(Omega::string_t(L"1111H").TrimLeft(L'1') == L"H");
	TEST(Omega::string_t(L"1111").TrimLeft(L'1').IsEmpty());
	TEST(Omega::string_t(L"123321H").TrimLeft(L"123") == L"H");
	TEST(Omega::string_t(L"123321").TrimLeft(L"123").IsEmpty());

	TEST(Omega::string_t(L"H1111").TrimRight(L'1') == L"H");
	TEST(Omega::string_t(L"1111").TrimRight(L'1').IsEmpty());
	TEST(Omega::string_t(L"H123321").TrimRight(L"123") == L"H");
	TEST(Omega::string_t(L"123321").TrimRight(L"123").IsEmpty());

	TEST(Omega::string_t(L"Hell") + L'o' == L"Hello");

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
		m_loc = setlocale(LC_ALL,"C");
	}

	~loc_holder()
	{
		setlocale(LC_ALL,"C");
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
	TEST(Omega::string_t(L"1st:{0} 2nd:{1}") % 1 % 2 == L"1st:1 2nd:2");
	TEST(Omega::string_t(L"2nd:{1} 1st:{0}") % 1 % 2 == L"2nd:2 1st:1");
	TEST(Omega::string_t(L"1st:{0,10} 2nd:{1}") % 1 % 2 ==  L"1st:         1 2nd:2");
	TEST(Omega::string_t(L"1st:{0,-10} 2nd:{1}") % 1 % 2 == L"1st:1          2nd:2");

	{
		loc_holder lh;

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t(L"{0:C}") % 12345.6789 == L"$12,345.68");
		}

		if (set_locale_helper(1031,"de_DE.utf8"))
		{
			TEST(Omega::string_t(L"{0:C}") % 12345.678 == L"12.345,68 \x20AC");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t(L"{0:D}") % 12345 == L"12345");
			TEST(Omega::string_t(L"{0:D8}") % 12345 == L"00012345");
			TEST(Omega::string_t(L"{0:E}") % 12345.6789 == L"1.234568E+004");
			TEST(Omega::string_t(L"{0:E10}") % 12345.6789 == L"1.2345678900E+004");
		}

		if (set_locale_helper(1036,"fr_FR.utf8"))
		{
			TEST(Omega::string_t(L"{0:E}") % 12345.6789 == L"1,234568E+004");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t(L"{0:e4}") % 12345.6789 == L"1.2346e+004");
			TEST(Omega::string_t(L"{0:F}") % 12345.6789 == L"12345.68");
		}

		if (set_locale_helper(1034,"es_ES.utf8"))
		{
			TEST(Omega::string_t(L"{0:F}") % 12345.6789 == L"12345,68");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t(L"{0:F0}") % 12345.6789 == L"12346");
			TEST(Omega::string_t(L"{0:F6}") % 12345.6789 == L"12345.678900");
			TEST(Omega::string_t(L"{0:G}") % 12345.6789 == L"12345.6789");
			TEST(Omega::string_t(L"{0:G7}") % 12345.6789 == L"12345.68");
			TEST(Omega::string_t(L"{0:G}") % 0.0000023 == L"2.3E-06");
			TEST(Omega::string_t(L"{0:G}") % 0.0023 == L"0.0023");
			TEST(Omega::string_t(L"{0:G2}") % 1234.0 == L"1.2E+03");
			TEST(Omega::string_t(L"{0:N}") % 12345.6789 == L"12,345.68");
		}

		if (set_locale_helper(1053,"sv_SE.utf8"))
		{
			TEST(Omega::string_t(L"{0:N}") % 12345.6789 == L"12\xa0" L"345,68");
		}

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			TEST(Omega::string_t(L"{0:N4}") % 123456789 == L"123,456,789.0000");
			TEST(Omega::string_t(L"{0:N4}") % 123456789.0 == L"123,456,789.0000");
		}
	}

	TEST(Omega::string_t(L"{0:x}") % 0x2c45e == L"2c45e");
	TEST(Omega::string_t(L"{0:X}") % 0x2c45e == L"2C45E");
	TEST(Omega::string_t(L"{0:X8}") % 0x2c45e == L"0002C45E");
	TEST(Omega::string_t(L"{0:x}") % 123456789 == L"75bcd15");

	TEST(Omega::string_t(L"{0:R}") % 1.23456789 == L"1.23456789");

	TEST(Omega::string_t(L"{0}") % true == L"true");
	TEST(Omega::string_t(L"{0}") % false == L"false");

	TEST(Omega::string_t(L"{0:#####}") % 123 == L"123");
	TEST(Omega::string_t(L"{0:00000}") % 123 == L"00123");
	TEST(Omega::string_t(L"{0:(###) ### - ####}") % 1234567890 == L"(123) 456 - 7890");

	{
		loc_holder lh;

		if (set_locale_helper(1033,"en_US.utf8"))
		{
			Omega::string_t t = Omega::string_t(L"{0:#.##}") % 1.2;

			TEST(Omega::string_t(L"{0:#.##}") % 1.2 == L"1.2");
			TEST(Omega::string_t(L"{0:0.00}") % 1.2 == L"1.20");
			TEST(Omega::string_t(L"{0:00.00}") % 1.2 == L"01.20");
			TEST(Omega::string_t(L"{0:#,#}") % 1234567890 == L"1,234,567,890");

			TEST(Omega::string_t(L"{0:0.###E+0}") % 86000 == L"8.6E+4");
			TEST(Omega::string_t(L"{0:0.###E+000}") % 86000 == L"8.6E+004");
			TEST(Omega::string_t(L"{0:0.###E-000}") % 86000 == L"8.6E004");
		}
	}

	TEST(Omega::string_t(L"{0:[##-##-##]}") % 123456 == L"[12-34-56]");
	TEST(Omega::string_t(L"{0:##;(##)}") % 12 == L"12");
	TEST(Omega::string_t(L"{0:##;(##)}") % -12 == L"(12)");

	TEST(Omega::string_t(L"{0:yes;no}") % true == L"yes");
	TEST(Omega::string_t(L"{0:yes;no}") % false == L"no");
	TEST(Omega::string_t(L"{0:'yes;';'no;'}") % true == L"'yes;'");
	TEST(Omega::string_t(L"{0:\"yes;\";\"no;\"}") % false == L"\"no;\"");

	return true;
}

bool string_tests_utf8()
{
#if defined(_MSC_VER)
	FILE* pInUTF8 = fopen("UTF-8-test.txt","rb");
#else
	FILE* pInUTF8 = fopen(OMEGA_STRINGIZE(TOP_SRC_DIR) "/test/UTF-8-test.txt","rb");
#endif

	if (!pInUTF8)
	{
		output("[Skipped]\n");
		return false;
	}

	// Loop reading and converting...
	std::string strUTF8;
	for (;;)
	{
		char szBuf[129];
		if (!fgets(szBuf,128,pInUTF8))
			break;

		szBuf[128] = '\0';
		strUTF8 += szBuf;
	}
	fclose(pInUTF8);

#if defined(_MSC_VER)
	FILE* pInUTF16 = fopen("UTF-16-test.txt","rb");
#else
	FILE* pInUTF16 = fopen(OMEGA_STRINGIZE(TOP_SRC_DIR) "/test/UTF-16-test.txt","rb");
#endif

	if (!pInUTF16)
	{
		output("[Skipped]\n");
		return false;
	}

	// Loop reading and converting...
	std::wstring strUTF16;
	strUTF16.reserve(strUTF8.size());
	for (;;)
	{
		Omega::uint16_t szBuf[128];
		if (!fread(szBuf,sizeof(Omega::uint16_t),128,pInUTF16))
			break;

		for (int i=0; i<128; ++i)
		{
			unsigned int v = szBuf[i];
			if (sizeof(wchar_t) == 4)
			{
				if (v >= 0xD800 && v <= 0xDBFF)
				{
					// Surrogate pair
					v = (v & 0x27FF) << 10;
					v += ((szBuf[++i] & 0x23FF) >> 10) + 0x10000;
				}
				else if (v >= 0xDC00 && v <= 0xDFFF)
				{
					// Surrogate pair
					v = (v & 0x23FF) >> 10;
					v += ((szBuf[++i] & 0x27FF) << 10) + 0x10000;
				}
			}

			strUTF16.append(1,wchar_t(v));
		}
	}
	fclose(pInUTF16);

	Omega::string_t str(strUTF8.c_str(),true);

	//TEST(str == strUTF16.c_str());
	const char* s = str.c_ustr();

#if 0
	FILE* pOutUTF8 = fopen("UTF-8-out.txt","wb");

	fputs(s,pOutUTF8);
	fclose(pOutUTF8);

#endif

	TEST(s == strUTF8);

	return true;
}

bool guid_tests()
{
	Omega::guid_t guid(Omega::guid_t::Null());
	TEST(guid == Omega::guid_t::Null());
	TEST(guid.ToString() == L"{00000000-0000-0000-0000-000000000000}");

	const wchar_t sz[] = L"{BCB02DAE-998A-4FC1-AB91-39290C237A37}";

	Omega::guid_t guid2(sz);
	TEST(guid2 != guid);
	TEST(guid2 != Omega::guid_t::Null());
	
	TEST(guid2.ToString().Compare(sz,0,Omega::string_t::npos,true)==0);
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
	TEST(Omega::Remoting::OID_StdObjectManager == Omega::guid_t(L"{63EB243E-6AE3-43bd-B073-764E096775F8}"));

	// Check whether OMEGA_GUIDOF works...
	TEST(OMEGA_GUIDOF(Omega::IObject) == OMEGA_GUIDOF(Omega::IObject*));

	return true;
}
