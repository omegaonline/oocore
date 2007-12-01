///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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
		virtual guid_t ActualIID() = 0;
		virtual IException* Cause() = 0;
		virtual string_t Description() = 0;
		virtual string_t Source() = 0;

		inline static IException* Create(const string_t& desc, const string_t& source = L"", IException* pCause = 0);
	};

	interface INoInterfaceException : public IException
	{
		virtual guid_t GetUnsupportedIID() = 0;

		inline static INoInterfaceException* Create(const guid_t& iid, const string_t& source = L"");
	};
}

#ifdef OMEGA_HAS_UUIDOF

#define OMEGA_DEFINE_IID(n_space, type, guid) \
	interface __declspec(uuid(guid)) n_space::type;

#define OMEGA_UUIDOF(n) Omega::guid_t::FromUuidof(__uuidof(n))

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

#define OMEGA_DEFINE_IID(n_space, type, guid) \
	namespace Omega { namespace System { namespace MetaInfo { \
	template<> struct uid_traits<n_space::type> { static const guid_t& GetUID() { static const guid_t v = guid_t::FromString(OMEGA_WIDEN_STRING(guid) ); return v; } }; \
	} } }

#define OMEGA_UUIDOF(n)	(Omega::System::MetaInfo::uid_traits<n>::GetUID())

#endif

#define OMEGA_EXPORT_OID(name) \
	extern "C" OMEGA_EXPORT const Omega::guid_t name;

#define OMEGA_IMPORT_OID(name) \
	extern "C" OMEGA_IMPORT const Omega::guid_t name;

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	extern "C" const Omega::guid_t n_space::name = Omega::guid_t::FromString(OMEGA_WIDEN_STRING(guid));

OMEGA_DEFINE_IID(Omega, IObject, "{076DADE7-2D08-40f9-9AFA-AC883EB8BA9B}");
OMEGA_DEFINE_IID(Omega, IException, "{4847BE7D-A467-447c-9B04-2FE5A4576293}");

#if !defined(OMEGA_FUNCNAME)
	#define OMEGA_SOURCE_INFO    (Omega::string_t::Format(L"%hs(%u)",__FILE__,__LINE__))
#else
	#define OMEGA_SOURCE_INFO    (Omega::string_t::Format(L"%hs(%u): %ls",__FILE__,__LINE__,Omega::string_t(OMEGA_FUNCNAME,false).c_str()))
#endif

#define OMEGA_THROW(msg)     throw Omega::IException::Create(msg,OMEGA_SOURCE_INFO)

#endif // OOCORE_BASE_H_INCLUDED_
