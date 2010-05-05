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
		OMEGA_NEW(u.pstrVal,string_t(*rhs.u.pstrVal));
		break;
	case TypeInfo::typeGuid:
		OMEGA_NEW(u.pgVal,guid_t(*rhs.u.pgVal));
		break;

	case TypeInfo::typeVoid:
		break;
	
	case TypeInfo::typeObjectPtr:

	case TypeInfo::typeAny:
	default:
		OMEGA_THROW(L"Invalid any_t type!");
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
	if (val != guid_t::Null())
		OMEGA_NEW(u.pgVal,guid_t(val));
	else
		u.pgVal = 0;
}

inline Omega::any_t::any_t(const string_t& val) :
	m_type(TypeInfo::typeString)
{
	OMEGA_NEW(u.pstrVal,string_t(val));
}

inline Omega::any_t::any_t(const wchar_t* wsz, size_t length, bool copy) :
	m_type(TypeInfo::typeString)
{
	OMEGA_NEW(u.pstrVal,string_t(wsz,length,copy));
}

inline Omega::any_t::any_t(const char* sz, bool bUTF8, size_t length) :
	m_type(TypeInfo::typeString)
{
	OMEGA_NEW(u.pstrVal,string_t(sz,bUTF8,length));
}

inline Omega::any_t::~any_t()
{
	clear();
}

inline void Omega::any_t::clear()
{
	if (m_type == TypeInfo::typeGuid)
		delete u.pgVal;
	else if (m_type == TypeInfo::typeString)
		delete u.pstrVal;
}

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			// Internal helper that must be public... do not use
			void throw_cast_exception(const any_t& value, any_t::CastResult_t reason, const type_holder* typeDest);

			struct any_t_safe_type
			{
				struct safe_type
				{
					TypeInfo::Type_t type;
					union discrim
					{
						// Stock types
						byte_t           byVal;
						int16_t          i16Val;
						uint16_t         ui16Val;
						int32_t          i32Val;
						uint32_t         ui32Val;
						int64_t          i64Val;
						uint64_t         ui64Val;
						float4_t         fl4Val;
						float8_t         fl8Val;
						
						// Custom safe types
						custom_safe_type<bool_t>::safe_type bVal;
						string_t_safe_type::safe_type       pstrVal;
						guid_base_t                         gVal;
						//obj_holder_base* pobjVal;
					} u;
				};

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(any_t_safe_type::create(val,true))
					{ }

					void update(safe_type& dest)
					{
						any_t_safe_type::release(dest);
						dest = any_t_safe_type::addref(m_val);
					}

					operator any_t&()
					{
						return m_val;
					}

				private:
					any_t m_val;
				};
				friend struct type_wrapper;
				
				struct safe_type_wrapper
				{
					safe_type_wrapper(const any_t& val) : m_val(any_t_safe_type::addref(val))
					{ }

					~safe_type_wrapper()
					{
						any_t_safe_type::release(m_val);
					}

					void update(any_t& dest)
					{
						dest = any_t_safe_type::create(m_val,true);
					}

					operator safe_type ()
					{
						return m_val;
					}

					safe_type* operator & ()
					{
						return &m_val;
					}

				private:
					safe_type m_val;
				};
				friend struct safe_type_wrapper;

				static safe_type clone(const any_t& s)
				{
					return addref(s);
				}

				static any_t clone(safe_type v)
				{
					return create(v,false);
				}

			private:
				static any_t create(safe_type v, bool addref)
				{
					switch (v.type)
					{
					case TypeInfo::typeVoid:
						return any_t();
					case TypeInfo::typeBool:
						return static_cast<bool_t>(v.u.bVal != 0);
					case TypeInfo::typeByte:
						return v.u.byVal;
					case TypeInfo::typeInt16:
						return v.u.i16Val;
					case TypeInfo::typeUInt16:
						return v.u.ui16Val;
					case TypeInfo::typeInt32:
						return v.u.i32Val;
					case TypeInfo::typeUInt32:
						return v.u.ui32Val;
					case TypeInfo::typeInt64:
						return v.u.i64Val;
					case TypeInfo::typeUInt64:
						return v.u.ui64Val;
					case TypeInfo::typeFloat4:
						return v.u.fl4Val;
					case TypeInfo::typeFloat8:
						return v.u.fl8Val;
					case TypeInfo::typeString:
						return string_t_safe_type::create(v.u.pstrVal,addref);
					case TypeInfo::typeGuid:
						return guid_t(v.u.gVal);
					case TypeInfo::typeObjectPtr:
					default:
						// Never going to happen ;)
						OMEGA_THROW(L"Invalid any_t type!");
					}					
				}

				static safe_type addref(const any_t& val)
				{
					safe_type ret;
					ret.type = static_cast<TypeInfo::Type_t>(val.m_type);
					switch (val.m_type)
					{
					case TypeInfo::typeVoid:
						break;
					case TypeInfo::typeBool:
						ret.u.bVal = (val.u.bVal ? 1 : 0);
						break;
					case TypeInfo::typeByte:
						ret.u.byVal = val.u.byVal;
						break;
					case TypeInfo::typeInt16:
						ret.u.i16Val = val.u.i16Val;
						break;
					case TypeInfo::typeUInt16:
						ret.u.ui16Val = val.u.ui16Val;
						break;
					case TypeInfo::typeInt32:
						ret.u.i32Val = val.u.i32Val;
						break;
					case TypeInfo::typeUInt32:
						ret.u.ui32Val = val.u.ui32Val;
						break;
					case TypeInfo::typeInt64:
						ret.u.i64Val = val.u.i64Val;
						break;
					case TypeInfo::typeUInt64:
						ret.u.ui64Val = val.u.ui64Val;
						break;
					case TypeInfo::typeFloat4:
						ret.u.fl4Val = val.u.fl4Val;
						break;
					case TypeInfo::typeFloat8:
						ret.u.fl8Val = val.u.fl8Val;
						break;
					case TypeInfo::typeString:
						ret.u.pstrVal = string_t_safe_type::addref(*val.u.pstrVal,true);
						break;
					case TypeInfo::typeGuid:
						ret.u.gVal = (val.u.pgVal ? guid_t::Null() : *val.u.pgVal);
						break;

					case TypeInfo::typeObjectPtr:
					default:
						// Never going to happen ;)
						OMEGA_THROW(L"Invalid any_t type!");
					}
					return ret;
				}

				static void release(safe_type val)
				{
					if (val.type == TypeInfo::typeString)
						string_t_safe_type::release(val.u.pstrVal);
				}
			};

			template <>
			struct custom_safe_type<any_t>
			{
				typedef struct any_t_safe_type impl;
			};
		}
	}
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
		return (u.pgVal ? *u.pgVal : guid_t::Null()) == (rhs.u.pgVal ? *rhs.u.pgVal : guid_t::Null());
	case TypeInfo::typeString:
		return (*u.pstrVal == *rhs.u.pstrVal);
	//case TypeInfo::typeObjectPtr:
	default:
		// Never going to happen ;)
		return false;
	}
}

// string_t::ToNumber<T> uses helpers defined in this file
OOCORE_EXPORTED_FUNCTION(Omega::int64_t,OOCore_wcsto64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base));
OOCORE_EXPORTED_FUNCTION(Omega::uint64_t,OOCore_wcstou64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base));
OOCORE_EXPORTED_FUNCTION(Omega::float8_t,OOCore_wcstod,2,((in),const Omega::string_t&,str,(out),size_t&,end_pos));

// Helper templates
namespace Omega
{
	namespace System
	{
		namespace Internal
		{
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
					
					return int_conv<To,From>::cast(to,from);
				}
			};

			template <typename T>
			struct integer_scanner_t
			{
				static any_t::CastResult_t ToNumber(T& ret, const string_t& val)
				{
					size_t end_pos = string_t::npos;
					int64_t v = OOCore_wcsto64(val,end_pos,10);
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
					uint64_t v = OOCore_wcstou64(val,end_pos,10);
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
					float8_t v = OOCore_wcstod(val,end_pos);
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
	if (other_type->next && other_type->type != TypeInfo::typeObjectPtr)
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
		return System::Internal::scanner_t<T>::type::ToNumber(v,*u.pstrVal);
				
	//case TypeInfo::typeObjectPtr:

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
		v = (u.pgVal ? *u.pgVal : guid_t::Null());
		return any_t::castValid;
	}
	else if (m_type == TypeInfo::typeString)
	{
		if (guid_t::FromString(*u.pstrVal,v))
			return any_t::castValid;
	}
	
	return any_t::castUnrelated;
}

inline Omega::any_t::CastResult_t Omega::any_t::Coerce(string_t& v) const
{
	switch (m_type)
	{
	case TypeInfo::typeBool:
		v = Formatting::ToString(u.bVal);
		break;
	case TypeInfo::typeByte:
		v = Formatting::ToString(u.byVal);
		break;
	case TypeInfo::typeInt16:
		v = Formatting::ToString(u.i16Val);
		break;
	case TypeInfo::typeUInt16:
		v = Formatting::ToString(u.ui16Val);
		break;
	case TypeInfo::typeInt32:
		v = Formatting::ToString(u.i32Val);
		break;
	case TypeInfo::typeUInt32:
		v = Formatting::ToString(u.ui32Val);
		break;
	case TypeInfo::typeInt64:
		v = Formatting::ToString(u.i64Val);
		break;
	case TypeInfo::typeUInt64:
		v = Formatting::ToString(u.ui64Val);
		break;
	case TypeInfo::typeFloat4:
		v = Formatting::ToString(u.fl4Val,L"R");
		break;
	case TypeInfo::typeFloat8:
		v = Formatting::ToString(u.fl8Val,L"R");
		break;
	case TypeInfo::typeGuid:
		if (u.pgVal)
			v = u.pgVal->ToString();
		else
			v = guid_t::Null().ToString();
		break;
	case TypeInfo::typeString:
		v = *u.pstrVal;
		break;

	//case TypeInfo::typeObjectPtr:
		// QI for something and let that throw...

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
		v = (u.bVal ? true : false);
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
		{
			string_t t(L"{0}");
			string_t f(L"{0}");
			t %= true;
			f %= false;
			if (*u.pstrVal == t)
				v = true;
			else if (*u.pstrVal == f)
				v = false;
			else
				return any_t::castUnrelated;
		}
		break;

	//case TypeInfo::typeObjectPtr:
		// Test for null

	case TypeInfo::typeVoid:
	case TypeInfo::typeGuid:
		// Always invalid
	default: 
		// Never going to happen ;)
		return any_t::castUnrelated;
	}

	return any_t::castValid;
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
					const C v = System::Internal::default_value<const C>::value();
					Omega::any_t::CastResult_t r = a.Coerce(v);
					if (r != any_t::castValid)
						throw_cast_exception(a,r,System::Internal::type_kind<const C>::type());

					return v;
				}
			};

			template <typename C> 
			struct cast_helper<const C&>
			{ 
				static const C& cast(const any_t& a)
				{
					const C& v = System::Internal::default_value<C>::value();
					Omega::any_t::CastResult_t r = a.Coerce(v);
					if (r != any_t::castValid)
						throw_cast_exception(a,r,System::Internal::type_kind<const C&>::type());

					return v;
				}
			};

			template <> 
			struct cast_helper<bool_t&> 
			{ 
				static bool_t& cast(any_t& a) 
				{ 
					return a.GetBoolValue(); 
				} 
			};
			
			template <> 
			struct cast_helper<byte_t&> 
			{ 
				static byte_t& cast(any_t& a) 
				{ 
					return a.GetByteValue(); 
				} 
			};
			
			template <> 
			struct cast_helper<int16_t&> 
			{ 
				static int16_t& cast(any_t& a) 
				{ 
					return a.GetInt16Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<uint16_t&> 
			{ 
				static uint16_t& cast(any_t& a) 
				{ 
					return a.GetUInt16Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<int32_t&> 
			{ 
				static int32_t& cast(any_t& a) 
				{ 
					return a.GetInt32Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<uint32_t&> 
			{ 
				static uint32_t& cast(any_t& a) 
				{ 
					return a.GetUInt32Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<int64_t&> 
			{ 
				static int64_t& cast(any_t& a) 
				{ 
					return a.GetInt64Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<uint64_t&> 
			{ 
				static uint64_t& cast(any_t& a) 
				{ 
					return a.GetUInt64Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<float4_t&> 
			{ 
				static float4_t& cast(any_t& a) 
				{ 
					return a.GetFloat4Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<float8_t&> 
			{ 
				static float8_t& cast(any_t& a) 
				{ 
					return a.GetFloat8Value(); 
				} 
			};
			
			template <> 
			struct cast_helper<guid_t&> 
			{ 
				static guid_t& cast(any_t& a) 
				{ 
					return a.GetGuidValue();
				} 
			};
			
			template <> 
			struct cast_helper<string_t&> 
			{ 
				static string_t& cast(any_t& a) 
				{ 
					return a.GetStringValue(); 
				} 
			};
		}
	}
}

template <typename T>
inline T Omega::any_t::cast() const
{
	return System::Internal::cast_helper<T>::cast(*this);
}

template <typename T>
inline T Omega::any_t::cast()
{
	return System::Internal::cast_helper<T>::cast(*this);
}

inline Omega::bool_t& Omega::any_t::GetBoolValue()
{
	if (m_type != TypeInfo::typeBool)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<bool_t&>::type());

	return u.bVal;
}

inline Omega::byte_t& Omega::any_t::GetByteValue()
{
	if (m_type != TypeInfo::typeByte)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<byte_t&>::type());

	return u.byVal;
}

inline Omega::int16_t& Omega::any_t::GetInt16Value()
{
	if (m_type != TypeInfo::typeInt16)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<int16_t&>::type());

	return u.i16Val;
}

inline Omega::uint16_t& Omega::any_t::GetUInt16Value()
{
	if (m_type != TypeInfo::typeUInt16)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<uint16_t&>::type());

	return u.ui16Val;
}

inline Omega::int32_t& Omega::any_t::GetInt32Value()
{
	if (m_type != TypeInfo::typeInt32)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<int32_t&>::type());

	return u.i32Val;
}

inline Omega::uint32_t& Omega::any_t::GetUInt32Value()
{
	if (m_type != TypeInfo::typeUInt32)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<uint32_t&>::type());

	return u.ui32Val;
}

inline Omega::int64_t& Omega::any_t::GetInt64Value()
{
	if (m_type != TypeInfo::typeInt64)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<int64_t&>::type());

	return u.i64Val;
}

inline Omega::uint64_t& Omega::any_t::GetUInt64Value()
{
	if (m_type != TypeInfo::typeUInt64)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<uint64_t&>::type());

	return u.ui64Val;
}

inline Omega::float4_t& Omega::any_t::GetFloat4Value()
{
	if (m_type != TypeInfo::typeFloat4)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<float4_t&>::type());

	return u.fl4Val;
}

inline Omega::float8_t& Omega::any_t::GetFloat8Value()
{
	if (m_type != TypeInfo::typeFloat8)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<float8_t&>::type());

	return u.fl8Val;
}

inline Omega::guid_t& Omega::any_t::GetGuidValue()
{
	if (m_type != TypeInfo::typeGuid)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<guid_t&>::type());

	if (!u.pgVal)
		OMEGA_NEW(u.pgVal,guid_t(guid_t::Null()));

	return *u.pgVal;
}

inline Omega::string_t& Omega::any_t::GetStringValue()
{
	if (m_type != TypeInfo::typeString)
		System::Internal::throw_cast_exception(*this,any_t::castUnrelated,System::Internal::type_kind<string_t&>::type());

	return *u.pstrVal;
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

inline Omega::int64_t Omega::string_t::wcsto64(const string_t& str, size_t& end_pos, unsigned int base)
{
	return OOCore_wcsto64(str,end_pos,base);
}

inline Omega::uint64_t Omega::string_t::wcstou64(const string_t& str, size_t& end_pos, unsigned int base)
{
	return OOCore_wcstou64(str,end_pos,base);
}

inline Omega::float8_t Omega::string_t::wcstod(const string_t& str, size_t& end_pos)
{
	return OOCore_wcstod(str,end_pos);
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

inline void Omega::System::Internal::throw_cast_exception(const any_t& value, any_t::CastResult_t reason, const type_holder* typeDest)
{
	OOCore_ICastException_Throw(value,reason,typeDest);
}

#endif // OMEGA_ANY_INL_INCLUDED_
