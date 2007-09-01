#include <OTL/OTL.h>

#include "../interfaces.h"
#include "./TestProcess.h"

const wchar_t TestProcessName[] = L"Test.Process";
OMEGA_DEFINE_OID(Test, OID_TestProcess, "{4BC2E65B-CEE0-40c6-90F2-39C7C306FC69}" );

class TestProcessImpl :
	public OTL::ObjectBase,
	public OTL::AutoObjectFactory<TestProcessImpl,&Test::OID_TestProcess>,
	public Test::Iface
{
public:
	TestProcessImpl()
	{ }

	Omega::string_t Hello();

	BEGIN_INTERFACE_MAP(TestProcessImpl)
		INTERFACE_ENTRY(Test::Iface)
	END_INTERFACE_MAP()
};

Omega::string_t
TestProcessImpl::Hello()
{
	return L"Hello!";
}

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestProcessImpl,TestProcessName)
END_PROCESS_OBJECT_MAP()

int main(int argc, char* argv[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		pE->Release();
		return -1;
	}

	int ret = 0;
	try
	{
		OTL::GetModule()->Run();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		ret = -1;
	}

	Omega::Uninitialize();

	return ret;
}
