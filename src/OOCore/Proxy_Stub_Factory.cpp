#include "./Proxy_Stub_Factory.h"
#include "./Binding.h"
#include "./ObjectManager.h"

namespace OOCore
{
namespace Impl
{
	class Object_Proxy : public OOObject::Object
	{
	public:
		static const OOObject::guid_t IID;
	};

	BEGIN_META_INFO(Object_Proxy)
	END_META_INFO()
};
};

DEFINE_IID(OOCore::Impl::Object_Proxy,45F040A3-5386-413e-AB21-7FA35EFCB7DD);

OOCORE_PS_BEGIN_PROXY_STUB_MAP(OOCore::Impl::Proxy_Stub_Factory::CreateProxyStub)
	OOCORE_PS_ENTRY_SHIM(OOCore::Impl::RemoteObjectFactory)
	OOCORE_PS_ENTRY_SHIM(OOCore::Impl::Object_Proxy)
END_META_INFO_MAP()

OOCore::Impl::Proxy_Stub_Factory::proxystub_node 
OOCore::Impl::Proxy_Stub_Factory::m_core_node = 
{
	ACE_DLL(), 
	&OOCore::Impl::Proxy_Stub_Factory::CreateProxy, 
	&OOCore::Impl::Proxy_Stub_Factory::CreateStub,
	&OOCore::Impl::Proxy_Stub_Factory::GetTypeInfo
};

OOCore::Impl::Proxy_Stub_Factory::Proxy_Stub_Factory(void)
{
	// This little bit of madness will register all the OOCore proxy/stubs automatically
	CreateProxyStub(4,0,OOObject::guid_t::NIL,0,OOCore::ProxyStubManager::cookie_t(),0,0,0,reinterpret_cast<const char*>(this));
}

OOCore::Impl::Proxy_Stub_Factory::~Proxy_Stub_Factory(void)
{
	for (std::map<OOObject::guid_t,proxystub_node*>::iterator i=m_dll_map.begin();i!=m_dll_map.end();++i)
	{
		if (i->second != &m_core_node)
			delete i->second;
	}
	m_dll_map.clear();
}

int 
OOCore::Impl::Proxy_Stub_Factory::create_proxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& cookie, OOObject::Object** proxy)
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
OOCore::Impl::Proxy_Stub_Factory::create_stub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOCore::Stub** ppStub)
{
	// Get the proxy/stub node
	proxystub_node* node;
	if (load_proxy_stub(iid,node) != 0)
		return -1;

	// Call CreateProxy
	if ((node->stub_fn)(manager,iid,obj,key,ppStub) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateStub failed: %m\n")),-1);
	
	return 0;
}

int 
OOCore::Impl::Proxy_Stub_Factory::create_type_info(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info)
{
	// Get the proxy/stub node
	proxystub_node* node;
	if (load_proxy_stub(iid,node) != 0)
		return -1;

	// Call CreateProxy
	if ((node->get_type_info_fn)(iid,type_info) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) GetTypeInfo failed: %m\n")),-1);
	
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

		// Bind to the GetTypeInfo function - C-style cast to please gcc
		new_node->get_type_info_fn = (OOCore::GetTypeInfo_Function)(new_node->dll.symbol(ACE_TEXT("GetTypeInfo")));
		if (new_node->proxy_fn == 0)
		{
			delete new_node;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) GetTypeInfo function missing from library\n")),-1);
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

int 
OOCore::Impl::Proxy_Stub_Factory::CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy) 
{
	return CreateProxyStub(0,manager,iid,0,key,proxy,0,0,0); 
}

int 
OOCore::Impl::Proxy_Stub_Factory::CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOCore::Stub** stub) 
{
	return CreateProxyStub(1,manager,iid,obj,key,0,stub,0,0); 
}

int 
OOCore::Impl::Proxy_Stub_Factory::GetTypeInfo(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info) 
{
	return CreateProxyStub(5,0,iid,0,OOCore::ProxyStubManager::cookie_t(),0,0,type_info,0); 
}

int OOCore_Export 
OOCore::GetTypeInfo(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info)
{
	return Impl::PROXY_STUB_FACTORY::instance()->create_type_info(iid,type_info);
}
