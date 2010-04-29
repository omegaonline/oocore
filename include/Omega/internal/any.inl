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
		u.pstrVal = string_t::addref(static_cast<string_t::handle_t*>(rhs.u.pstrVal),false);
		break;
	case TypeInfo::typeGuid:
		OMEGA_NEW(u.pgVal,guid_t(*rhs.u.pgVal));
		break;

	case TypeInfo::typeObjectPtr:
	default:
		// Never going to happen ;)
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
	u.pstrVal = string_t::addref(val.m_handle,false);
}

inline Omega::any_t::any_t(const wchar_t* wsz, size_t length, bool copy) :
	m_type(TypeInfo::typeString)
{
	u.pstrVal = string_t::addref(string_t(wsz,length,copy).m_handle,false);	
}

inline Omega::any_t::any_t(const char* sz, bool bUTF8, size_t length) :
	m_type(TypeInfo::typeString)
{
	u.pstrVal = string_t::addref(string_t(sz,bUTF8,length).m_handle,false);
}

inline Omega::any_t::~any_t()
{
	try
	{
		clear();
	}
	catch (...)
	{}
}

inline void Omega::any_t::clear()
{
	if (m_type == TypeInfo::typeGuid)
		delete u.pgVal;
	else if (m_type == TypeInfo::typeString)
		string_t::release(static_cast<string_t::handle_t*>(u.pstrVal));
}

inline bool Omega::any_t::Equal(const any_t& rhs) const
{
	if (&rhs == this)
		return true;

	if (rhs.m_type != m_type)
		return false;
	
	switch (m_type)
	{
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
	case TypeInfo::typeString:
		return (string_t(static_cast<string_t::handle_t*>(u.pstrVal),true) == string_t(static_cast<string_t::handle_t*>(rhs.u.pstrVal),true));
	case TypeInfo::typeGuid:
		return (u.pgVal ? *u.pgVal : guid_t::Null()) == (rhs.u.pgVal ? *rhs.u.pgVal : guid_t::Null());
	//case TypeInfo::typeObjectPtr:
	default:
		// Never going to happen ;)
		return false;
	}
}

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

	// Check for wierd stuff...
	if (!std::numeric_limits<T>::is_specialized)
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
	
	//case TypeInfo::typeString:
	//case TypeInfo::typeObjectPtr:

	case TypeInfo::typeGuid:
	default:
		// Never going to happen ;)
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
	else if (m_type == TypeInfo::typeString && u.pstrVal)
	{
		if (guid_t::FromString(string_t(static_cast<string_t::handle_t*>(u.pstrVal),true),v))
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
		v = string_t(static_cast<string_t::handle_t*>(u.pstrVal),true);
		break;

	//case TypeInfo::typeObjectPtr:
		// QI for something and let that throw...

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
	case TypeInfo::typeGuid:
		v = (u.pgVal ? *u.pgVal != guid_t::Null() : false);
		break;
	case TypeInfo::typeString:
		v = (u.pstrVal ? !string_t(static_cast<string_t::handle_t*>(u.pstrVal),true).IsEmpty() : false);
		break;

	//case TypeInfo::typeObjectPtr:
		// Test for null

	default: 
		// Never going to happen ;)
		return any_t::castUnrelated;
	}

	return any_t::castValid;
}

template <typename T>
inline Omega::any_t::operator T() const
{
	typedef typename Omega::System::Internal::remove_const<T>::type non_constT;

	non_constT v = System::Internal::default_value<non_constT>::value();
	CastResult_t r = Coerce(v);
	if (r != any_t::castValid)
		throw ICastException::Create(*this,r,System::Internal::type_kind<non_constT>::type()->type);

	return v;
}

inline Omega::ICastException* Omega::ICastException::Create(const any_t& /*value*/, any_t::CastResult_t /*reason*/, TypeInfo::Type_t /*typeDest*/)
{
	// Make this an internal function that walks the type info correctly
	OMEGA_THROW(L"TODO!");
}

#endif // OMEGA_ANY_INL_INCLUDED_
