// ControlTest.cpp : Defines the entry point for the console application.
//

#include <tchar.h>

#include "../../src/OOCore/OOClient.h"
#include "../../src/OOCore/Client_Service.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = OOClient_Init();
	if (ret==0)
	{
		{
			OOObj::Object_Ptr<OOObj::Client_Service> pObj;
			if (OOClient_CreateObject("control_service",&pObj) == 0)
			{
				OOObj::uint16_t arr[12];
				pObj->Array_Test_In(12,arr);

				OOObj::uint32_t count = 12;
				OOObj::uint16_t* parr = arr;
				pObj->Array_Test_InOut(&count,&parr);

				// Crash test dummy!
				//exit(-1);

				OOCore_Free(parr);

				pObj->Array_Test_Out(&count,&parr);
				OOCore_Free(parr);

				OOObj::uint16_t remaining = 0;
				pObj->Stop(false,&remaining);
			}
		}
				
		OOClient_Term();
	}

	return ret;
}