#include "../../include/Omega/Omega.h"
#include "../../include/OTL/OTL.h"

#include "../SimpleTest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace Omega
{
	namespace TestSuite
	{
		extern "C" const Omega::guid_t OID_TestProcess;
	}
}

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestProcess, "{4BC2E65B-CEE0-40c6-90F2-39C7C306FC69}");

class TestProcessImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestProcessImpl,&Omega::TestSuite::OID_TestProcess>,
		public OTL::IProvideObjectInfoImpl<TestProcessImpl>,
		public SimpleTestImpl
{
public:
	TestProcessImpl()
	{ }

	void Abort();

	BEGIN_INTERFACE_MAP(TestProcessImpl)
		INTERFACE_ENTRY(Omega::TestSuite::ISimpleTest)
		INTERFACE_ENTRY(Omega::TestSuite::ISimpleTest2)
		INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
	END_INTERFACE_MAP()
};

void TestProcessImpl::Abort()
{
	exit(EXIT_FAILURE);
}

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestProcessImpl)
END_PROCESS_OBJECT_MAP()

int main(int /*argc*/, char* /*argv*/[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		pE->Release();
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;
	try
	{
		OTL::GetModule()->Run();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		ret = EXIT_FAILURE;
	}

	Omega::Uninitialize();

	return ret;
}
