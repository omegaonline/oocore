#ifndef SIMPLE_TEST_INCLUDED
#define SIMPLE_TEST_INCLUDED

#include "interfaces.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

class SimpleTestImpl :
	public Omega::TestSuite::ISimpleTest,
	public Omega::TestSuite::ISimpleTest2
{
public:
	Omega::bool_t BoolNot1(Omega::bool_t v)
	{
		return !v;
	}
	void BoolNot2(const Omega::bool_t v, Omega::bool_t& r)
	{
		r = !v;
	}
	void BoolNot3(const Omega::bool_t& v, Omega::bool_t& r)
	{
		r = !v;
	}
	void BoolNot4(Omega::bool_t& v)
	{
		v = !v;
	}

	Omega::byte_t ByteInc1(Omega::byte_t v)
	{
		return v+1;
	}
	void ByteInc2(const Omega::byte_t v, Omega::byte_t& r)
	{
		r = v+1;
	}
	void ByteInc3(const Omega::byte_t& v, Omega::byte_t& r)
	{
		r = v+1;
	}
	void ByteInc4(Omega::byte_t& v)
	{
		++v;
	}

	Omega::int16_t Int16Inc1(Omega::int16_t v)
	{
		return v+1;
	}
	void Int16Inc2(const Omega::int16_t v, Omega::int16_t& r)
	{
		r = v+1;
	}
	void Int16Inc3(const Omega::int16_t& v, Omega::int16_t& r)
	{
		r = v+1;
	}
	void Int16Inc4(Omega::int16_t& v)
	{
		++v;
	}

	Omega::float4_t Float4Mul31(Omega::float4_t v)
	{
		return v * 3;
	}
	void Float4Mul32(const Omega::float4_t v, Omega::float4_t& r)
	{
		r = v * 3;
	}
	void Float4Mul33(const Omega::float4_t& v, Omega::float4_t& r)
	{
		r = v * 3;
	}
	void Float4Mul34(Omega::float4_t& v)
	{
		v = v * 3;
	}

	Omega::float8_t Float8Mul31(Omega::float8_t v)
	{
		return v * 3;
	}
	void Float8Mul32(const Omega::float8_t v, Omega::float8_t& r)
	{
		r = v * 3;
	}
	void Float8Mul33(const Omega::float8_t& v, Omega::float8_t& r)
	{
		r = v * 3;
	}
	void Float8Mul34(Omega::float8_t& v)
	{
		v = v * 3;
	}

	Omega::string_t Hello()
	{
		return L"Hello!";
	}

	void Throw(Omega::uint32_t err)
	{
		throw Omega::ISystemException::Create(err,L"TestLibraryImpl");
	}

	Omega::string_t WhereAmI()
	{
		return L"Inner";
	}

	Omega::uint32_t ListUInt32_Count(const std::list<Omega::uint32_t>& list)
	{
		return static_cast<Omega::uint32_t>(list.size());
	}

	std::list<Omega::uint32_t> ListUInt32_Fill()
	{
		std::list<Omega::uint32_t> list;
		list.push_back(1);
		list.push_back(2);
		return list;
	}
};

#endif // SIMPLE_TEST_INCLUDED
