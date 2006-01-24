#ifndef OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_
#define OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_

#include "./ObjectManager.h"

namespace OOCore
{

class OOCore_Export CustomMarshaller
{
public:
	CustomMarshaller(void);
	virtual ~CustomMarshaller(void);
	
	int Open();
	int CreateRequest(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, const OOObject::uint32_t& key, OOObject::uint32_t* trans_id, OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::uint32_t* key, OOObject::Object** ppVal);
	OOObject::int32_t CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::uint32_t* key, OOObject::Object** ppVal);
	OOObject::int32_t Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input);

private:
	class CM_Channel : public Object_Impl<Channel>
	{
	public:
		CM_Channel(CustomMarshaller* owner) :
		  m_owner(owner)
		{}

	private:
		CustomMarshaller* m_owner;
		
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

	Object_Ptr<CM_Channel>	m_ptrChannel;
	Object_Ptr<CustomOM>	m_ptrOM;
};
};

#endif // OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_
