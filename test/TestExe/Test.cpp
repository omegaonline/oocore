#define OMEGA_GUID_LINK_HERE

#include <vld.h>

#include "../TestDll/testdll.h"
#include <OTL/OTL.h>

#include <stdio.h>

int main(int argc, char* argv[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		printf("Init failed %s.\n",(const char*)pE->Description());
		pE->Release();
		return -1;
	}

	printf("Everything is cool - we wait for 10 secs so you can read this!\n");

	::Sleep(10000);

	/*try
	{
		OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
		OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");
		
		printf(ptrTest->Hello());
	}
	catch(Omega::IException* pE)
	{
		printf(pE->Description());
		pE->Release();
	}*/

	Omega::Uninitialize();

	return 0;
}
