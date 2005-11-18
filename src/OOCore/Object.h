#ifndef OOCORE_OBJECT_H_INCLUDED_
#define OOCORE_OBJECT_H_INCLUDED_

#include "./Object_Types.h"
#include "./GUID.h"

#include "./OOCore_export.h"

#define DECLARE_IID(export)			static const export OOObj::guid_t IID;
#define DEFINE_IID(type,val)		const OOObj::guid_t type::IID(Impl::create_guid(#val));

namespace OOObj
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
	OOCore_Export void* Alloc(size_t size);
	OOCore_Export void Free(void* p);
	OOCore_Export OOObj::int32_t CreateObject(const string_t service_name, const guid_t& iid, Object** ppVal);
	OOCore_Export int RegisterProxyStub(const OOObj::guid_t& iid, const char* dll_name);
	
	template <class T>
	int CreateObject(const string_t service_name, T** ppVal)
	{
		return CreateObject(service_name,T::IID,reinterpret_cast<Object**>(ppVal));
	}
};

#endif // OOCORE_OBJECT_H_INCLUDED_
