#ifndef OOCORE_BASE_H_INCLUDED_
#define OOCORE_BASE_H_INCLUDED_

#include <OOCOre/Preprocessor/base.h>

// In order to link properly, we avoid __declspec(selectany) because it is just not portable
// So, #include the relevant classes, having #define OMEGA_GUID_LINK_HERE first in ONE souce file only
//
// E.g source1.c
//
// #define OMEGA_GUID_LINK_HERE
// #include <OOCore/OOCore.h>
//
// and your project should now link!

#define OMEGA_DECLARE_IID(type) \
	extern const Omega::guid_t OMEGA_CONCAT(IID_,type); 

#define OMEGA_DECLARE_IID_TRAITS(n_space,type) \
	namespace Omega { namespace MetaInfo { \
	template<> struct iid_traits<n_space::type> { inline static const guid_t& GetIID() { return n_space::OMEGA_CONCAT(IID_,type); } }; \
	template<> struct iid_traits<n_space::type*> { inline static const guid_t& GetIID() { return n_space::OMEGA_CONCAT(IID_,type); } }; \
	} }

#if (defined(OMEGA_GUID_LINK_HERE))
#define OMEGA_DEFINE_IID(n_space, type, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	const Omega::guid_t n_space::OMEGA_CONCAT(IID_,type) = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }; \
	OMEGA_DECLARE_IID_TRAITS(n_space,type)

#define OMEGA_DEFINE_OID(n_space,name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	namespace n_space { extern const ::Omega::guid_t name; } \
	const Omega::guid_t n_space::name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } };

#else

#define OMEGA_DEFINE_IID(n_space, type, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	OMEGA_DECLARE_IID_TRAITS(n_space,type)

#define OMEGA_DEFINE_OID(n_space, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	namespace n_space { extern const ::Omega::guid_t name; }
#endif

namespace Omega
{
	namespace MetaInfo
	{
		template <typename T> struct iid_traits;
	}

	// The root of all objects
	interface IObject
	{
		virtual void AddRef() = 0;
		virtual void Release() = 0;
		virtual IObject* QueryInterface(const guid_t& iid) = 0;
	};
	OMEGA_DECLARE_IID(IObject);

	interface IException : public IObject
	{
		virtual guid_t ActualIID() = 0;
		virtual IException* Cause() = 0;
		virtual string_t Description() = 0;
		virtual string_t Source() = 0;
	
		static void Throw(const char_t* desc, const char_t* source = 0, IException* pCause = 0);
	};
	OMEGA_DECLARE_IID(IException);

	interface INoInterfaceException : public IException
	{
		virtual guid_t GetUnsupportedIID() = 0;

		static void Throw(const guid_t& iid, const char_t* source = 0);
	};
	OMEGA_DECLARE_IID(INoInterfaceException);

}	

OMEGA_DEFINE_IID(Omega, IObject, 0x6d2040c, 0xba2d, 0x45f5, 0x94, 0x4e, 0x32, 0x9f, 0x69, 0xa8, 0xd3, 0x40);
OMEGA_DEFINE_IID(Omega, IException, 0x8ce2dc6, 0x2234, 0x4a6f, 0xa0, 0x15, 0x49, 0x18, 0x69, 0x60, 0x9, 0xbe);

#if !defined(OMEGA_FUNCNAME)
	#define OMEGA_SOURCE_INFO    static_cast<const Omega::char_t*>(Omega::string_t::Format("%s(%u)",__FILE__,__LINE__))
#else
	#define OMEGA_SOURCE_INFO    static_cast<const Omega::char_t*>(Omega::string_t::Format("%s(%u): %s",__FILE__,__LINE__,OMEGA_FUNCNAME))
#endif

#define OMEGA_THROW(msg)     Omega::IException::Throw(msg,OMEGA_SOURCE_INFO)
#define OMEGA_THROW2(msg,pE) Omega::IException::Throw(msg,OMEGA_SOURCE_INFO,pE)

#endif // OOCORE_BASE_H_INCLUDED_
