// ControlTest.cpp : Defines the entry point for the console application.
//

//#include <tchar.h>

#include <OOCore/OOCore_Util.h>

#include "../Test/Test.h"

void DoTests(OOCore::Object_Ptr<Test::Test>& ptrTest)
{
	OOObject::uint16_t arr[11] = {0};
	OOObject::uint32_t count = 7;
		
	ACE_OS::printf("Calling Array_Test_In with %u items: ",count);
	if (ptrTest->Array_Test_In(count,arr)==0)
		ACE_OS::printf("succeeded.");
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");

	OOObject::uint16_t* parr = 0;
	ACE_OS::printf("Calling Array_Test_Out: ");
	if (ptrTest->Array_Test_Out(&count,&parr) == 0)
		ACE_OS::printf("succeeded, %u items received.",count);
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");
	
	ACE_OS::printf("Calling Array_Test_InOut with %u items: ",count);
	if (ptrTest->Array_Test_InOut(&count,&parr) == 0)
		ACE_OS::printf("succeeded, %u items received.",count);
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");

	OOObject::Free(parr);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int ret = OOObject::Init();
	if (ret==0)
	{
		{
            OOCore::Object_Ptr<Test::Test> ptrTest;
			if (OOObject::CreateObject(CLSID_Test,&ptrTest) == 0)
				DoTests(ptrTest);
			else
				ACE_OS::perror("CreateObject failed");
						
			ptrTest = 0;
			if (OOObject::CreateRemoteObject("tcp://localhost:5000",CLSID_Test,&ptrTest) == 0)
				DoTests(ptrTest);
			else
				ACE_OS::perror("CreateRemoteObject failed");
		}
				
		OOObject::Term();
	}

	return ret;
}