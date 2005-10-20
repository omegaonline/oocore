#include "./OOCore.h"

#include <ace/CDR_Stream.h>
#include <ace/Active_Map_Manager.h>

ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_Active_Map_Manager_Key& key)
{
	// Read the channel key
	key.decode(input.rd_ptr());
	if (!input.skip_bytes(ACE_Active_Map_Manager_Key::size()))
		return false;

	return true;
}

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_Active_Map_Manager_Key& key)
{
	ACE_CDR::Octet* buf;
	ACE_NEW_RETURN(buf,ACE_CDR::Octet[ACE_Active_Map_Manager_Key::size()],false);
	
	key.encode(buf);

	bool ret = output.write_octet_array(buf,ACE_Active_Map_Manager_Key::size());
	delete [] buf;
	if (!ret)
		return false;
		
	return true;	
}

ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_CDR::Boolean& val)
{
	return input.read_boolean(val);
}

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_CDR::Boolean& val)
{
	return output.write_boolean(val);
}

#include "./Transport_Service.h"
#include "./Client_Service.h"

extern "C" int OOCore_Export CreateStub(const OOObj::GUID& iid, OOObj::Object* obj, OOCore_Object_Stub_Base** stub)
{
	if (iid==OOCore_Transport_Service::IID)
		ACE_NEW_RETURN(*stub,OOCore_Transport_Service_Stub(obj),-1);
	else if (iid==OOObj::Client_Service::IID)
		ACE_NEW_RETURN(*stub,OOObj::Client_Service_Stub(obj),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Stub IID\n")),-1);
	
	return 0;
}

extern "C" int OOCore_Export CreateProxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler, OOObj::Object** proxy)
{
	if (iid==OOCore_Transport_Service::IID)
		ACE_NEW_RETURN(*proxy,OOCore_Transport_Service_Proxy(key,handler),-1);
	else if (iid==OOObj::Client_Service::IID)
		ACE_NEW_RETURN(*proxy,OOObj::Client_Service_Proxy(key,handler),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Proxy IID\n")),-1);
	
	(*proxy)->AddRef();
	return 0;
}

OOCore_Export void* OOCore_Alloc(size_t size)
{
	return ACE_OS::malloc(size);
}

OOCore_Export void OOCore_Free(void* p)
{
	ACE_OS::free(p);
}
