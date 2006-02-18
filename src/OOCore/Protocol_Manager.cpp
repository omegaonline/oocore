#include "./Protocol_Manager.h"

OOObject::int32_t 
OOCore::Impl::Protocol_Manager::RegisterProtocol(const OOObject::char_t* name, OOCore::Protocol* protocol)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_CString,OOCore::Object_Ptr<OOCore::Protocol> >::iterator i=m_protocol_map.find(name);
	if (i!=m_protocol_map.end())
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Duplicate protocol\n")),-1);
	}

	m_protocol_map.insert(std::map<ACE_CString,OOCore::Object_Ptr<OOCore::Protocol> >::value_type(name,protocol));

	return 0;
}

OOObject::int32_t 
OOCore::Impl::Protocol_Manager::UnregisterProtocol(const OOObject::char_t* name)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_CString,OOCore::Object_Ptr<OOCore::Protocol> >::iterator i=m_protocol_map.find(name);
	if (i==m_protocol_map.end())
	{
		errno = ENOENT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No such protocol\n")),-1);
	}

	m_protocol_map.erase(i);

	return 0;
}

OOObject::int32_t 
OOCore::Impl::Protocol_Manager::create_remote_object(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	// URL format = <protocol>://<protocol_specific_address>
	ACE_CString strURL(remote_url);

	// Find the first colon
	ssize_t colon = strURL.find("://");
	if (colon==-1)
	{
		errno = ENOENT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Bad remote transport address %s\n"),ACE_TEXT_CHAR_TO_TCHAR(remote_url)),-1);
	}

	// Lookup the corresponding protocol
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_CString,OOCore::Object_Ptr<OOCore::Protocol> >::iterator i=m_protocol_map.find(strURL.substr(0,colon));
	if (i==m_protocol_map.end())
	{
		errno = EPFNOSUPPORT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No such protocol\n")),-1);
	}
	Object_Ptr<Protocol> protocol = i->second;

	guard.release();

	// Connect to the transport
	Object_Ptr<Transport> transport;
	if (protocol->Connect(strURL.substr(colon+3).c_str(),&transport) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Connect failed\n")),-1);

	// Ask the transport to create the object
    return transport->CreateObject(clsid,pOuter,iid,ppVal);	
}
