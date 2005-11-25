#include "./ProxyStub.h"

Marshall_A::ProxyStub_Base::ProxyStub_Base(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) :
	m_bStub(false),
    m_key(key),
	m_manager(manager),
	m_refcount(0)
{
}

Marshall_A::ProxyStub_Base::ProxyStub_Base(OOCore::ProxyStubManager* manager, OOObj::Object* obj) :
	m_bStub(true),
	m_object(obj),
 	m_manager(manager),
	m_refcount(0)
{
	add_delegate(offsetof(ProxyStub_Base,addref_id),NULL);
	add_delegate(offsetof(ProxyStub_Base,release_id),NULL);
	add_delegate(offsetof(ProxyStub_Base,qi_id),(new Delegate::D2<const OOObj::guid_t&,OOObj::Object**>())->bind<OOObj::Object,&OOObj::Object::QueryInterface>(obj));
}

Marshall_A::ProxyStub_Base::~ProxyStub_Base()
{
	for (std::map<size_t,Delegate::Base*>::iterator i = dispatch_tbl.begin();i!=dispatch_tbl.end();++i)
		delete i->second;
}

OOObj::int32_t 
Marshall_A::ProxyStub_Base::AddRef_i()
{
	++m_refcount;
	return 0;
}

OOObj::int32_t 
Marshall_A::ProxyStub_Base::Release_i()
{
	return (m_bStub ? Release_stub() : Release_proxy());
}

OOObj::int32_t 
Marshall_A::ProxyStub_Base::QueryInterface_i(const OOObj::guid_t& iid, OOObj::Object** ppVal)
{
	return (method(offsetof(ProxyStub_Base,qi_id)) << iid << object(iid,ppVal))();
}

OOObj::int32_t 
Marshall_A::ProxyStub_Base::Release_proxy()
{
	if (--m_refcount == 0)
	{
		method(offsetof(ProxyStub_Base,release_id))();
		delete this;
	}

	return 0;
}

OOObj::int32_t 
Marshall_A::ProxyStub_Base::Release_stub()
{
	if (--m_refcount == 0)
		delete this;

	return 0;
}

Marshall_A::Proxy_Marshaller
Marshall_A::ProxyStub_Base::method(size_t m, OOObj::bool_t sync)
{
	OOObj::uint32_t method = static_cast<OOObj::uint32_t>(m);
	OOObj::uint32_t trans_id;
	OOCore::Object_Ptr<OOCore::OutputStream> output;

	if (m_bStub || m_manager->CreateRequest(m_key,method,sync,&trans_id,&output) != 0)
		return Proxy_Marshaller();
	
	return Proxy_Marshaller(m_manager,sync,output,trans_id);
}

Marshall_A::Object_Marshaller 
Marshall_A::ProxyStub_Base::object(const OOObj::guid_t& iid, OOObj::Object** ppObj, bool in)
{
	return Object_Marshaller(iid,ppObj,in);
}

Marshall_A::Object_Marshaller 
Marshall_A::ProxyStub_Base::object(const OOObj::guid_t& iid, OOObj::Object* pObj)
{
	return Object_Marshaller(iid,pObj);
}

int 
Marshall_A::ProxyStub_Base::Invoke_i(OOObj::uint32_t method, OOObj::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
{
	Stub_Marshaller mshl(m_manager,input);

	int inv_code = invoke(method,ret_code,mshl);
	if (inv_code!=0 && inv_code!=1)
		return -1;

	if (ret_code==0)
	{
		// Write results
		if (mshl.output_response(output) != 0)
			return -1;
	}

	return inv_code;
}

int 
Marshall_A::ProxyStub_Base::invoke(OOObj::uint32_t method, OOObj::int32_t& ret_code, Stub_Marshaller& mshl)
{
	ret_code = 0;

	if (method==offsetof(ProxyStub_Base,release_id))
	{
		// Special for Release()
		ret_code = 0;
		return 1;
	}
	else
	{
		std::map<size_t,Delegate::Base*>::iterator i=dispatch_tbl.find(method);
		if (i==dispatch_tbl.end() || !i->second)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid method index %ul\n"),method),-1);

		ret_code = i->second->invoke(mshl);
	}

	return 0;
}

void 
Marshall_A::ProxyStub_Base::add_delegate(size_t method, Delegate::Base* del)
{
	if (!dispatch_tbl.insert(std::map<size_t,Delegate::Base*>::value_type(method,del)).second)
	{
		// If this fails we need to redesign the dispatch impl!
		// A method has been added twice, or has the same offset!
		ACE_OS::abort();
	}
}
