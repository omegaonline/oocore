#include "./Proxy_Stub_Factory.h"

#include "./Binding.h"

OOCore::Impl::Proxy_Stub_Factory::proxystub_node OOCore::Impl::Proxy_Stub_Factory::m_core_node = {ACE_DLL(), &Impl::Proxy_Stub_Factory::CreateProxy, &Impl::Proxy_Stub_Factory::CreateStub};

int 
OOCore::Impl::Proxy_Stub_Factory::create_proxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& cookie, OOObject::Object** proxy)
{
	// TODO optimize this by reusing Proxies

	// Get the proxy/stub node
	proxystub_node* node;
	if (load_proxy_stub(iid,node) != 0)
		return -1;

	// Call CreateProxy
	if ((node->proxy_fn)(manager,iid,cookie,proxy) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateProxy failed\n")),-1);
	
	return 0;
}

int 
OOCore::Impl::Proxy_Stub_Factory::create_stub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, OOCore::Stub** ppStub)
{
	// TODO optimize this by reusing Stubs

	// Get the proxy/stub node
	proxystub_node* node;
	if (load_proxy_stub(iid,node) != 0)
		return -1;

	// Call CreateProxy
	if ((node->stub_fn)(manager,iid,obj,ppStub) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateStub failed\n")),-1);
	
	return 0;
}

int 
OOCore::Impl::Proxy_Stub_Factory::load_proxy_stub(const OOObject::guid_t& iid, proxystub_node*& node)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<OOObject::guid_t,proxystub_node*>::const_iterator i=m_dll_map.find(iid);
	if (i==m_dll_map.end())
	{
		// Find the stub DLL name
		ACE_NS_WString dll_name;
		if (BINDING::instance()->find(OOCore::Impl::guid_to_string(iid).c_str(),dll_name) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No proxy/stub registered\n")),-1);
			
		// Check if it's us
		if (dll_name==ACE_TEXT_WIDE("OOCore"))
		{
			node = &m_core_node;
			return 0;
		}

		// Open the DLL
		proxystub_node* new_node;
		ACE_NEW_RETURN(new_node,proxystub_node,-1);

		if (new_node->dll.open(ACE_TEXT_WCHAR_TO_TCHAR(dll_name.c_str())) != 0)
		{
			delete new_node;
			return -1;
		}
		
		// Bind to the CreateStub function - C-style cast to please gcc
		new_node->stub_fn = (OOCore::CreateStub_Function)(new_node->dll.symbol(ACE_TEXT("CreateStub")));
		if (new_node->stub_fn == 0)
		{
			delete new_node;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) CreateStub function missing from library\n")),-1);
		}

		// Bind to the CreateProxy function - C-style cast to please gcc
		new_node->proxy_fn = (OOCore::CreateProxy_Function)(new_node->dll.symbol(ACE_TEXT("CreateProxy")));
		if (new_node->proxy_fn == 0)
		{
			delete new_node;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) CreateProxy function missing from library\n")),-1);
		}

		// Add the node info to the map
		if (!m_dll_map.insert(std::map<OOObject::guid_t,proxystub_node*>::value_type(iid,new_node)).second)
		{
			delete new_node;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to bind proxy/stub info\n")),-1);
		}

		node = new_node;
	}
	else
	{
		node = i->second;
	}

	return 0;
}

#include "./OOCore_PS.h"

#ifdef _DEBUG
#include "./Test.h"
#endif

int 
OOCore::Impl::Proxy_Stub_Factory::CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy)
{
	if (iid==OOCore::RemoteObjectFactory::IID)
		*proxy = CREATE_AUTO_PROXY(OOCore::RemoteObjectFactory,manager,key);

#ifdef OOCORE_TEST_H_INCLUDED_
	else if (iid==OOCore::Test::IID)
		*proxy = CREATE_AUTO_PROXY(OOCore::Test,manager,key);
#endif

	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Proxy IID\n")),-1);
	
	if (*proxy==0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Proxy create failed\n")),-1);

	(*proxy)->AddRef();
	return 0;
}

int 
OOCore::Impl::Proxy_Stub_Factory::CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, OOCore::Stub** stub)
{
	if (iid==OOCore::RemoteObjectFactory::IID)
		*stub = CREATE_AUTO_STUB(OOCore::RemoteObjectFactory,manager,obj);
		
#ifdef OOCORE_TEST_H_INCLUDED_
	else if (iid==OOCore::Test::IID)
		*stub = CREATE_AUTO_STUB(OOCore::Test,manager,obj);
#endif

	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Stub IID\n")),-1);
	
	if (*stub==0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Stub create failed\n")),-1);

	(*stub)->AddRef();
	return 0;
}