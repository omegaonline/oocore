#include "../../include/Omega/Service.h"
#include "../../include/OTL/OTL.h"

namespace Omega
{
	namespace TestSuite
	{
		extern const Omega::guid_t OID_TestService;
	}
}

const Omega::guid_t Omega::TestSuite::OID_TestService("{32A13162-BC9C-2CC1-531A-F0A8BF153E0D}");

class TestServiceImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<TestServiceImpl,&Omega::TestSuite::OID_TestService>,
		public Omega::System::IService
{
public:
	TestServiceImpl()
	{
	}

	void Start(const Omega::string_t& strName, Omega::Registry::IKey* pKey, Omega::System::IService::socket_map_t& socket_map);

	void Stop()
	{}

	BEGIN_INTERFACE_MAP(TestServiceImpl)
		INTERFACE_ENTRY(Omega::System::IService)
	END_INTERFACE_MAP()
};

#include <stdio.h>

void TestServiceImpl::Start(const Omega::string_t& strName, Omega::Registry::IKey* pKey, Omega::System::IService::socket_map_t& socket_map)
{
	printf("YAY!\n");
}

BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestServiceImpl)
END_LIBRARY_OBJECT_MAP()
