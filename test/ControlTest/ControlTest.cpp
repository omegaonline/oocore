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
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");

	OOObject::int16_t* parr = 0;
	ACE_OS::printf("Calling Array_Test_Out: ");
	if (ptrTest->Array_Test_Out(&count,&parr) == 0)
		ACE_OS::printf("succeeded, %u items received.",count);
	else
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");
	
	ACE_OS::printf("Calling Array_Test_InOut with %u items: ",count);
	OOObject::int16_t* parr2 = parr;
	if (ptrTest->Array_Test_InOut(&count,&parr2) == 0)
		ACE_OS::printf("succeeded, %u items received.",count);
	else
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");

	OOObject::Free(parr);
	if (parr2 != parr)
		OOObject::Free(parr2);

	ACE_OS::printf("Calling String_Test_In ");
	if (ptrTest->String_Test_In("Hello, can you hear me?")==0)
		ACE_OS::printf("succeeded.");
	else
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");

	OOObject::char_t* pstr = 0;
	ACE_OS::printf("Calling String_Test_Out: ");
	if (ptrTest->String_Test_Out(&pstr) == 0)
		ACE_OS::printf("succeeded, received \"%s\".",pstr);
	else
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");

	ACE_OS::printf("Calling String_Test_InOut ",count);
	OOObject::char_t* pstr2 = pstr;
	if (ptrTest->String_Test_InOut(&pstr2) == 0)
		ACE_OS::printf("succeeded, received \"%s\".",pstr2);
	else
		ACE_OS::perror("failed.");
	ACE_OS::printf("\n");

	OOObject::Free(pstr);
	if (pstr2 != pstr)
		OOObject::Free(pstr2);
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

			char szBuf[512] = "tcp://localhost:5000";
			if (argc == 2)
				strcpy(szBuf,argv[1]);
							
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
