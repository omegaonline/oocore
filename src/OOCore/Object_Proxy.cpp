#include "./Object_Proxy.h"

#include "./ProxyStub_Handler.h"
#include "./Object_Marshaller.h"

OOCore_Object_Proxy_Base::OOCore_Object_Proxy_Base(const OOObj::cookie_t& key, 
												   OOCore_ProxyStub_Handler* handler) :
    m_key(key),
	m_handler(handler),
	m_refcount(0)
{
	m_handler->addref();
}

OOCore_Object_Proxy_Base::~OOCore_Object_Proxy_Base(void)
{
	m_handler->release();
}

OOCore_Proxy_Marshaller& OOCore_Object_Proxy_Base::marshaller(unsigned int method, bool sync)
{
	return m_handler->create_marshaller(m_key,method,sync);
}

int OOCore_Object_Proxy_Base::AddRef_i()
{
	++m_refcount;

	return 0;
}

int OOCore_Object_Proxy_Base::Release_i()
{
	if (--m_refcount == 0)
	{
		marshaller(1)();
		delete this;
	}

	return 0;
}

int OOCore_Object_Proxy_Base::QueryInterface_i(const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	return (marshaller(2) << iid << OOCore_Object_Marshaller(iid,ppVal))();
}
