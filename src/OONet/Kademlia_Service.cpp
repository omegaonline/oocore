#include "./Kademlia_Service.h"

#include <ace/Service_Config.h>
#include <ace/Get_Opt.h>

#include "./OONet_export.h"

#include "../OOCore/Guid.h"

ACE_FACTORY_DEFINE(OONet,Kademlia_Service)

DEFINE_IID(Kademlia::RPC_Request,6AAE8C33-699A-4414-AF84-25E74E693207);
DEFINE_IID(Kademlia::RPC_Response,6AAE8C33-699A-4414-AF84-25E74E693207);

int Kademlia_Service::init(int argc, ACE_TCHAR *argv[])
{
	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":p:"),0);
	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		//case ACE_TEXT('p'):
		//	uPort = ACE_OS::atoi(cmd_opts.opt_arg());
		//	break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	// Artifically increment our RefCount, the ACE_Svc_Config will delete us
    AddRef();

	if (OOCore::RegisterStaticInterface(Kademlia::RPC_Request::IID,this) != 0 ||
		OOCore::RegisterStaticInterface(Kademlia::RPC_Response::IID,this) != 0)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to register object factory\n")),-1);
	}
	
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Kademlia service started.\n")));
			
	return 0;
}

int 
Kademlia_Service::fini(void)
{
	OOCore::UnregisterStaticInterface(Kademlia::RPC_Request::IID);
	OOCore::UnregisterStaticInterface(Kademlia::RPC_Response::IID);

	return 0;
}

OOObject::int32_t 
Kademlia_Service::CreateObject(const OOObject::guid_t& clsid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	return -1;
}
