#include "./Proxy_Stub_Factory.h"

#include "./Binding.h"

//extern "C" OOCore_Export int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy);
//extern "C" OOCore_Export int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOCore::Stub** stub);


int 
OOCore::Impl::Proxy_Stub_Factory::create_proxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& cookie, OOObject::Object** proxy)
{
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
OOCore::Impl::Proxy_Stub_Factory::create_stub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOCore::Stub** ppStub)
{
	// Get the proxy/stub node
	proxystub_node* node;
	if (load_proxy_stub(iid,node) != 0)
		return -1;

	// Call CreateProxy
	if ((node->stub_fn)(manager,iid,obj,key,ppStub) != 0)
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
		ACE_TString dll_name;
		if (BINDING::instance()->find(OOCore::Impl::guid_to_string(iid).c_str(),dll_name) != 0)
		{
			errno = ENOENT;
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No proxy/stub registered\n")),-1);
		}
			
		// Check if it's us
		if (dll_name==ACE_TEXT("OOCore"))
		{
			node = &m_core_node;
			return 0;
		}

		// Open the DLL
		proxystub_node* new_node;
		ACE_NEW_RETURN(new_node,proxystub_node,-1);

		if (new_node->dll.open(dll_name.c_str(),RTLD_NOW) != 0)
		{
			delete new_node;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to load library: %m\n")),-1);
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
#include "./ObjectManager.h"

#ifdef _DEBUG
#include "./Test.h"
#endif

 /*
BEGIN_PROXY_STUB_MAP(OOCore_Export,OOCore)
	PROXY_STUB_AUTO_ENTRY(OOCore::Impl::RemoteObjectFactory)

#ifdef _DEBUG
	PROXY_STUB_AUTO_ENTRY(OOCore::Test)
#endif

END_PROXY_STUB_MAP()
 */

// /*
// BEGIN_PROXY_STUB_MAP(OOCore_Export,OOCore) =
static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name); 

extern "C" OOCore_Export int RegisterLib(bool bRegister) 
{
	return CreateProxyStub((bRegister?2:3),0,OOObject::guid_t::NIL,0,OOObject::cookie_t(),0,0, "OOCore" ); 
}
extern "C" OOCore_Export int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy) 
{
    return CreateProxyStub(0,manager,iid,0,key,proxy,0,0); 
}
extern "C" OOCore_Export int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOCore::Stub** stub) 
{
    return CreateProxyStub(1,manager,iid,obj,key,0,stub,0); 
}
static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name) 
{
	if ((type==0 && proxy==0) || 
		(type==1 && stub==0) || 
		((type==2 || type==3) && dll_name==0) || 
		type<0 || 
		type>3) 
	{
		errno = EINVAL; 
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1); 
	}
	if (type==0) 
		*proxy=0; 

	if (type==1) 
		*stub=0;
	
// PROXY_STUB_AUTO_ENTRY(OOCore::Impl::RemoteObjectFactory) =
	if (type==2) 
		OOCore::RegisterProxyStub(OOCore::Impl::RemoteObjectFactory::IID, dll_name ); 
	else if (type==3)
		OOCore::UnregisterProxyStub(OOCore::Impl::RemoteObjectFactory::IID, dll_name ); 
    else if (iid==OOCore::Impl::RemoteObjectFactory::IID) 
	{ 
		if (type==0) 
			*proxy=OOCORE_PS_CREATE_AUTO_PROXY(OOCore::Impl::RemoteObjectFactory,manager,key); 
		else if (type==1) 
			*stub=OOCORE_PS_CREATE_AUTO_STUB(OOCore::Impl::RemoteObjectFactory,manager,key,obj); 
		goto end;
	}

// END_PROXY_STUB_MAP() =
end:
	if ((type==0 && *proxy==0) || (type==1 && *stub==0)) 
	{ 
		errno = ENOENT; 
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Proxy/Stub create failed\n")),-1); 
	}
    if (type==0) 
		(*proxy)->AddRef(); 
	if (type==1) 
		(*stub)->AddRef(); 
	return 0; 
}
	
// */

OOCore::Impl::Proxy_Stub_Factory::proxystub_node 
OOCore::Impl::Proxy_Stub_Factory::m_core_node = 
	{
		ACE_DLL(), 
		&CreateProxy, 
		&CreateStub
	};
