#include "../../include/Omega/Service.h"
#include "../../include/OTL/OTL.h"

#if defined(_WIN32)
#include <WinSock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

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
	}

	void Run(const Omega::string_t& strName, Omega::Registry::IKey* pKey, Omega::System::IService::socket_map_t& socket_map);
	void Stop();

	BEGIN_INTERFACE_MAP(TestServiceImpl)
		INTERFACE_ENTRY(Omega::System::IService)
	END_INTERFACE_MAP()

private:
	bool m_stop;
};

void TestServiceImpl::Run(const Omega::string_t& strName, Omega::Registry::IKey* /*pKey*/, Omega::System::IService::socket_map_t& socket_map)
{
	printf("%s started!\n",strName.c_str());

	if (socket_map.empty())
		return;

	Omega::System::IService::socket_t sock = socket_map["One"];
	listen(sock,1);

	printf("%s spinning\n",strName.c_str());

	while (!m_stop)
	{
		while (Omega::HandleRequest(500))
		{}

		Omega::System::IService::socket_t nsock = accept(sock,NULL,NULL);
		if (nsock != Omega::System::IService::socket_t(-1))
			send(nsock,"HELLO\n",6,0);

#if defined(_WIN32)
		closesocket(nsock);
#else
		close(nsock);
#endif
	}

	printf("%s stopping\n",strName.c_str());
}

void TestServiceImpl::Stop()
{
	m_stop = true;
}

BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(TestServiceImpl)
END_LIBRARY_OBJECT_MAP()
