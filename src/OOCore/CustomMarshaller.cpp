#include "./CustomMarshaller.h"
#include "./OutputStream_CDR.h"

OOCore::CustomMarshaller::CustomMarshaller(void)
{
}

OOCore::CustomMarshaller::~CustomMarshaller(void)
{
}

int 
OOCore::CustomMarshaller::Open()
{
	// Create the ObjectManager
	ACE_NEW_RETURN(m_ptrChannel,CM_Channel(this),-1);
	ACE_NEW_RETURN(m_ptrOM,CustomOM,-1);

	if (m_ptrOM->Open(m_ptrChannel) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("Failed to open outer object manager.\n")),-1);

	return 0;
}

OOObject::int32_t 
OOCore::CustomMarshaller::CreateObject(const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::uint32_t* key, OOObject::Object** ppVal)
{
	return CreateRemoteObject(0,clsid,pOuter,iid,key,ppVal);
}

OOObject::int32_t 
OOCore::CustomMarshaller::CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::uint32_t* key, OOObject::Object** ppVal)
{
	Object_Ptr<OOObject::Object> ptrObj;
	if (OOObject::CreateRemoteObject(remote_url,clsid,pOuter,iid,&ptrObj) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("Outer object manager CreateRemoteObject failed.\n")),-1);

	if (m_ptrOM->CreateStub(iid,ptrObj,key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("Outer object manager CreateStub failed.\n")),-1);

	if (ppVal)
	{
		*ppVal = ptrObj;
		(*ppVal)->AddRef();
	}

	return 0;
}

int 
OOCore::CustomMarshaller::CreateRequest(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, const OOObject::uint32_t& key, OOObject::uint32_t* trans_id, OutputStream** output)
{
	if (m_ptrOM->CreateRequest(method,flags,key,trans_id,output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("Outer object manager CreateRequest failed.\n")),-1);

	return 0;
}

int 
OOCore::CustomMarshaller::CancelRequest(OOObject::uint32_t trans_id)
{
	return m_ptrOM->CancelRequest(trans_id);
}

OOObject::int32_t 
OOCore::CustomMarshaller::Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input)
{
	return m_ptrOM->SendAndReceive(flags,wait_secs,output,trans_id,input);
}

int 
OOCore::CustomMarshaller::CM_Channel::CreateOutputStream(OutputStream** ppStream)
{
	if (!ppStream)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	Impl::OutputStream_CDR* pStream;
	ACE_NEW_RETURN(pStream,Impl::OutputStream_CDR(),-1);

	*ppStream = pStream;
	(*ppStream)->AddRef();

	return 0;
}

int 
OOCore::CustomMarshaller::CM_Channel::Send(OutputStream* output)
{
	// See if output is a Impl::OutputStream_CDR
	Object_Ptr<Impl::OutputStream_CDR> pStream;
	if (output->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&pStream)) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream passed in to transport\n")),-1);
	
	// Align the output buffer end position, because it may get concatenated with another
	pStream->align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Copy the block
	ACE_Message_Block* mb = pStream->begin()->duplicate();
	Object_Ptr<Impl::InputStream_CDR> i;
	ACE_NEW_NORETURN(i,Impl::InputStream_CDR(ACE_InputCDR(mb)));
	mb->release();
	if (!i)
		return -1;
	
	return m_owner->m_ptrOM->ProcessMessage(i);
}
