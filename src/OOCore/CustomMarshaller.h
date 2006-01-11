#ifndef OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_
#define OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Message_Queue.h>

#include "./ObjectManager.h"

namespace OOCore
{

class OOCore_Export CustomMarshaller
{
public:
	CustomMarshaller(void);
	virtual ~CustomMarshaller(void);
	
	int Open();
	int CreateRequest(TypeInfo::Method_Attributes_t flags, const OOCore::ProxyStubManager::cookie_t& key, OOObject::uint32_t method, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOCore::ProxyStubManager::cookie_t* key, OOObject::Object** ppVal);
	OOObject::int32_t CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOCore::ProxyStubManager::cookie_t* key, OOObject::Object** ppVal);
	OOObject::int32_t Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);

private:
	class CM_Channel : public Object_Impl<Channel>
	{
	public:
		CM_Channel(CustomMarshaller* owner, const bool inner) :
		  m_owner(owner), m_is_inner(inner)
		{}

	private:
		CustomMarshaller* m_owner;
		const bool m_is_inner;

	// OOCore::Channel
	public:
		int CreateOutputStream(OutputStream** ppStream);
		int Send(OutputStream* output);
	};
	friend class CM_Channel;

	class CustomOM : public ObjectManager
	{
	public:
	};

	Object_Ptr<CM_Channel> m_inner_channel;
	Object_Ptr<CM_Channel> m_outer_channel;
	Object_Ptr<ObjectManager> m_inner_OM;
	Object_Ptr<CustomOM> m_outer_OM;

	ACE_Message_Queue<ACE_MT_SYNCH> m_msg_queue;

	int recv_from_inner(ACE_Message_Block* mb);
	int recv_from_outer(ACE_Message_Block* mb);
};
};

#endif // OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_
