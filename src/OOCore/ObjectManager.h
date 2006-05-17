#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

#include <ace/Recursive_Thread_Mutex.h>

#include "./ProxyStub.h"
#include "./PSMap.h"

#include <set>

namespace OOCore
{
namespace Impl
{
	class RemoteObjectFactory : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;
		
		HAS_IID;
	};

	BEGIN_META_INFO(RemoteObjectFactory)
		METHOD_EX((async),SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
	END_META_INFO()
};

class OOCore_Export ObjectManager :
	public OOUtil::Object_Root<ObjectManager>,
	public Impl::RemoteObjectFactory,
	public OOObject::ProxyStubManager
{	
public:
	int RequestClose();
	void Closed();
	int ProcessMessage(OOObject::InputStream* input);
	OOObject::int32_t CreateRemoteObject(const OOObject::guid_t& oid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal);

protected:
	ObjectManager();
	virtual ~ObjectManager();

private:
	struct response_wait
	{
		response_wait(ObjectManager* t, OOObject::uint32_t k, OOObject::InputStream** i) :
			pThis(t), trans_id(k), input(i)
		{ }

		ObjectManager* pThis;
		OOObject::uint32_t trans_id;
		OOObject::InputStream** input;
	};

	enum OPCODES
	{
		RESPONSE = 0,
		REQUEST = 1,
		CONNECT = 2,
		STATIC = 3
	};

	bool m_is_opening;
	ACE_Recursive_Thread_Mutex m_lock;

	Impl::PSMap m_proxy_obj_map;
	Impl::PSMap m_stub_obj_map;
	std::map<OOObject::uint32_t,OOUtil::Object_Ptr<OOObject::Stub> > m_stub_map;
	OOObject::uint32_t m_next_stub_key;
	
	OOUtil::Object_Ptr<Impl::RemoteObjectFactory> m_ptrRemoteFactory;
	OOUtil::Object_Ptr<OOObject::Channel> m_ptrChannel;

	std::map<OOObject::uint32_t,OOUtil::InputStream_Ptr > m_response_map;
	OOObject::uint32_t m_next_trans_id;
	std::set<OOObject::uint32_t> m_transaction_set;
	
	int request_remote_factory();
	int process_request(OOUtil::InputStream_Ptr& input);
	int process_response(OOUtil::InputStream_Ptr& input);
	int process_connect(OOUtil::InputStream_Ptr& input);
	bool await_response_i(OOObject::uint32_t trans_id, OOObject::InputStream** input);
	int create_pass_thru(const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& stub_key, OOObject::Stub** stub);
	
	static bool await_response(void* p);
	static bool await_connect(void * p);
	static bool await_close(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(Impl::RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::Impl::RemoteObjectFactory
public:
	OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote);
	
// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObject::guid_t& iid, const OOObject::uint32_t& key, OOObject::Object** ppVal);
	int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOObject::uint32_t* key);
	int ReleaseProxy(const OOObject::uint32_t& key);
	int ReleaseStub(const OOObject::uint32_t& key);	
	int CreateRequest(OOObject::uint32_t method, OOObject::TypeInfo::Method_Attributes_t flags, const OOObject::uint32_t& proxy_key, OOObject::uint32_t* trans_id, OOObject::OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	OOObject::int32_t SendAndReceive(OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::OutputStream* output, OOObject::uint32_t trans_id, OOObject::InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
