///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_BASE_H_INCLUDED_
#define OOCORE_BASE_H_INCLUDED_

#include "cpp/pre_base.h"

#if !defined(interface)
#define interface struct
#endif

namespace Omega
{
	// The root of all objects
	interface IObject
	{
		virtual void AddRef() = 0;
		virtual void Release() = 0;
		virtual IObject* QueryInterface(const guid_t& iid) = 0;

    protected:
        virtual ~IObject() {}
	};

	interface IException : public IObject
	{
		virtual void Throw() = 0;
		virtual guid_t GetThrownIID() = 0;
		virtual IException* GetCause() = 0;
		virtual string_t GetDescription() = 0;
		virtual string_t GetSource() = 0;
	};

	interface ISystemException : public IException
	{
		virtual uint32_t GetErrorCode() = 0;

		static ISystemException* Create(uint32_t errno_val, const string_t& source = string_t());
		static ISystemException* Create(const std::exception& e, const string_t& source = string_t());
		static ISystemException* Create(const string_t& desc, const string_t& source = string_t());
	};

	interface INoInterfaceException : public IException
	{
		virtual guid_t GetUnsupportedIID() = 0;

		static INoInterfaceException* Create(const guid_t& iid, const string_t& source = string_t());
	};

	interface ITimeoutException : public IException
	{
		static ITimeoutException* Create();
	};

	namespace TypeInfo
	{
		enum Type
		{
			typeVoid = 0,
			typeBool,
			typeByte,
			typeInt16,
			typeUInt16,
			typeInt32,
			typeUInt32,
			typeInt64,
			typeUInt64,
			typeFloat4,
			typeFloat8,
			typeString,
			typeGuid,
			typeObject
		};
		enum Modifier
		{
			// type modifiers imply a 'next' entry in TypeDetail_t
			modifierConst = 0x10,
			modifierPointer,
			modifierReference,

			// STL collection types
			modifierList = 0x18,
			modifierSet,
			modifierMap = 0x20,
			modifierMultimap
		};
		typedef byte_t Type_t;

		typedef std::vector<Type_t> TypeDetail_t;

		enum MethodAttributes
		{
			Synchronous = 0,
			Asynchronous = 1,
			Unreliable = 2,
			Encrypted = 4
		};
		typedef uint16_t MethodAttributes_t;

		enum ParamAttributes
		{
			attrIn = 1,
			attrOut = 2,
			attrInOut = (attrIn | attrOut),
			attrIid_is = 4,
			attrSize_is = 8
		};
		typedef byte_t ParamAttributes_t;

		interface IInterfaceInfo : public IObject
		{	
			virtual string_t GetName() = 0;
			virtual guid_t GetIID() = 0;
			virtual uint32_t GetMethodCount() = 0;
			virtual IInterfaceInfo* GetBaseType() = 0;
			virtual void GetMethodInfo(uint32_t method_idx, string_t& strName, MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, TypeDetail_t& return_type) = 0;
			virtual void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, TypeDetail_t& type, ParamAttributes_t& attribs) = 0;
			virtual byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, ParamAttributes_t attrib) = 0;
		};
	}
}

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			template <typename T> struct uid_traits;

			template <typename T> struct uid_traits<T*>
			{
				static const guid_t& GetUID()
				{
					return uid_traits<T>::GetUID();
				}
			};

			template <typename T> struct uid_traits<const T>
			{
				static const guid_t& GetUID()
				{
					return uid_traits<T>::GetUID();
				}
			};

			template <typename T> struct uid_traits<T&>
			{
				static const guid_t& GetUID()
				{
					return uid_traits<T>::GetUID();
				}
			};
		}
	}
}

#if defined(OMEGA_HAS_UUIDOF)

#define OMEGA_SET_GUIDOF(n_space, type, guid) \
	interface __declspec(uuid(guid)) n_space::type; \
	namespace Omega { namespace System { namespace Internal { \
	template<> struct uid_traits<n_space::type> { static const guid_t& GetUID() { static const guid_t v(__uuidof(n_space::type)); return v; } }; \
	} } }

#elif defined(DOXYGEN)

/// Associate a guid_t value with a type
#define OMEGA_SET_GUIDOF(n_space, type, guid)

#else

#define OMEGA_SET_GUIDOF(n_space, type, guid) \
	namespace Omega { namespace System { namespace Internal { \
	template<> struct uid_traits<n_space::type> { static const guid_t& GetUID() { static const guid_t v = guid_t::FromString(OMEGA_WIDEN_STRING(guid)); return v; } }; \
	} } }

#endif

/// Return the guid_t value associated with a type
#define OMEGA_GUIDOF(type)     Omega::System::Internal::uid_traits<type>::GetUID()

#if !defined(DOXYGEN)

#define OMEGA_EXPORT_OID(name) \
	extern "C" OMEGA_EXPORT const Omega::guid_t name;

#define OMEGA_IMPORT_OID(name) \
	extern "C" OMEGA_IMPORT const Omega::guid_t name;

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	extern "C" const Omega::guid_t n_space::name = Omega::guid_t::FromString(OMEGA_WIDEN_STRING(guid));

#else // DOXYGEN

#define OMEGA_EXPORT_OID(name) \
	extern "C" const Omega::guid_t name;

#define OMEGA_IMPORT_OID(name) \
	extern "C" const Omega::guid_t name;

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	const Omega::guid_t n_space::name = guid;

#endif

OMEGA_SET_GUIDOF(Omega, IObject, "{01010101-0101-0101-0101-010101010101}");
OMEGA_SET_GUIDOF(Omega, IException, "{4847BE7D-A467-447c-9B04-2FE5A4576293}");

#if defined(DOXYGEN)
/// Return the current source filename and line as a string_t
#define OMEGA_SOURCE_INFO
#elif !defined(OMEGA_FUNCNAME)
#define OMEGA_SOURCE_INFO    (L"{0}({1})" % Omega::string_t(__FILE__,false) % __LINE__)
#else
#define OMEGA_SOURCE_INFO    (L"{0}({1}): {2}" % Omega::string_t(__FILE__,false) % __LINE__ % Omega::string_t(OMEGA_FUNCNAME,false))
#endif

#define OMEGA_THROW(e)       throw Omega::ISystemException::Create(e,OMEGA_SOURCE_INFO)

#endif // OOCORE_BASE_H_INCLUDED_
