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
	int CreateRequest(OOObject::uint32_t method, OOObject::TypeInfo::Method_Attributes_t flags, const OOObject::uint32_t& key, OOObject::uint32_t* trans_id, OOObject::OutputStream** output);
	int CancelRequest(OOObject::uint32_t trans_id);
	OOObject::int32_t CreateObject(const OOObject::guid_t& oid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::uint32_t* key, OOObject::Object** ppVal);
	OOObject::int32_t Invoke(OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::OutputStream* output, OOObject::uint32_t trans_id, OOObject::InputStream** input);

private:
	class CM_Channel : public OOUtil::Object_Impl<OOObject::Channel>
	{
	public:
		CM_Channel(CustomMarshaller* owner) :
		  m_owner(owner)
		{}

	private:
		CustomMarshaller* m_owner;
		
	// OOCore::Channel
	public:
		int CreateOutputStream(OOObject::OutputStream** ppStream);
		int Send(OOObject::OutputStream* output);
	};
	friend class CM_Channel;

	class CustomOM : public ObjectManager
	{
	public:
	};

	OOUtil::Object_Ptr<CM_Channel>	m_ptrChannel;
	OOUtil::Object_Ptr<CustomOM>	m_ptrOM;
};
};

#endif // OOCORE_CUSTOM_MARSHALLER_H_INCLUDED_
