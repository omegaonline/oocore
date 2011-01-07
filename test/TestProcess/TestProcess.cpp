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
#if defined(_WIN32)
	TerminateProcess(GetCurrentProcess(),EXIT_FAILURE);
#else
	_exit(EXIT_FAILURE);
#endif
}

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestProcessImpl)
END_PROCESS_OBJECT_MAP()

#include <iostream>

static void exception_details(Omega::IException* pE);

static void report_cause(Omega::IException* pE)
{
	Omega::IException* pCause = pE->GetCause();
	if (pCause)
	{
		std::cerr << "Cause: ";
		exception_details(pCause);
	}
}

void report_exception(Omega::IException* pE)
{
	std::cerr << "Exception: ";
	exception_details(pE);
}

static void exception_details(Omega::IException* pOrig)
{
	try
	{
		pOrig->Rethrow();
	}
	catch (Omega::IInternalException* pE)
	{
		std::cerr << "Omega::IInternalException - ";

		std::string s;
		pE->GetDescription().ToNative(s);

		std::cerr << s << std::endl;

		Omega::string_t strSource = pE->GetSource();
		if (!strSource.IsEmpty())
		{
			strSource.ToNative(s);
			std::cerr << "At: " << s << std::endl;
		}

		report_cause(pE);
		pE->Release();
	}
	catch (Omega::IException* pE)
	{
		std::string s;
		pE->GetDescription().ToNative(s);

		std::cerr << s << std::endl;
		report_cause(pE);
		pE->Release();
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		report_exception(pE);
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;
	try
	{
		OTL::GetModule()->Run();
	}
	catch (Omega::IException* pE)
	{
		report_exception(pE);
		ret = EXIT_FAILURE;
	}

	Omega::Uninitialize();

	return ret;
}
