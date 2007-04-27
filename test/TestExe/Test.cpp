#define OMEGA_GUID_LINK_HERE

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

#include "../TestDll/testdll.h"
#include <OTL/OTL.h>

#include <stdio.h>
#include <conio.h>

static void printf_exception(Omega::IException* pE)
{
	printf("Exception: %s\nAt: %s.\n\nPress any key to continue...\n",(const char*)pE->Description(),(const char*)pE->Source());
	_getch();
	pE->Release();
}

int main(int argc, char* argv[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		printf_exception(pE);
		return -1;
	}
	
	try
	{
		OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
		OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");
	
		printf(ptrTest->Hello());
	}
	catch(Omega::IException* pE)
	{
		printf_exception(pE);
		Omega::Uninitialize();
		return -1;
	}

	printf("Everything is cool - Press any key to continue...\n");
	_getch();

	Omega::Uninitialize();
	return 0;
}
