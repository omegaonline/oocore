#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

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
		virtual OOObject::int32_t RequestRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;
		virtual OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;
		virtual OOObject::int32_t AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory) = 0;
		virtual OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid) = 0;
	
		DECLARE_IID(OOCore);
	};

	BEGIN_META_INFO(RemoteObjectFactory)
		METHOD(RequestRemoteObject,4,((in)(string),const OOObject::char_t*,remote_url,(in),const OOObject::guid_t&,clsid,(in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,ppVal))
		METHOD_EX((async),SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
		METHOD(AddObjectFactory,3,((in),ObjectFactory::Flags_t,flags,(in),const OOObject::guid_t&,clsid,(in)(iid_is(OOCore::ObjectFactory::IID)),OOCore::ObjectFactory*,pFactory))
		METHOD(RemoveObjectFactory,1,((in),const OOObject::guid_t&,clsid))
	END_META_INFO()
};

class OOCore_Export ObjectManager :
	public Object_Root,
	public Impl::RemoteObjectFactory,
	public ProxyStubManager
{	
public:
	ObjectManager();
	
	int Open(Channel* channel);
	int RequestClose();
	void Closed();
	int ProcessMessage(InputStream* input);
	OOObject::int32_t CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

protected:
	virtual ~ObjectManager();

private:
	struct response_wait
	{
		response_wait(ObjectManager* t, OOObject::uint32_t k, InputStream** i) :
			pThis(t), trans_id(k), input(i)
		{ }

		Object_Ptr<ObjectManager> pThis;
		OOObject::uint32_t trans_id;
		InputStream** input;
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
	std::map<OOObject::uint32_t,Stub*> m_stub_map;
	OOObject::uint32_t m_next_stub_key;
	
	Object_Ptr<Impl::RemoteObjectFactory> m_ptrRemoteFactory;
	Object_Ptr<Channel> m_ptrChannel;

	std::map<OOObject::uint32_t,Object_Ptr<InputStream> > m_response_map;
	OOObject::uint32_t m_next_trans_id;
	std::set<OOObject::uint32_t> m_transaction_set;
	
	int request_remote_factory();
	int process_request(InputStream_Wrapper& input);
	int process_response(InputStream_Wrapper& input);
	int process_connect(InputStream_Wrapper& input);
	bool await_response_i(OOObject::uint32_t trans_id, InputStream** input);
	int create_pass_thru(const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& stub_key, Stub** stub);
	
	static bool await_response(void* p);
	static bool await_connect(void * p);
	static bool await_close(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(Impl::RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::Impl::RemoteObjectFactory
public:
	OOObject::int32_t RequestRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
	OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote);
	OOObject::int32_t AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory);
	OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
	
// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObject::guid_t& iid, const OOObject::uint32_t& key, OOObject::Object** ppVal);
	int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOObject::uint32_t* key);
	int ReleaseProxy(const OOObject::uint32_t& key);
	int ReleaseStub(const OOObject::uint32_t& key);	
	int CreateRequest(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, const OOObject::uint32_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	OOObject::int32_t SendAndReceive(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
