// OOps.cpp : Defines the entry point for the console application.
//

#include <tchar.h>

#include "../../../src/OOCore/OOClient.h"
#include "../OOps_Svc/Local_Service.h"

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<=1)
		return -1;

	int ret = OOClient_Init();
	if (ret==0)
	{
		{
			OOObj::Object_Ptr<OOps_Local_Service> pObj;
			if (OOClient_CreateObject("OOps",&pObj) == 0)
			{
				pObj->Ping(argv[1]);
			}
		}
				
		OOClient_Term();
	}

	return ret;
}