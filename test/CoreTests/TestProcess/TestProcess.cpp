#include <OTL/OTL.h>

#include "../SimpleTest.h"

namespace Omega {
namespace TestSuite
{
	extern "C" const Omega::guid_t OID_TestProcess;
} }

OMEGA_DEFINE_OID(Omega::TestSuite, OID_TestProcess, "{4BC2E65B-CEE0-40c6-90F2-39C7C306FC69}" );

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

BEGIN_PROCESS_OBJECT_MAP(L"TestProcess")
	OBJECT_MAP_ENTRY(TestProcessImpl,L"Test.Process")
END_PROCESS_OBJECT_MAP()

static int install(int argc, char* argv[])
{
	try
	{
		Omega::string_t strSubsts;
		if (argc == 3)
			strSubsts = Omega::string_t(argv[2],false);
		else
			strSubsts = L"MODULE_PATH=" + Omega::string_t(argv[0],false);

		if (strcmp(argv[1],"-i") == 0 || strcmp(argv[1],"--install") == 0)
		{
	#if !defined(_WIN32)
			if (argc != 3) return -1;
	#endif

			OTL::GetModule()->RegisterObjects(true,false,strSubsts);
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[1],"-u") == 0 || strcmp(argv[1],"--uninstall") == 0)
		{
			OTL::GetModule()->RegisterObjects(false,false,strSubsts);
			return EXIT_SUCCESS;
		}
		else
		{
			// Invalid argument
			fprintf(stderr,"Invalid argument -%s\n",argv[1]);
			return EXIT_FAILURE;
		}
	}
	catch (Omega::IException* pE)
	{
		fprintf(stderr,"%ls\n",pE->GetDescription().c_str());
		pE->Release();
		return EXIT_FAILURE;
	}
}

int main(int argc, char* argv[])
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
		if (argc > 1)
		{
			ret = install(argc,argv);
		}
		else
		{
			OTL::GetModule()->Run();
		}
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		ret = EXIT_FAILURE;
	}

	Omega::Uninitialize();

	return ret;
}
