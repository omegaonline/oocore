#ifndef OOCORE_OBJECT_H_INCLUDED_
#define OOCORE_OBJECT_H_INCLUDED_

#include "./Object_Types.h"
#include "./GUID.h"

#include "./OOCore_export.h"

#define DECLARE_IID(export)			static const export OOObject::guid_t IID;
#define DEFINE_IID(type,val)		const OOObject::guid_t type::IID(OOCore::Impl::create_guid(#val));

#define DECLARE_CLSID(export,cls)	export extern const OOObject::guid_t cls;
#define DEFINE_CLSID(cls,val)		const OOObject::guid_t cls(OOCore::Impl::create_guid(#val));

namespace OOObject
{
	class Object
	{
	public:
		virtual int32_t AddRef() = 0;
		virtual int32_t Release() = 0;
		virtual int32_t QueryInterface(const guid_t& iid, Object** ppVal) = 0;

		DECLARE_IID(OOCore_Export);
	};

	// API functions
	OOCore_Export int Init();
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
