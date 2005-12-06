// ControlTest.cpp : Defines the entry point for the console application.
//

#include <tchar.h>

#include "../../src/OOCore/OOCore_Util.h"
#include "../../src/OOCore/Test.h"
//#include "../../src/OOCore/Client_Service.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = OOObject::Init();
	if (ret==0)
	{
		{
			OOCore::Object_Ptr<OOCore::Test> pTest;
			if (OOObject::CreateObject(OOCore::CLSID_Test,&pTest) == 0)
			{
				OOObject::uint16_t arr[12];
				pTest->Array_Test_In(12,arr);

				OOObject::uint32_t count = 12;
				OOObject::uint16_t* parr = arr;
				pTest->Array_Test_InOut(&count,&parr);

				// Crash test dummy!
				//exit(-1);

				OOObject::Free(parr);

				pTest->Array_Test_Out(&count,&parr);
				OOObject::Free(parr);

				//OOObject::uint16_t remaining = 0;
				//pObj->Stop(false,&remaining);
			}
			else
			{
				ACE_ERROR((LM_DEBUG,ACE_TEXT("Failed to create object: %m.\n")));
			}
		}
				
		OOObject::Term();
	}

	return ret;
}