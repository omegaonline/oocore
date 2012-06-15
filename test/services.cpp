#include "../include/Omega/Remoting.h"
#include "../include/OTL/Registry.h"
#include "interfaces.h"

#if defined(_WIN32)

#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace Omega
{
	namespace TestSuite
	{
		extern const Omega::guid_t OID_TestService;
	}
}

#include "Test.h"

const Omega::guid_t Omega::TestSuite::OID_TestService("{16C07A2A-242F-48F5-A10E-1DCA3FADB9A6}");

#if defined(_MSC_VER)
	static const char* s_dll = "TestService.dll";
#elif defined(_WIN32)
	static const char* s_dll = OMEGA_STRINGIZE(BUILD_DIR) "/TestService/.libs/testservice.dll";
#else
	static const char* s_dll = OMEGA_STRINGIZE(BUILD_DIR) "/TestService/testservice.la";
#endif

static bool register_service()
{
	Omega::string_t strOid = Omega::TestSuite::OID_TestService.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("/Local User/Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey->OpenKey("Test.Service",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("OID",strOid);
	ptrSubKey = ptrKey->OpenKey("OIDs/" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("Library",s_dll);

	ptrKey = ptrKey->OpenKey("/System/Services/TestService",Omega::Registry::IKey::OpenCreate);
	ptrKey->SetValue("OID",strOid);
	ptrKey = ptrKey->OpenKey("Connections",Omega::Registry::IKey::OpenCreate);
	ptrKey->SetValue("One","ipv4/stream/tcp/127.0.0.1/7654");

	return true;
}

static bool unregister_service()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("/Local User/Objects");

	if (ptrKey->IsKey("Test.Service"))
		ptrKey->DeleteSubKey("Test.Service");

	Omega::string_t strOid = Omega::TestSuite::OID_TestService.ToString();

	if (ptrKey->IsKey("OIDs/" + strOid))
		ptrKey->DeleteSubKey("OIDs/" + strOid);

	if (ptrKey->IsKey("/System/Services/TestService"))
	{
		ptrKey = ptrKey = ptrKey->OpenKey("/System/Services");
		ptrKey->DeleteSubKey("TestService");
	}

	return true;
}

bool restart_services()
{
#if defined(_WIN32)

	GetServiceController

	Do a restart

#else

	TEST(system("kill -HUP `cat ./ooserverd.pid`") == 0);
	sleep(5);

#endif

	return true;
}

bool service_tests()
{
	// Register the new service
	TEST(register_service());

	// Restart the services
	restart_services();

	// Wait a bit

	// Test the service has a socket listening

	int sock = socket(AF_INET,SOCK_STREAM,0);
	TEST(sock != -1);

	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(7654);

	int err = 0;
	for (size_t c = 0; c < 1000; ++c)
	{
		if (connect(sock,(sockaddr*)&addr,sizeof(addr)) != 0);
#if defined(_WIN32)
		err = GetLastError();
		if (!er || err != ENOENT)
#else
		err = errno;
		if (!err || (err != ENOENT && err != ECONNREFUSED))
#endif
			break;
	}

	if (err)
		add_failure("Failed to connect to test service\n");
	else
	{

	}

	// Restart again to test stopping
	unregister_service();

	// Restart the services
	restart_services();

	return true;
}
