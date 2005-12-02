#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

#include <map>
#include <set>

#include "./OOCore_Impl.h"
#include "./ProxyStub.h"

namespace OOCore
{
namespace Impl
{
	class RemoteObjectFactory : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t CreateRemoteObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;
		virtual OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;
		virtual OOObject::int32_t AddObjectFactory(const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory) = 0;
		virtual OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid) = 0;
	
		DECLARE_IID(OOCore_Export);
	};

	BEGIN_AUTO_PROXY_STUB(RemoteObjectFactory)
		METHOD(CreateRemoteObject,3,((in),const OOObject::guid_t&,clsid,(in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,ppVal))
		METHOD(SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
		METHOD(AddObjectFactory,2,((in),const OOObject::guid_t&,clsid,(in)(iid_is(OOCore::ObjectFactory::IID)),OOCore::ObjectFactory*,pFactory))
		METHOD(RemoveObjectFactory,1,((in),const OOObject::guid_t&,clsid))
	END_AUTO_PROXY_STUB()
};

class OOCore_Export ObjectManager :
	public Object_Root,
	public Impl::RemoteObjectFactory,
	public ProxyStubManager
{
public:
	ObjectManager(void);
	
	int Open(Transport* transport, const bool AsAcceptor);
	int Close();
	int ProcessMessage(InputStream* input);
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

private:
	virtual ~ObjectManager(void);

	struct response_wait
	{
		response_wait(ObjectManager* t, OOObject::uint32_t k, InputStream** i) :
			pThis(t), trans_id(k), input(i)
		{ }

		Object_Ptr<ObjectManager> pThis;
		OOObject::uint32_t trans_id;
		InputStream** input;
	};

	bool m_bIsAcceptor;
	ACE_Recursive_Thread_Mutex m_lock;
	std::map<OOObject::cookie_t,OOObject::Object*> m_proxy_map;
	std::map<OOObject::Object*,OOObject::cookie_t> m_rev_proxy_map;
	std::map<OOObject::Object*,OOObject::cookie_t> m_stub_obj_map;
	std::map<OOObject::cookie_t,OOObject::Object*> m_rev_stub_obj_map;
	ACE_Active_Map_Manager<Stub*> m_stub_map;
	Object_Ptr<Impl::RemoteObjectFactory> m_ptrRemoteFactory;
	Object_Ptr<Transport> m_ptrTransport;
	std::map<OOObject::uint32_t,Object_Ptr<InputStream> > m_response_map;
	OOObject::uint32_t m_next_trans_id;
	std::set<OOObject::uint32_t> m_transaction_set;
	
	int connect();
	int accept();
	int process_request(Impl::InputStream_Wrapper& input);
	int process_response(Impl::InputStream_Wrapper& input);
	int process_connect(Impl::InputStream_Wrapper& input);
	bool await_response_i(OOObject::uint32_t trans_id, InputStream** input);

	static bool await_response(void* p);
	static bool await_connect(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(Impl::RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::Impl::RemoteObjectFactory
public:
	OOObject::int32_t CreateRemoteObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
	OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote);
	OOObject::int32_t AddObjectFactory(const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory);
	OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
	
// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** ppVal);
	int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOObject::cookie_t* key);
	int ReleaseProxy(const OOObject::cookie_t& key);
	int ReleaseStub(const OOObject::cookie_t& key);	
	int CreateRequest(Marshall_Flags flags, const OOObject::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	int SendAndReceive(Marshall_Flags flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
