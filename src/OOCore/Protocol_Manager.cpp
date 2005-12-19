#include "./Protocol_Manager.h"

OOObject::int32_t 
OOCore::Impl::Protocol_Manager::AddProtocol(const OOObject::char_t* name, OOCore::Protocol* protocol)
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
OOCore::Impl::Protocol_Manager::RemoveProtocol(const OOObject::char_t* name)
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
OOCore::Impl::Protocol_Manager::create_remote_object(const OOObject::char_t* remote_addr, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	// URL format = <protocol>://<protocol_specific_address>
	ACE_CString strURL(remote_addr);

	// Find the first colon
	ssize_t colon = strURL.find("://");
	if (colon==-1)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Bad remote transport address %s\n"),ACE_TEXT_CHAR_TO_TCHAR(remote_addr)),-1);
	}

	ACE_CString strAddress = strURL.substr(colon+3);

	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_CString,OOCore::Object_Ptr<OOCore::Protocol> >::iterator i=m_protocol_map.find(strURL.substr(0,colon));
	if (i==m_protocol_map.end())
	{
		errno = EPFNOSUPPORT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No such protocol\n")),-1);
	}
	
	return -1;	
}
