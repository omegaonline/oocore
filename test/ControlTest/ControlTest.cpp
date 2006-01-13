// ControlTest.cpp : Defines the entry point for the console application.
//

//#include <tchar.h>

#include <OOCore/OOCore_Util.h>

#include "../Test/Test.h"

void DoTests(OOCore::Object_Ptr<Test::Test>& ptrTest)
{
	OOObject::int16_t arr[11] = {0};
	OOObject::uint32_t count = 7;
		
	ACE_OS::printf("Calling Array_Test_In with %u items: ",count);
	if (ptrTest->Array_Test_In(count,arr)==0)
		ACE_OS::printf("succeeded.");
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");

	OOObject::int16_t* parr = 0;
	ACE_OS::printf("Calling Array_Test_Out: ");
	if (ptrTest->Array_Test_Out(&count,&parr) == 0)
		ACE_OS::printf("succeeded, %u items received.",count);
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");
	
	ACE_OS::printf("Calling Array_Test_InOut with %u items: ",count);
	if (ptrTest->Array_Test_InOut(&count,&parr) == 0)
	{
		ACE_OS::printf("succeeded, %u items received.",count);
		OOObject::Free(parr);
	}
	else
		ACE_OS::perror("failed");
	ACE_OS::printf("\n");
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int ret = OOObject::Init();
	if (ret==0)
	{
		// The inner braces ensure that all objects are destructed before OOObject::Term() is called
		{
            OOCore::Object_Ptr<Test::Test> ptrTest;
			if ((ret=OOObject::CreateObject(CLSID_Test,&ptrTest)) == 0)
				DoTests(ptrTest);
			else
			{
				ACE_OS::printf("CreateObject failed: errno %d, ",errno);
				ACE_OS::perror("");
			}

			char szBuf[512] = "tcp://";
			if (argc == 2)
				strcat(szBuf,argv[1]);
			else
				strcat(szBuf,"localhost:5000");
				
			ptrTest = 0;
			if ((ret=OOObject::CreateRemoteObject(szBuf,CLSID_Test,&ptrTest)) == 0)
				DoTests(ptrTest);
			else
			{
				ACE_OS::printf("CreateRemoteObject failed: errno %d, ",errno);
				ACE_OS::perror("");
			}

			if (ret != 0)
				ACE_OS::sleep(5);
		}
				
		OOObject::Term();
	}

	return ret;
}
