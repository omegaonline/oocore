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

		inline static IException* Create(const char_t* desc, const char_t* source = 0, IException* pCause = 0);
	};

	interface INoInterfaceException : public IException
	{
		virtual guid_t GetUnsupportedIID() = 0;

		inline static INoInterfaceException* Create(const guid_t& iid, const char_t* source = 0);
	};
}

#ifdef OMEGA_HAS_UUIDOF

#define OMEGA_DEFINE_IID(n_space, type, guid) \
	interface __declspec(uuid(guid)) n_space::type;

#define OMEGA_UUIDOF(n) (*reinterpret_cast<const Omega::guid_t*>(&__uuidof(n)))

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
	template<> struct uid_traits<n_space::type> { static const guid_t& GetUID() { static const guid_t v = guid_t::FromString( guid ); return v; } }; \
	} } }

#define OMEGA_UUIDOF(n)	(Omega::System::MetaInfo::uid_traits<n>::GetUID())

#endif

#define OMEGA_EXPORT_OID(name) \
	extern "C" OMEGA_EXPORT const Omega::guid_t name;

#define OMEGA_IMPORT_OID(name) \
	extern "C" OMEGA_IMPORT const Omega::guid_t name;

#define OMEGA_DEFINE_OID(n_space, name, guid) \
	extern "C" const Omega::guid_t n_space::name = Omega::guid_t::FromString(guid);

OMEGA_DEFINE_IID(Omega, IObject, "{076DADE7-2D08-40f9-9AFA-AC883EB8BA9B}");
OMEGA_DEFINE_IID(Omega, IException, "{4847BE7D-A467-447c-9B04-2FE5A4576293}");

#if !defined(OMEGA_FUNCNAME)
	#define OMEGA_SOURCE_INFO    static_cast<const Omega::char_t*>(Omega::string_t::Format("%s(%u)",__FILE__,__LINE__))
#else
	#define OMEGA_SOURCE_INFO    static_cast<const Omega::char_t*>(Omega::string_t::Format("%s(%u): %s",__FILE__,__LINE__,OMEGA_FUNCNAME))
#endif

#define OMEGA_THROW(msg)     throw Omega::IException::Create(msg,OMEGA_SOURCE_INFO)

#endif // OOCORE_BASE_H_INCLUDED_
