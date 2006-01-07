#ifndef OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
#define OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_

#include "./OOCore_Impl.h"
#include "./ProxyStub.h"
#include "./PSMap.h"

#include <map>
#include <set>

namespace OOCore
{
namespace Impl
{
	class RemoteObjectFactory : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;
		virtual OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;
		virtual OOObject::int32_t AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory) = 0;
		virtual OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid) = 0;
	
		DECLARE_IID(OOCore);
	};

	BEGIN_META_INFO(RemoteObjectFactory)
		METHOD(CreateRemoteObject,4,((in)(string),const OOObject::char_t*,remote_url,(in),const OOObject::guid_t&,clsid,(in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,ppVal))
		METHOD(SetReverse,1,((in)(iid_is(RemoteObjectFactory::IID)),RemoteObjectFactory*,pRemote)) 
		METHOD(AddObjectFactory,3,((in),ObjectFactory::Flags_t,flags,(in),const OOObject::guid_t&,clsid,(in)(iid_is(OOCore::ObjectFactory::IID)),OOCore::ObjectFactory*,pFactory))
		METHOD(RemoveObjectFactory,1,((in),const OOObject::guid_t&,clsid))
	END_META_INFO()
};

class OOCore_Export ObjectManager :
	public Object_Root,
	public Impl::RemoteObjectFactory,
	public ProxyStubManager
{
	friend class shutup_gcc_warnings;
	
public:
	ObjectManager();
	
	int Open(Channel* channel, const bool AsAcceptor);
	int Close(bool channel_alive);
	int ProcessMessage(InputStream* input);
	OOObject::int32_t RequestRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

private:
	virtual ~ObjectManager();

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
	enum
	{
		NOT_OPENED,
		OPEN,
		CLOSED
	} m_bOpened;
	ACE_Recursive_Thread_Mutex m_lock;

	Impl::PSMap m_proxy_obj_map;
	Impl::PSMap m_stub_obj_map;
	ACE_Active_Map_Manager<Stub*> m_stub_map;
	
	Object_Ptr<Impl::RemoteObjectFactory> m_ptrRemoteFactory;
	Object_Ptr<Channel> m_ptrChannel;

	std::map<OOObject::uint32_t,Object_Ptr<InputStream> > m_response_map;
	OOObject::uint32_t m_next_trans_id;
	std::set<OOObject::uint32_t> m_transaction_set;
	
	int connect();
	int accept();
	int process_request(Impl::InputStream_Wrapper& input);
	int process_response(Impl::InputStream_Wrapper& input);
	int process_connect(Impl::InputStream_Wrapper& input);
	bool await_response_i(OOObject::uint32_t trans_id, InputStream** input);
	int create_pass_thru(OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& stub_key, Stub*& stub);

	static bool await_response(void* p);
	static bool await_connect(void * p);
	static bool await_close(void * p);

BEGIN_INTERFACE_MAP(ObjectManager)
	INTERFACE_ENTRY(Impl::RemoteObjectFactory)
	INTERFACE_ENTRY(ProxyStubManager)
END_INTERFACE_MAP()

// OOCore::Impl::RemoteObjectFactory
public:
	OOObject::int32_t CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
	OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote);
	OOObject::int32_t AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory);
	OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
	
// OOCore::ProxyStubManager
public:
	int CreateProxy(const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** ppVal);
	int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOCore::ProxyStubManager::cookie_t* key);
	int ReleaseProxy(const OOCore::ProxyStubManager::cookie_t& key);
	int ReleaseStub(const OOCore::ProxyStubManager::cookie_t& key);	
	int CreateRequest(Stub::Flags_t flags, const OOCore::ProxyStubManager::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	int SendAndReceive(Stub::Flags_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);
};

};

#endif // OOCORE_OBJECTMANAGER_IMPL_H_INCLUDED_
