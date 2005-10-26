//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_OBJECT_PROXY_H_INCLUDED_
#define _OOCORE_OBJECT_PROXY_H_INCLUDED_

#include "./Proxy_Marshaller.h"
#include "./Object_Impl.h"

class OOCore_ProxyStub_Handler;

#include "./OOCore_export.h"

class OOCore_Export OOCore_Object_Proxy_Base
{
protected:
	OOCore_Object_Proxy_Base(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* proxy);
	virtual ~OOCore_Object_Proxy_Base(void);

	int AddRef_i();
	int Release_i();
	int QueryInterface_i(const OOObj::GUID& iid, OOObj::Object** ppVal);
	
	OOCore_Proxy_Marshaller& marshaller(unsigned int method, bool sync = true);

protected:
	OOCore_ProxyStub_Handler* handler()
	{
		return m_handler;
	}

private:
	OOCore_Object_Proxy_Base() {};

	OOObj::cookie_t m_key;
	OOCore_ProxyStub_Handler* m_handler;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
};	

namespace OOObj
{

// This is the definition of the exported CreateProxy function
typedef int (*CreateProxy_Function)(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler, OOObj::Object** proxy);

template <class OBJECT>
class Object_Proxy : 
	public OOCore_Object_Proxy_Base,
	public OBJECT
{
public:
	Object_Proxy(const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler) :
		OOCore_Object_Proxy_Base(key,handler)
	{
	}

	virtual int AddRef()
	{
		return AddRef_i();
	}

	virtual int Release()
	{
		return Release_i();
	}

	virtual int QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
	{
		return QueryInterface_i(iid,ppVal);
	}
};

};

#endif // _OOCORE_OBJECT_PROXY_H_INCLUDED_
