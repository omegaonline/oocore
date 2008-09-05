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

#include <OOCore/Preprocessor/base.h>

namespace Omega
{
	// The root of all objects
	interface IObject
	{
		virtual void AddRef() = 0;
		virtual void Release() = 0;
		virtual IObject* QueryInterface(const guid_t& iid) = 0;
	};

	interface IException : public IObject
	{
		virtual guid_t GetThrownIID() = 0;
		virtual IException* GetCause() = 0;
		virtual string_t GetDescription() = 0;
		virtual string_t GetSource() = 0;
	};

	interface ISystemException : public IException
	{
		virtual uint32_t GetErrorCode() = 0;

		inline static ISystemException* Create(uint32_t errno_val, const string_t& source = L"");
		inline static ISystemException* Create(const std::exception& e, const string_t& source = L"");
		inline static ISystemException* Create(const string_t& desc, const string_t& source = L"");
	};

	interface INoInterfaceException : public IException
	{
		virtual guid_t GetUnsupportedIID() = 0;

		inline static INoInterfaceException* Create(const guid_t& iid, const string_t& source = L"");
	};

	namespace TypeInfo
	{
		enum Types
		{
			typeUnknown = 0xFFFF,
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
			typeObject,

			typeConst = 0x100,
			typeArray = 0x200,
			typeReference = 0x400,
		};
		typedef uint32_t Types_t;

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

		interface ITypeInfo : public IObject
		{	
			virtual string_t GetName() = 0;
			virtual guid_t GetIID() = 0;
			virtual uint32_t GetMethodCount() = 0;
			virtual ITypeInfo* GetBaseType() = 0;
			virtual void GetMethodInfo(uint32_t method_idx, string_t& strName, MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, Types_t& return_type) = 0;
			virtual void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Types_t& type, ParamAttributes_t& attribs) = 0;
			virtual byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, ParamAttributes_t attrib) = 0;
		};
	}
}

#if defined(OMEGA_HAS_UUIDOF)

#define OMEGA_SET_GUIDOF(n_space, type, guid) \
	interface __declspec(uuid(guid)) n_space::type;

#define OMEGA_GUIDOF(type) Omega::guid_t::FromUuidof(__uuidof(type))

#elif defined(DOXYGEN)

/// Associate a guid_t value with a type
#define OMEGA_SET_GUIDOF(n_space, type, guid)

/// Return the guid_t value associated with a type
#define OMEGA_GUIDOF(type)

#else

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			template <typename T> struct uid_traits;

			template <typename T> struct uid_traits<T*>
			{
				static const guid_t& GetUID()
				{
					return uid_traits<T>::GetUID();
				}
			};
		}
	}
}

#define OMEGA_SET_GUIDOF(n_space, type, guid) \
	namespace Omega { namespace System { namespace MetaInfo { \
	template<> struct uid_traits<n_space::type> { static const guid_t& GetUID() { static const guid_t v = guid_t::FromString(OMEGA_WIDEN_STRING(guid) ); return v; } }; \
	} } }

#define OMEGA_GUIDOF(type)	(Omega::System::MetaInfo::uid_traits<type>::GetUID())

#endif

#if !defined(DOXYGEN)
#define OMEGA_EXPORT_OID(name) \
	extern "C" OMEGA_EXPORT const Omega::guid_t name;

#define OMEGA_IMPORT_OID(name) \
	extern "C" OMEGA_IMPORT const Omega::guid_t name;

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	extern "C" const Omega::guid_t n_space::name = Omega::guid_t::FromString(OMEGA_WIDEN_STRING(guid));

#else // DOXYGEN

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	const Omega::guid_t n_space::name = guid;

#endif

OMEGA_SET_GUIDOF(Omega, IObject, "{076DADE7-2D08-40f9-9AFA-AC883EB8BA9B}");
OMEGA_SET_GUIDOF(Omega, IException, "{4847BE7D-A467-447c-9B04-2FE5A4576293}");
OMEGA_SET_GUIDOF(Omega::TypeInfo, ITypeInfo, "{13EC66A0-D266-4682-9A47-6E2F178C40BD}");

#if defined(DOXYGEN)
	/// Return the current source filename and line as a string_t
	#define OMEGA_SOURCE_INFO
#elif !defined(OMEGA_FUNCNAME)
	#define OMEGA_SOURCE_INFO    (Omega::string_t::Format(L"%hs(%u)",__FILE__,__LINE__))
#else
	#define OMEGA_SOURCE_INFO    (Omega::string_t::Format(L"%hs(%u): %ls",__FILE__,__LINE__,Omega::string_t(OMEGA_FUNCNAME,false).c_str()))
#endif

#define OMEGA_THROW(e)           throw Omega::ISystemException::Create(e,OMEGA_SOURCE_INFO)

#endif // OOCORE_BASE_H_INCLUDED_
