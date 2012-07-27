#include "../../include/Omega/Service.h"
#include "../../include/OTL/OTL.h"

#include <sys/socket.h>
#include <stdio.h>

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
	TestServiceImpl() : m_stop(false)
	{
	}

	~TestServiceImpl()
	{
		printf("DEAD!\n");
	}

	void Run(const Omega::string_t& strName, Omega::Registry::IKey* pKey, Omega::System::IService::socket_map_t& socket_map);
	void Stop();

	BEGIN_INTERFACE_MAP(TestServiceImpl)
		INTERFACE_ENTRY(Omega::System::IService)
	END_INTERFACE_MAP()

private:
	bool m_stop;
};

void TestServiceImpl::Run(const Omega::string_t& strName, Omega::Registry::IKey* pKey, Omega::System::IService::socket_map_t& socket_map)
{
	printf("TestService started!\n");

	/*if (socket_map.empty())
	{
		printf("Socket map!\n");
		return;
	}

	Omega::System::IService::socket_t sock = socket_map.at("One");
	listen(sock,1);

	Omega::System::IService::socket_t nsock = accept(sock,NULL,NULL);*/

	printf("TestService spinning\n");

	while (!m_stop && Omega::HandleRequest())
	{}

	printf("TestService stopping\n");

	//sleep(10);
}

void TestServiceImpl::Stop()
{
	printf("TestService stop signalled\n");
	m_stop = true;
}

BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestServiceImpl)
END_LIBRARY_OBJECT_MAP()
