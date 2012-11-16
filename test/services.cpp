#include "../include/Omega/Service.h"
#include "../include/OTL/Registry.h"
#include "interfaces.h"

#if defined(_WIN32)
#include <winsock2.h>
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

Omega::string_t make_absolute(const char* wsz);

const Omega::guid_t Omega::TestSuite::OID_TestService("{32A13162-BC9C-2CC1-531A-F0A8BF153E0D}");

#if defined(_MSC_VER)
	#if defined(WIN64_HYBRID)
		static const char* s_dll = "..\\x64\\TestService_msvc.dll";
	#else
		static const char* s_dll = "..\\Win32\\TestService_msvc.dll";
	#endif
#elif defined(_WIN32)
	static const char* s_dll = OMEGA_STRINGIZE(BUILD_DIR) "/TestService/.libs/testservice.dll";
#else
	static const char* s_dll = OMEGA_STRINGIZE(BUILD_DIR) "/TestService/testservice.la";
#endif

static bool register_service()
{
	Omega::string_t strOid = Omega::TestSuite::OID_TestService.ToString();

	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("/System/Sandbox/Objects",Omega::Registry::IKey::OpenCreate);
	OTL::ObjectPtr<Omega::Registry::IKey> ptrSubKey = ptrKey->OpenKey("Test.Service",Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("OID",strOid);
	ptrSubKey = ptrKey->OpenKey("OIDs/" + strOid,Omega::Registry::IKey::OpenCreate);
	ptrSubKey->SetValue("Library",make_absolute(s_dll));

	ptrKey = ptrKey->OpenKey("/System/Services/TestService",Omega::Registry::IKey::OpenCreate);
	ptrKey->SetValue("OID","Test.Service");
	ptrKey = ptrKey->OpenKey("Connections",Omega::Registry::IKey::OpenCreate);
	ptrKey->SetValue("One","ipv4/stream/tcp/127.0.0.1/7654");

	return true;
}

static bool unregister_service()
{
	OTL::ObjectPtr<Omega::Registry::IKey> ptrKey("/System/Sandbox/Objects");

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

static bool start_service()
{
	OTL::ObjectPtr<Omega::System::IServiceController> ptrController(Omega::System::OID_ServiceController);

	if (!ptrController->IsServiceRunning("TestService"))
		ptrController->StartService("TestService");

	return true;
}

static bool stop_service()
{
	OTL::ObjectPtr<Omega::System::IServiceController> ptrController(Omega::System::OID_ServiceController);

	if (ptrController->IsServiceRunning("TestService"))
		ptrController->StopService("TestService");

	return true;
}

bool service_tests()
{
	// Stop any services
	TEST(stop_service());

	// Register the new service
	TEST(register_service());

	TEST(start_service());

	// Test the service has a socket listening
	Omega::System::IServiceController::service_set_t svcs = OTL::ObjectPtr<Omega::System::IServiceController>(Omega::System::OID_ServiceController)->GetRunningServices();
	TEST(svcs.find("TestService") != svcs.end());

#if _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
	SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
#else
	int sock = socket(AF_INET,SOCK_STREAM,0);
#endif

	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(7654);

	int err = 0;
	for (size_t c = 0; c < 1000; ++c)
	{
		if (connect(sock,(sockaddr*)&addr,sizeof(addr)) == 0)
		{
			err = 0;
			break;
		}

#if defined(_WIN32)
		err = WSAGetLastError();
		if (err != WSAECONNREFUSED)
#else
		err = errno;
		if (err != ENOENT && err != ECONNREFUSED)
#endif
			break;
	}

	if (err)
	{
		add_failure("Failed to connect to test service\n");
	}
	else
	{
		char buf[256] = {0};
		recv(sock,buf,6,0);
	}

#if defined(_WIN32)
	closesocket(sock);
#else
	close(sock);
#endif

	stop_service();

	unregister_service();

	return true;
}
