#include <Omega/Omega.h>
#include "Test.h"

bool any_tests()
{
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
	}


	return true;
}
