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

#ifndef OMEGA_ANY_H_INCLUDED_
#define OMEGA_ANY_H_INCLUDED_

namespace Omega
{
	// Forward declare friend types
	namespace System
	{
		namespace Internal
		{
			struct any_t_safe_type;
		}
	}

	class any_t
	{
	public:
		any_t();
		any_t(bool_t val);
		any_t(byte_t val);
		any_t(int16_t val);
		any_t(uint16_t val);
		any_t(int32_t val);
		any_t(uint32_t val);
		any_t(const int64_t& val);
		any_t(const uint64_t& val);
		any_t(float4_t val);
		any_t(const float8_t& val);
		any_t(const guid_t& val);

		// String constructors
		any_t(const string_t& val);
		any_t(const wchar_t* wsz, size_t length = string_t::npos, bool copy = true);
		any_t(const char* sz, bool bUTF8, size_t length = string_t::npos);

		template <typename I>
		any_t(I* pObj, const guid_t& iid = OMEGA_GUIDOF(I));

		any_t(const any_t& rhs);
		any_t& operator = (const any_t& rhs);

		~any_t();

		// Comparison operators
		template <typename T> bool operator == (T v) const { return equal(v); }
		template <typename T> bool operator != (T v) const { return !equal(v); }
				
		TypeInfo::Type GetType() const
		{
			return m_type;
		}

		enum CastResult
		{
			castValid = 0,
			castOverflow,
			castPrecisionLoss,
			castUnrelated
		};
		typedef uint32_t CastResult_t;

		// Attempt the cast, but don't throw
		template <typename T>
		CastResult_t Coerce(T& val) const;
		CastResult_t Coerce(bool_t& v) const;
		CastResult_t Coerce(guid_t& v) const;
		CastResult_t Coerce(string_t& v) const;

		template <typename I>
		CastResult_t Coerce(I*& val, const guid_t& iid = OMEGA_GUIDOF(I));

		// Perform a cast, and throw on failure (includes loss of precision, etc...)
		template <typename T> operator T() const;
		template <typename T> operator T&();

		// gcc gets baffled without these...
		operator const any_t&() const { return *this; }
		operator any_t&() { return *this; }
				
		// Explicit reference accessors
		bool_t& GetBoolValue();
		byte_t& GetByteValue();
		int16_t& GetInt16Value();
		uint16_t& GetUInt16Value();
		int32_t& GetInt32Value();
		uint32_t& GetUInt32Value();
		int64_t& GetInt64Value();
		uint64_t& GetUInt64Value();
		float4_t& GetFloat4Value();
		float8_t& GetFloat8Value();
		guid_t& GetGuidValue();
		string_t& GetStringValue();
				
	private:
		friend struct Omega::System::Internal::any_t_safe_type;

		struct obj_holder_base
		{
			virtual ~obj_holder_base() {}

			virtual const guid_t& get_iid() const = 0;
			virtual IObject* get_base_obj() const = 0;

			template <typename I>
			I* get_obj() const
			{
				return static_cast<I*>(get_base_obj());
			}

		private:
			obj_holder_base& operator = (const obj_holder_base&);
			obj_holder_base(const obj_holder_base&);
		};

		template <typename I>
		struct obj_holder : public obj_holder_base
		{
			obj_holder(I* pObj) : m_pObj(pObj)
			{
				if (m_pObj) m_pObj->AddRef();
			}

			virtual ~obj_holder()
			{
				if (m_pObj) m_pObj->Release();
			}

			virtual const guid_t& get_iid() const
			{
				return OMEGA_GUIDOF(I);
			}

			virtual IObject* get_base_obj() const
			{
				return m_pObj;
			}

		private:
			mutable I* m_pObj;
		};

		TypeInfo::Type m_type;
		union tagDisc
		{
			bool_t           bVal;
			byte_t           byVal;
			int16_t          i16Val;
			uint16_t         ui16Val;
			int32_t          i32Val;
			uint32_t         ui32Val;
			int64_t          i64Val;
			uint64_t         ui64Val;
			float4_t         fl4Val;
			float8_t         fl8Val;
			guid_t*          pgVal;
			string_t*        pstrVal;
			obj_holder_base* pobjVal;
		} u;

		// Helpers
		void swap(const any_t& rhs);
		void clear();
		bool equal(const any_t& rhs) const;
	};

	template <typename T>
	inline static T any_cast(const any_t& val)
	{
		return val;
	}

	template <typename T>
	inline static T any_cast(any_t& val)
	{
		return val;
	}

	interface ICastException : public IException
	{
	};

	namespace System
	{
		namespace Internal
		{
			template <> struct type_kind<any_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeAny, 0 };
					return &t;
				}
			};
		}
	}
}

#endif // OMEGA_ANY_H_INCLUDED_
