#include "./PassThruStub.h"
#include "./OutputStream_CDR.h"

OOCore::Impl::PassThruStub::PassThruStub(OOCore::ObjectManager* stub_manager, const OOObject::uint32_t& stub_key, Object_Ptr<OOCore::ProxyStubManager>& proxy_manager, const OOObject::uint32_t& proxy_key,Object_Ptr<OOCore::Proxy>& proxy) :
	m_stub_manager(stub_manager),
	m_stub_key(stub_key),
	m_proxy_manager(proxy_manager),
	m_proxy_key(proxy_key),
	m_proxy(proxy)
{
}

int 
OOCore::Impl::PassThruStub::init(const OOObject::guid_t& iid, Stub* stub)
{
	if (stub == 0)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}
	
	// Get the TypeInfo
	Object_Ptr<TypeInfo> ptrTypeInfo;	
	if (OOCore::GetTypeInfo(iid,&ptrTypeInfo) != 0)
		return -1;
		
	// Enum the methods looking for any with iid_is attributes
	const OOObject::char_t* name;
	size_t method_count;
	if (ptrTypeInfo->GetMetaInfo(&name,&method_count) != 0)
		return -1;
		
	for (size_t method=2;method<method_count;++method)
	{
		TypeInfo::Method_Attributes_t attributes;
		OOObject::uint16_t wait_secs;
		size_t param_count;
		if (ptrTypeInfo->GetMethodInfo(method,&name,&param_count,&attributes,&wait_secs) != 0)
			return -1;
			
		for (size_t param=0;param<param_count;++param)
		{
			TypeInfo::Type_t type;
			if (ptrTypeInfo->GetParamInfo(method,param,&name,&type) != 0)
				return -1;
				
			// Check to see if we have a parameter of type Object
			if ((type & TypeInfo::TYPE_MASK) == TypeInfo::Object)
			{
				m_iid_methods.insert(method);
				break;	
			}
		}
	}
	
	// Stash the stub for later
	m_stub = stub;
	
	return 0;
}		

int 
OOCore::Impl::PassThruStub::Invoke(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output)
{
	// Check for Release first
	if (method==1)
	{
		m_proxy = 0;
		m_stub = 0;
		return 0;
	}
	else if (m_iid_methods.find(method) != m_iid_methods.end())
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) pass thru routing method %u to stub.\n"),method));
		
		// If it is a method with a contained object, just call the stub
		return m_stub->Invoke(method,flags,wait_secs,input,output);
	}
	
	// Create a request output stream
	OOObject::uint32_t trans_id;
	OOCore::Object_Ptr<OOCore::OutputStream> request;
	if (m_proxy_manager->CreateRequest(method,flags,m_proxy_key,&trans_id,&request) != 0)
		return -1;
	
	// Copy input to request
	if (copy(input,request) != 0)
	{
		m_proxy_manager->CancelRequest(trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy request\n")),-1);
	}

	// Send the request
	OOCore::Object_Ptr<OOCore::InputStream> response;
	OOObject::int32_t ret = m_proxy_manager->SendAndReceive(flags,wait_secs,request,trans_id,&response);
	
	if (!(flags & TypeInfo::async_method))
	{
		// Write error code out
		if (output->WriteLong(ret) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write ret_code\n")),-1);

		if (ret != 0)
		{
			if (output->WriteLong(errno) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1);
		}
		else
		{
			// Copy response to output
			if (copy(response,output) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy response\n")),-1);
		}
	}
	
	return ret;
}

int 
OOCore::Impl::PassThruStub::GetObject(OOObject::Object** ppVal)
{
	errno = EFAULT;
	ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("GetObject is not allowed on a PassThruStub\n")),-1);
}

int 
OOCore::Impl::PassThruStub::copy(OOCore::InputStream* in, OOCore::OutputStream* out)
{
#ifdef DONT_USE_DYNAMIC_CAST
	// Check the magic numbers and copy
	Object_Ptr<Impl::OutputStream_CDR> out_cdr;
	if (out->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&out_cdr)) != 0)
	{
		errno = EFAULT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream\n")),-1);
	}

	Object_Ptr<Impl::InputStream_CDR> in_cdr;
	if (in->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&in_cdr)) != 0)
	{
		errno = EFAULT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid input stream\n")),-1);
	}
#else
	Impl::InputStream_CDR* in_cdr = dynamic_cast<Impl::InputStream_CDR*>(in);
	Impl::OutputStream_CDR* out_cdr = dynamic_cast<Impl::OutputStream_CDR*>(out);
#endif

	return out_cdr->copy_from(in_cdr);
}
