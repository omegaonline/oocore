#define OMEGA_GUID_LINK_HERE

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

#include "../TestDll/testdll.h"
#include <OTL/OTL.h>

#include <stdio.h>
#include <conio.h>

int main(int argc, char* argv[])
{
	try
	{
		Omega::IException* pE = Omega::Initialize();
		if (pE)
			throw pE;

		OTL::ObjectPtr<Test::DllTest> ptrTest("Test.Dll");
		OTL::ObjectPtr<Test::DllTest> ptrTest2("Test.Dll");
		
		printf(ptrTest->Hello());

		Omega::Uninitialize();
	}
	catch(Omega::IException* pE)
	{
		printf("Exception: %s\nAt: %s.\n\nPress any key to continue...\n",(const char*)pE->Description(),(const char*)pE->Source());
		_getch();

		pE->Release();
		return -1;
	}

	printf("Everything is cool - Press any key to continue...\n");
	_getch();

	return 0;
}
