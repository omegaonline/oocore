#include "../include/Omega/Omega.h"
#include "Test.h"

bool any_tests()
{
	TEST(Omega::any_t().GetType() == Omega::TypeInfo::typeVoid);
	TEST(Omega::any_t(Omega::bool_t(false)).GetType() == Omega::TypeInfo::typeBool);
	TEST(Omega::any_t(Omega::byte_t(1)).GetType() == Omega::TypeInfo::typeByte);
	TEST(Omega::any_t(Omega::int16_t(2)).GetType() == Omega::TypeInfo::typeInt16);
	TEST(Omega::any_t(Omega::uint16_t(3)).GetType() == Omega::TypeInfo::typeUInt16);
	TEST(Omega::any_t(Omega::int32_t(4)).GetType() == Omega::TypeInfo::typeInt32);
	TEST(Omega::any_t(Omega::uint32_t(5)).GetType() == Omega::TypeInfo::typeUInt32);
	TEST(Omega::any_t(Omega::int64_t(6)).GetType() == Omega::TypeInfo::typeInt64);
	TEST(Omega::any_t(Omega::uint64_t(7)).GetType() == Omega::TypeInfo::typeUInt64);
	TEST(Omega::any_t(Omega::float4_t(8.0)).GetType() == Omega::TypeInfo::typeFloat4);
	TEST(Omega::any_t(Omega::float8_t(0.9)).GetType() == Omega::TypeInfo::typeFloat8);

	TEST(Omega::any_t(Omega::guid_t::Null()).GetType() == Omega::TypeInfo::typeGuid);

	TEST(Omega::any_t(Omega::string_t(L"Blah")).GetType() == Omega::TypeInfo::typeString);
	TEST(Omega::any_t(L"Blah").GetType() == Omega::TypeInfo::typeString);
	TEST(Omega::any_t(L"").GetType() == Omega::TypeInfo::typeString);
	TEST(Omega::any_t(L"Blah",4).GetType() == Omega::TypeInfo::typeString);
	TEST(Omega::any_t("Blah",false).GetType() == Omega::TypeInfo::typeString);

	{
		Omega::any_t a1(L"String");
		Omega::any_t a2 = a1;
		TEST(a2 == L"String");
		TEST(a2 == a1);
		Omega::any_t a3(1);

		a3 = a1;
		TEST(a2 == a2);
	}

	{
		TEST(Omega::any_t() == Omega::any_t());
		TEST(Omega::any_t(false) != Omega::any_t());
	}

	// any_cast<T> tests constructor + any_t::operator T()
	{
		Omega::byte_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::int16_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::uint16_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::int32_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::uint32_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::int64_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::uint64_t v = 23;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::float4_t v = 23.0f;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::float8_t v = 23.0;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}
	{
		Omega::float4_t v = 23.5f;

		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.5f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == static_cast<Omega::float8_t>(23.5f));
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23.5");
	}
	{
		Omega::float8_t v = 23.5;

		TEST(Omega::any_cast<Omega::float4_t>(v) == static_cast<Omega::float4_t>(23.5));
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.5);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23.5");
	}

	{
		Omega::bool_t v = true;

		TEST(Omega::any_cast<Omega::bool_t>(v) == true);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 1);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 1);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 1);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 1);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 1);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 1);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 1);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 1.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 1.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"true");

		v = false;

		TEST(Omega::any_cast<Omega::bool_t>(v) == false);
		TEST(Omega::any_cast<Omega::byte_t>(v) == 0);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 0);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 0);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 0);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 0);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 0);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 0);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 0.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 0.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"false");

		Omega::any_t a(v);
		TEST(a == true);
		TEST(a != false);
	}

	{
		Omega::string_t v = L"23";

		TEST(Omega::any_cast<Omega::byte_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint16_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint32_t>(v) == 23);
		TEST(Omega::any_cast<Omega::int64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::uint64_t>(v) == 23);
		TEST(Omega::any_cast<Omega::float4_t>(v) == 23.0f);
		TEST(Omega::any_cast<Omega::float8_t>(v) == 23.0);
		TEST(Omega::any_cast<Omega::string_t>(v) == L"23");
	}

	// Now test the random comparisons
	TEST(Omega::any_t(100) == L"100");
	TEST(Omega::any_t(L"100") == 100.0);

	// Test some cast exceptions
	try
	{
		Omega::any_cast<int>(1.1f);
	}
	catch (Omega::ICastException* pE)
	{
		TEST(pE->GetValue() == Omega::any_t(1.1f));
		TEST(pE->GetReason() == Omega::any_t::castPrecisionLoss);
		pE->Release();
	}

	return true;
}
