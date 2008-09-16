#ifndef TEST_INTERFACES_INCLUDED
#define TEST_INTERFACES_INCLUDED

namespace Omega
{
	namespace TestSuite
	{
		interface ISimpleTest : public Omega::IObject
		{
			virtual Omega::bool_t BoolNot1(Omega::bool_t v) = 0;
			virtual void BoolNot2(const Omega::bool_t v, Omega::bool_t& r) = 0;
			virtual void BoolNot3(const Omega::bool_t& v, Omega::bool_t& r) = 0;
			virtual void BoolNot4(Omega::bool_t& v) = 0;

			virtual Omega::byte_t ByteInc1(Omega::byte_t v) = 0;
			virtual void ByteInc2(const Omega::byte_t v, Omega::byte_t& r) = 0;
			virtual void ByteInc3(const Omega::byte_t& v, Omega::byte_t& r) = 0;
			virtual void ByteInc4(Omega::byte_t& v) = 0;

			virtual Omega::int16_t Int16Inc1(Omega::int16_t v) = 0;
			virtual void Int16Inc2(const Omega::int16_t v, Omega::int16_t& r) = 0;
			virtual void Int16Inc3(const Omega::int16_t& v, Omega::int16_t& r) = 0;
			virtual void Int16Inc4(Omega::int16_t& v) = 0;

			virtual Omega::float4_t Float4Mul31(Omega::float4_t v) = 0;
			virtual void Float4Mul32(const Omega::float4_t v, Omega::float4_t& r) = 0;
			virtual void Float4Mul33(const Omega::float4_t& v, Omega::float4_t& r) = 0;
			virtual void Float4Mul34(Omega::float4_t& v) = 0;

			virtual Omega::float8_t Float8Mul31(Omega::float8_t v) = 0;
			virtual void Float8Mul32(const Omega::float8_t v, Omega::float8_t& r) = 0;
			virtual void Float8Mul33(const Omega::float8_t& v, Omega::float8_t& r) = 0;
			virtual void Float8Mul34(Omega::float8_t& v) = 0;

			virtual Omega::string_t Hello() = 0;

			virtual void Throw(Omega::uint32_t err) = 0;
			virtual void Abort() = 0;
		};
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::TestSuite, ISimpleTest, "{8488359E-C953-4e99-B7E5-ECA150C92F48}",

	// Methods
	OMEGA_METHOD(Omega::bool_t,BoolNot1,1,((in),Omega::bool_t,v))
	OMEGA_METHOD_VOID(BoolNot2,2,((in),const Omega::bool_t,v,(out),Omega::bool_t&,r))
	OMEGA_METHOD_VOID(BoolNot3,2,((in),const Omega::bool_t&,v,(out),Omega::bool_t&,r))
	OMEGA_METHOD_VOID(BoolNot4,1,((in_out),Omega::bool_t&,v))

	OMEGA_METHOD(Omega::byte_t,ByteInc1,1,((in),Omega::byte_t,v))
	OMEGA_METHOD_VOID(ByteInc2,2,((in),const Omega::byte_t,v,(out),Omega::byte_t&,r))
	OMEGA_METHOD_VOID(ByteInc3,2,((in),const Omega::byte_t&,v,(out),Omega::byte_t&,r))
	OMEGA_METHOD_VOID(ByteInc4,1,((in_out),Omega::byte_t&,v))

	OMEGA_METHOD(Omega::int16_t,Int16Inc1,1,((in),Omega::int16_t,v))
	OMEGA_METHOD_VOID(Int16Inc2,2,((in),const Omega::int16_t,v,(out),Omega::int16_t&,r))
	OMEGA_METHOD_VOID(Int16Inc3,2,((in),const Omega::int16_t&,v,(out),Omega::int16_t&,r))
	OMEGA_METHOD_VOID(Int16Inc4,1,((in_out),Omega::int16_t&,v))

	OMEGA_METHOD(Omega::float4_t,Float4Mul31,1,((in),Omega::float4_t,v))
	OMEGA_METHOD_VOID(Float4Mul32,2,((in),const Omega::float4_t,v,(out),Omega::float4_t&,r))
	OMEGA_METHOD_VOID(Float4Mul33,2,((in),const Omega::float4_t&,v,(out),Omega::float4_t&,r))
	OMEGA_METHOD_VOID(Float4Mul34,1,((in_out),Omega::float4_t&,v))

	OMEGA_METHOD(Omega::float8_t,Float8Mul31,1,((in),Omega::float8_t,v))
	OMEGA_METHOD_VOID(Float8Mul32,2,((in),const Omega::float8_t,v,(out),Omega::float8_t&,r))
	OMEGA_METHOD_VOID(Float8Mul33,2,((in),const Omega::float8_t&,v,(out),Omega::float8_t&,r))
	OMEGA_METHOD_VOID(Float8Mul34,1,((in_out),Omega::float8_t&,v))

	OMEGA_METHOD(Omega::string_t,Hello,0,())

	OMEGA_METHOD_VOID(Throw,1,((in),Omega::uint32_t,err))
	OMEGA_METHOD_VOID(Abort,0,())
)

#endif // TEST_INTERFACES_INCLUDED
