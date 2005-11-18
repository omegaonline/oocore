#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

#include <map>
#include <set>

#include "./OOCore_Util.h"

namespace OOCore
{

class OOCore_Export ObjectManager :
	public Object_Root,
	public RemoteObjectFactory,
	public ProxyStubManager
{
public:
	ObjectManager(void);
	
	int Open(Transport* transport, OOObj::bool_t AsServer);
	int Close();
	int ProcessMessage(InputStream* input);
	OOObj::int32_t CreateRemoteObject(const OOObj::string_t class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal);

private:
	virtual ~ObjectManager(void);

	struct response_wait
	{
		response_wait(ObjectManager* t, OOObj::uint32_t k, InputStream** i) :
			pThis(t), trans_id(k), input(i)
		{ }

		Object_Ptr<ObjectManager> pThis;
		OOObj::uint32_t trans_id;
		InputStream** input;
	};

	bool m_bServer;
	ACE_Thread_Mutex m_lock;
	ACE_Active_Map_Manager<Stub*> m_stub_map;
	Object_Ptr<RemoteObjectFactory> m_ptrRemoteFactory;
	Object_Ptr<Transport> m_ptrTransport;
	std::map<OOObj::uint32_t,Object_Ptr<InputStream> > m_response_map;
	OOObj::uint32_t m_next_trans_id;
	std::set<OOObj::uint32_t> m_transaction_set;
	
	int connect();
	int accept();
	int process_request(InputStream* input);
	int process_response(InputStream* input);
	int process_connect(InputStream* input);
	int remove_stub(const OOObj::cookie_t& key);
	bool await_response_i(OOObj::uint32_t trans_id, InputStream** input);

	static bool await_response(void* p);
	static bool await_connect(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::RemoteObjectFactory
public:
	OOObj::int32_t CreateObject(const OOObj::string_t class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal);
	OOObj::int32_t SetReverse(RemoteObjectFactory* pRemote);

// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** ppVal);
	int CreateStub(const OOObj::guid_t& iid, OOObj::Object* pObj, OutputStream* output);
	int CreateRequest(const OOObj::cookie_t& proxy_key, OOObj::uint32_t method, OOObj::bool_t sync, OOObj::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObj::uint32_t trans_id);
	int SendAndReceive(OutputStream* output, OOObj::uint32_t trans_id, InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
