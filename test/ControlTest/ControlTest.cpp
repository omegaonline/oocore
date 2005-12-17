// ControlTest.cpp : Defines the entry point for the console application.
//

#include <tchar.h>

#include <OOCore/OOCore_Util.h>
#include "../Test/Test.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = OOObject::Init();
	if (ret==0)
	{
		{
			OOCore::Object_Ptr<Test::Test> pTest;
			if (OOObject::CreateObject(CLSID_Test,&pTest) == 0)
			{
				OOObject::uint16_t arr[12];
				pTest->Array_Test_In(12,arr);

				OOObject::uint32_t count = 12;
				OOObject::uint16_t* parr = arr;
				if (pTest->Array_Test_InOut(&count,&parr) == 0)
					if (parr!=arr) OOObject::Free(parr);

				// Crash test dummy!
				//exit(-1);

				if (pTest->Array_Test_Out(&count,&parr) == 0)
					if (parr!=arr) OOObject::Free(parr);

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