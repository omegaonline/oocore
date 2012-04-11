///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OMEGA_ANY_INL_INCLUDED_
#define OMEGA_ANY_INL_INCLUDED_

inline Omega::any_t::any_t(const any_t& rhs)
{
	swap(rhs);
}

inline void Omega::any_t::swap(const any_t& rhs)
{
	m_type = rhs.m_type;

	switch (rhs.m_type)
	{
	case TypeInfo::typeBool:
		u.bVal = rhs.u.bVal;
		break;
	case TypeInfo::typeByte:
		u.byVal = rhs.u.byVal;
		break;
	case TypeInfo::typeInt16:
		u.i16Val = rhs.u.i16Val;
		break;
	case TypeInfo::typeUInt16:
		u.ui16Val = rhs.u.ui16Val;
		break;
	case TypeInfo::typeInt32:
		u.i32Val = rhs.u.i32Val;
		break;
	case TypeInfo::typeUInt32:
		u.ui32Val = rhs.u.ui32Val;
		break;
	case TypeInfo::typeInt64:
		u.i64Val = rhs.u.i64Val;
		break;
	case TypeInfo::typeUInt64:
		u.ui64Val = rhs.u.ui64Val;
		break;
	case TypeInfo::typeFloat4:
		u.fl4Val = rhs.u.fl4Val;
		break;
	case TypeInfo::typeFloat8:
		u.fl8Val = rhs.u.fl8Val;
		break;
	case TypeInfo::typeString:
		strVal = rhs.strVal;
		break;
	case TypeInfo::typeGuid:
		u.gVal = rhs.u.gVal;
		break;

	default:
	case TypeInfo::typeVoid:
		break;
	}
}

inline Omega::any_t& Omega::any_t::operator = (const any_t& rhs)
{
	if (this != &rhs)
	{
		clear();
		swap(rhs);
	}
	return *this;
}

inline Omega::any_t::any_t() :
		m_type(TypeInfo::typeVoid)
{
}

inline Omega::any_t::any_t(bool_t val) :
		m_type(TypeInfo::typeBool)
{
	u.bVal = val;
}

inline Omega::any_t::any_t(byte_t val) :
		m_type(TypeInfo::typeByte)
{
	u.byVal = val;
}

inline Omega::any_t::any_t(int16_t val) :
		m_type(TypeInfo::typeInt16)
{
	u.i16Val = val;
}

inline Omega::any_t::any_t(uint16_t val) :
		m_type(TypeInfo::typeUInt16)
{
	u.ui16Val = val;
}

inline Omega::any_t::any_t(int32_t val) :
		m_type(TypeInfo::typeInt32)
{
	u.i32Val = val;
}

inline Omega::any_t::any_t(uint32_t val) :
		m_type(TypeInfo::typeUInt32)
{
	u.ui32Val = val;
}

inline Omega::any_t::any_t(const int64_t& val) :
		m_type(TypeInfo::typeInt64)
{
	u.i64Val = val;
}

inline Omega::any_t::any_t(const uint64_t& val) :
		m_type(TypeInfo::typeUInt64)
{
	u.ui64Val = val;
}

inline Omega::any_t::any_t(float4_t val) :
		m_type(TypeInfo::typeFloat4)
{
	u.fl4Val = val;
}

inline Omega::any_t::any_t(const float8_t& val) :
		m_type(TypeInfo::typeFloat8)
{
	u.fl8Val = val;
}

inline Omega::any_t::any_t(const guid_t& val) :
		m_type(TypeInfo::typeGuid)
{
	u.gVal = val;
}

inline Omega::any_t::any_t(const string_t& val) :
		m_type(TypeInfo::typeString)
{
	strVal = val;
}

inline Omega::any_t::any_t(const char* val) :
		m_type(TypeInfo::typeString)
{
	strVal = val;
}

inline Omega::any_t::~any_t()
{
	clear();
}

inline void Omega::any_t::clear()
{
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_any_t_equal,2,((in),const Omega::any_t&,lhs,(in),const Omega::any_t&,rhs));

inline bool Omega::any_t::equal(const any_t& rhs) const
{
	if (&rhs == this)
		return true;

	if (rhs.m_type != m_type)
		return OOCore_any_t_equal(*this,rhs);

	switch (m_type)
	{
	case TypeInfo::typeVoid:
		return true;
	case TypeInfo::typeBool:
		return (u.bVal == rhs.u.bVal);
	case TypeInfo::typeByte:
		return (u.byVal == rhs.u.byVal);
	case TypeInfo::typeInt16:
		return (u.i16Val == rhs.u.i16Val);
	case TypeInfo::typeUInt16:
		return (u.ui16Val == rhs.u.ui16Val);
	case TypeInfo::typeInt32:
		return (u.i32Val == rhs.u.i32Val);
	case TypeInfo::typeUInt32:
		return (u.ui32Val == rhs.u.ui32Val);
	case TypeInfo::typeInt64:
		return (u.i64Val == rhs.u.i64Val);
	case TypeInfo::typeUInt64:
		return (u.ui64Val == rhs.u.ui64Val);
	case TypeInfo::typeFloat4:
		return (u.fl4Val == rhs.u.fl4Val);
	case TypeInfo::typeFloat8:
		return (u.fl8Val == rhs.u.fl8Val);
	case TypeInfo::typeGuid:
		return (guid_t(u.gVal) == guid_t(rhs.u.gVal));
	case TypeInfo::typeString:
		return (strVal == rhs.strVal);
	default:
		// Never going to happen ;)
		return false;
	}
}

// string_t::ToNumber<T> uses helpers defined in this file
OOCORE_EXPORTED_FUNCTION(Omega::int64_t,OOCore_strto64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base));
OOCORE_EXPORTED_FUNCTION(Omega::uint64_t,OOCore_strtou64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base));
OOCORE_EXPORTED_FUNCTION(Omega::float8_t,OOCore_strtod,2,((in),const Omega::string_t&,str,(out),size_t&,end_pos));

// Helper templates
namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			// Internal helper that must be public... do not use
			void throw_cast_exception(const any_t& value, any_t::CastResult_t reason, const type_holder* typeDest);

			template <typename To, typename From,
				bool to_signed = std::numeric_limits<To>::is_signed,
				bool from_signed = std::numeric_limits<From>::is_signed,
				bool no_trunc = std::numeric_limits<To>::digits >= std::numeric_limits<From>::digits>
			struct int_conv;

			template <typename To, typename From, bool Sign>
			struct int_conv<To,From,Sign,Sign,true>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					// Can't overflow
					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From>
			struct int_conv<To,From,false,true,true>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from < 0)
						return any_t::castOverflow;

					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From>
			struct int_conv<To,From,false,true,false>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from < 0 || from > static_cast<From>(std::numeric_limits<To>::max()))
						return any_t::castOverflow;

					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From, bool Rank>
			struct int_conv<To,From,true,false,Rank>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from > static_cast<From>(std::numeric_limits<To>::max()))
						return any_t::castOverflow;

					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From>
			struct int_conv<To,From,false,false,false>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from > std::numeric_limits<To>::max())
						return any_t::castOverflow;

					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From>
			struct int_conv<To,From,true,true,false>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from < std::numeric_limits<To>::min() || from > std::numeric_limits<To>::max())
						return any_t::castOverflow;

					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From,
				bool no_trunc = std::numeric_limits<To>::digits >= std::numeric_limits<From>::digits>
			struct flt_conv
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					if (from < std::numeric_limits<To>::min() || from > std::numeric_limits<To>::max())
						return any_t::castOverflow;

					To to2 = static_cast<To>(from);

					if (static_cast<From>(to2) != from)
						return any_t::castPrecisionLoss;

					to = to2;
					return any_t::castValid;
				}
			};

			template <typename To, typename From>
			struct flt_conv<To,From,true>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					// Can't overflow
					to = static_cast<To>(from);
					return any_t::castValid;
				}
			};

			template <typename To, typename From,
				bool to_integer = std::numeric_limits<To>::is_integer,
				bool from_integer = std::numeric_limits<From>::is_integer>
			struct converter
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					return flt_conv<To,From>::cast(to,from);
				}
			};

			// Integer -> Integer
			template <typename To, typename From>
			struct converter<To,From,true,true>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					return int_conv<To,From>::cast(to,from);
				}
			};

			// Float -> Integer
			template <typename To, typename From>
			struct converter<To,From,true,false>
			{
				static any_t::CastResult_t cast(To& to, typename optimal_param<From>::type from)
				{
					// Check for precision loss...
					From int_part;
					From frac_part = std::modf(from,&int_part);
					if (frac_part != 0)
						return any_t::castPrecisionLoss;

					return int_conv<To,From>::cast(to,int_part);
				}
			};

			template <typename T>
			struct integer_scanner_t
			{
				static any_t::CastResult_t ToNumber(T& ret, const string_t& val)
				{
					size_t end_pos = string_t::npos;
					int64_t v = OOCore_strto64(val,end_pos,10);
					if (end_pos != string_t::npos)
						return any_t::castOverflow;

					return converter<T,int64_t>::cast(ret,v);
				}
			};

			template <typename T>
			struct unsigned_integer_scanner_t
			{
				static any_t::CastResult_t ToNumber(T& ret, const string_t& val)
				{
					size_t end_pos = string_t::npos;
					uint64_t v = OOCore_strtou64(val,end_pos,10);
					if (end_pos != string_t::npos)
						return any_t::castOverflow;

					return converter<T,uint64_t>::cast(ret,v);
				}
			};

			template <typename T>
			struct float_scanner_t
			{
				static any_t::CastResult_t ToNumber(T& ret, const string_t& val)
				{
					size_t end_pos = string_t::npos;
					float8_t v = OOCore_strtod(val,end_pos);
					if (end_pos != string_t::npos)
						return any_t::castOverflow;

					return converter<T,float8_t>::cast(ret,v);
				}
			};

			// Typename T is not a 'number'
			template <typename T>
			struct invalid_scanner_t;

			template <typename T>
			struct scanner_t
			{
				typedef typename if_else_t<std::numeric_limits<T>::is_specialized,
					typename if_else_t<std::numeric_limits<T>::is_integer,
						typename if_else_t<std::numeric_limits<T>::is_signed,integer_scanner_t<T>,unsigned_integer_scanner_t<T> >::result,
						float_scanner_t<T>
					>::result,
					invalid_scanner_t<T>
				>::result type;
			};

			template <typename T>
			struct scanner_t<const T>
			{
				typedef typename scanner_t<T>::type type;
			};

			// Don't pass pointers or refs
			template <typename T> struct scanner_t<T*>;
			template <typename T> struct scanner_t<T&>;
			template <typename T, size_t S> struct scanner_t<T[S]>;

			// Long doubles are not compiler agnostic...
			template <> struct scanner_t<long double>;
		}
	}
}

template <typename T>
inline Omega::any_t::CastResult_t Omega::any_t::Coerce(T& v) const
{
	// Check for pointers/refs/STL etc
	const System::Internal::type_holder* other_type = System::Internal::type_kind<T>::type();
	if (other_type->next && other_type->type != TypeInfo::typeObject)
		return any_t::castUnrelated;

	switch (m_type)
	{
	case TypeInfo::typeBool:
		v = static_cast<T>(u.bVal ? 1 : 0);
		return any_t::castValid;
	case TypeInfo::typeByte:
		return System::Internal::converter<T,byte_t>::cast(v,u.byVal);
	case TypeInfo::typeInt16:
		return System::Internal::converter<T,int16_t>::cast(v,u.i16Val);
	case TypeInfo::typeUInt16:
		return System::Internal::converter<T,uint16_t>::cast(v,u.ui16Val);
	case TypeInfo::typeInt32:
		return System::Internal::converter<T,int32_t>::cast(v,u.i32Val);
	case TypeInfo::typeUInt32:
		return System::Internal::converter<T,uint32_t>::cast(v,u.ui32Val);
	case TypeInfo::typeInt64:
		return System::Internal::converter<T,int64_t>::cast(v,u.i64Val);
	case TypeInfo::typeUInt64:
		return System::Internal::converter<T,uint64_t>::cast(v,u.ui64Val);
	case TypeInfo::typeFloat4:
		return System::Internal::converter<T,float4_t>::cast(v,u.fl4Val);
	case TypeInfo::typeFloat8:
		return System::Internal::converter<T,float8_t>::cast(v,u.fl8Val);
	case TypeInfo::typeString:
		return System::Internal::scanner_t<T>::type::ToNumber(v,strVal);

	case TypeInfo::typeVoid:
	case TypeInfo::typeGuid:
	default:
		return any_t::castUnrelated;
	}
}

inline Omega::any_t::CastResult_t Omega::any_t::Coerce(guid_t& v) const
{
	if (m_type == TypeInfo::typeGuid)
	{
		v = u.gVal;
		return any_t::castValid;
	}
	else if (m_type == TypeInfo::typeString)
	{
		if (guid_t::FromString(strVal,v))
			return any_t::castValid;
	}

	return any_t::castUnrelated;
}

inline Omega::any_t::CastResult_t Omega::any_t::Coerce(string_t& v, const string_t& strFormat) const
{
	switch (m_type)
	{
	case TypeInfo::typeBool:
		v = Formatting::ToString(u.bVal,strFormat);
		break;
	case TypeInfo::typeByte:
		v = Formatting::ToString(u.byVal,strFormat);
		break;
	case TypeInfo::typeInt16:
		v = Formatting::ToString(u.i16Val,strFormat);
		break;
	case TypeInfo::typeUInt16:
		v = Formatting::ToString(u.ui16Val,strFormat);
		break;
	case TypeInfo::typeInt32:
		v = Formatting::ToString(u.i32Val,strFormat);
		break;
	case TypeInfo::typeUInt32:
		v = Formatting::ToString(u.ui32Val,strFormat);
		break;
	case TypeInfo::typeInt64:
		v = Formatting::ToString(u.i64Val,strFormat);
		break;
	case TypeInfo::typeUInt64:
		v = Formatting::ToString(u.ui64Val,strFormat);
		break;
	case TypeInfo::typeFloat4:
		v = Formatting::ToString(u.fl4Val,strFormat.IsEmpty() ? string_t::constant("R") : strFormat);
		break;
	case TypeInfo::typeFloat8:
		v = Formatting::ToString(u.fl8Val,strFormat.IsEmpty() ? string_t::constant("R") : strFormat);
		break;
	case TypeInfo::typeGuid:
		v = guid_t(u.gVal).ToString(strFormat);
		break;
	case TypeInfo::typeString:
		v = strVal;
		break;

	case TypeInfo::typeVoid:
	default:
		// Never going to happen ;)
		return any_t::castUnrelated;
	}

	return any_t::castValid;
}

inline Omega::any_t::CastResult_t Omega::any_t::Coerce(bool_t& v) const
{
	switch (m_type)
	{
	case TypeInfo::typeBool:
		v = u.bVal;
		break;
	case TypeInfo::typeByte:
		v = (u.byVal ? true : false);
		break;
	case TypeInfo::typeInt16:
		v = (u.i16Val ? true : false);
		break;
	case TypeInfo::typeUInt16:
		v = (u.ui16Val ? true : false);
		break;
	case TypeInfo::typeInt32:
		v = (u.i32Val ? true : false);
		break;
	case TypeInfo::typeUInt32:
		v = (u.ui32Val ? true : false);
		break;
	case TypeInfo::typeInt64:
		v = (u.i64Val ? true : false);
		break;
	case TypeInfo::typeUInt64:
		v = (u.ui64Val ? true : false);
		break;
	case TypeInfo::typeFloat4:
		v = (u.fl4Val ? true : false);
		break;
	case TypeInfo::typeFloat8:
		v = (u.fl8Val ? true : false);
		break;
	case TypeInfo::typeString:
		if (strVal == Formatting::ToString(true))
			v = true;
		else if (strVal == Formatting::ToString(false))
			v = false;
		else
			return any_t::castUnrelated;
		break;

	case TypeInfo::typeVoid:
	case TypeInfo::typeGuid:
		// Always invalid
	default:
		// Never going to happen ;)
		return any_t::castUnrelated;
	}

	return any_t::castValid;
}

inline Omega::string_t Omega::any_t::ToString(const string_t& strFormat) const
{
	string_t v;
	any_t::CastResult_t r = Coerce(v,strFormat);
	if (r != any_t::castValid)
		System::Internal::throw_cast_exception(*this,r,System::Internal::type_kind<string_t>::type());

	return v;
}

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			template <typename C>
			struct cast_helper
			{
				static C cast(const any_t& a)
				{
					C v = System::Internal::default_value<C>::value();
					Omega::any_t::CastResult_t r = a.Coerce(v);
					if (r != any_t::castValid)
						throw_cast_exception(a,r,System::Internal::type_kind<C>::type());

					return v;
				}
			};

			template <typename C>
			struct cast_helper<const C>
			{
				static const C cast(const any_t& a)
				{
					C v = System::Internal::default_value<const C>::value();
					Omega::any_t::CastResult_t r = a.Coerce(v);
					if (r != any_t::castValid)
						throw_cast_exception(a,r,System::Internal::type_kind<const C>::type());

					return v;
				}
			};

			// Do not attempt to cast any_t to a reference type
			template <typename C>
			struct cast_helper<C&>;
		}
	}
}

template <typename T>
inline T Omega::any_t::cast() const
{
	return System::Internal::cast_helper<T>::cast(*this);
}

// string_t::ToNumber<T> uses helpers defined in this file
template <typename T>
inline T Omega::string_t::ToNumber() const
{
	T ret = System::Internal::default_value<T>::value();
	any_t::CastResult_t r = System::Internal::scanner_t<T>::type::ToNumber(ret,*this);
	if (r != any_t::castValid)
		System::Internal::throw_cast_exception(any_t(*this),r,System::Internal::type_kind<T>::type());
	return ret;
}

inline Omega::int64_t Omega::string_t::strto64(const string_t& str, size_t& end_pos, unsigned int base)
{
	return OOCore_strto64(str,end_pos,base);
}

inline Omega::uint64_t Omega::string_t::strtou64(const string_t& str, size_t& end_pos, unsigned int base)
{
	return OOCore_strtou64(str,end_pos,base);
}

inline Omega::float8_t Omega::string_t::strtod(const string_t& str, size_t& end_pos)
{
	return OOCore_strtod(str,end_pos);
}

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			// type_holder is safe across DLL boundaries...
			template <>
			struct is_c_abi<type_holder>
			{
				enum
				{
					result = 1
				};
			};
		}
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_ICastException_Throw,3,((in),const Omega::any_t&,value,(in),Omega::any_t::CastResult_t,reason,(in),const Omega::System::Internal::type_holder*,typeDest));

inline void Omega::System::Internal::throw_cast_exception(const Omega::any_t& value, Omega::any_t::CastResult_t reason, const type_holder* typeDest)
{
	OOCore_ICastException_Throw(value,reason,typeDest);
}

#endif // OMEGA_ANY_INL_INCLUDED_
