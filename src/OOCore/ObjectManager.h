#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

#include <map>
#include <set>

#include "./OOCore_Impl.h"

namespace OOCore
{

class OOCore_Export ObjectManager :
	public Object_Root,
	public RemoteObjectFactory,
	public ProxyStubManager
{
public:
	ObjectManager(void);
	
	int Open(Transport* transport, OOObject::bool_t AsServer);
	int Close();
	int ProcessMessage(InputStream* input);
	OOObject::int32_t CreateRemoteObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

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

	bool m_bServer;
	ACE_Thread_Mutex m_lock;
	ACE_Active_Map_Manager<Stub*> m_stub_map;
	Object_Ptr<RemoteObjectFactory> m_ptrRemoteFactory;
	Object_Ptr<Transport> m_ptrTransport;
	std::map<OOObject::uint32_t,Object_Ptr<InputStream> > m_response_map;
	OOObject::uint32_t m_next_trans_id;
	std::set<OOObject::uint32_t> m_transaction_set;
	
	int connect();
	int accept();
	int process_request(Impl::InputStream_Wrapper& input);
	int process_response(Impl::InputStream_Wrapper& input);
	int process_connect(Impl::InputStream_Wrapper& input);
	int remove_stub(const OOObject::cookie_t& key);
	bool await_response_i(OOObject::uint32_t trans_id, InputStream** input);

	static bool await_response(void* p);
	static bool await_connect(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::RemoteObjectFactory
public:
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
	OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote);

// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** ppVal);
	int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OutputStream* output);
	int CreateRequest(const OOObject::cookie_t& proxy_key, OOObject::uint32_t method, OOObject::bool_t sync, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	int SendAndReceive(OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
