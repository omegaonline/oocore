#ifndef _OONET_KADEMLIA_SERVER_H_INCLUDED_
#define _OONET_KADEMLIA_SERVER_H_INCLUDED_

#include <ace/Service_Object.h>

#include "./Kademlia.h"

class Kademlia_Service : 
	public ACE_Service_Object,
	public OOCore::Object_Root,
	public OOCore::ObjectFactory,
	public Kademlia::RPC_Request,
	public Kademlia::RPC_Response
{
public:
	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

BEGIN_INTERFACE_MAP(Kademlia_Service)
	INTERFACE_ENTRY(OOCore::ObjectFactory)
	INTERFACE_ENTRY(Kademlia::RPC_Request)
	INTERFACE_ENTRY(Kademlia::RPC_Response)
END_INTERFACE_MAP()

// ObjectFactory members
public:
	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);

// RPC_Request members
public:

// RPC_Response members
public:

};

#endif // _OONET_KADEMLIA_SERVER_H_INCLUDED_
