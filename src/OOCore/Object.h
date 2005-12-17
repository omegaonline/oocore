#ifndef OOCORE_OBJECT_H_INCLUDED_
#define OOCORE_OBJECT_H_INCLUDED_

#include "./Object_Types.h"
#include "./Guid.h"

#include "./OOCore_export.h"

#define DECLARE_IID(lib)			static const lib##_Export OOObject::guid_t IID;
#define DEFINE_IID(type,val)		const OOObject::guid_t type::IID(OOCore::Impl::create_guid(#val));

#define DECLARE_CLSID(cls,lib)		lib##_Export extern const OOObject::guid_t CLSID_##cls;
#define DEFINE_CLSID(cls,val)		const OOObject::guid_t CLSID_##cls(OOCore::Impl::create_guid(#val));

namespace OOObject
{
	class Object
	{
	public:
		virtual int32_t AddRef() = 0;
		virtual int32_t Release() = 0;
		virtual int32_t QueryInterface(const guid_t& iid, Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	// API functions
	OOCore_Export int Init(unsigned int threads = 1);
	OOCore_Export void Term();	
	OOCore_Export void* Alloc(const size_t size);
	OOCore_Export void Free(void* p);
	OOCore_Export OOObject::int32_t CreateObject(const guid_t& clsid, const guid_t& iid, Object** ppVal);
	
	template <class T>
	OOObject::int32_t CreateObject(const guid_t& clsid, T** ppVal)
	{
		return CreateObject(clsid,T::IID,reinterpret_cast<Object**>(ppVal));
	}
};

#endif // OOCORE_OBJECT_H_INCLUDED_
