#include <Omega/Omega.h>
#include "Test.h"

bool any_tests()
{
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
