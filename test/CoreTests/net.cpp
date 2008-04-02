#include <OTL/OTL.h>
#include "Test.h"

class Callback :
	public OTL::ObjectBase,
	public Omega::IO::IAsyncStreamCallback
{
public:
	BEGIN_INTERFACE_MAP(Callback)
		INTERFACE_ENTRY(Omega::IO::IAsyncStreamCallback)
	END_INTERFACE_MAP()
	
// IAsyncStreamCallback members
public:
	void OnSignal(Omega::IO::IAsyncStreamCallback::SignalType_t type, Omega::uint32_t cbBytes, const Omega::byte_t* pData);
};

BEGIN_PROCESS_OBJECT_MAP(L"CoreTests")
END_PROCESS_OBJECT_MAP()

void Callback::OnSignal(Omega::IO::IAsyncStreamCallback::SignalType_t type, Omega::uint32_t cbBytes, const Omega::byte_t* pData)
{
}

bool net_tests()
{
	char szRequest[] = "GET / HTTP/1.1\r\nHost: www.google.co.uk\r\n\r\n";
	char szBuf[2048];

	OTL::ObjectPtr<Omega::IO::IStream> ptrStream;

	// Try sync
	ptrStream.Attach(Omega::IO::OpenStream(L"http://www.google.com"));
	TEST(ptrStream);
	
	ptrStream->WriteBytes((Omega::uint32_t)strlen(szRequest),(Omega::byte_t*)szRequest);

	Omega::uint32_t cbBytes = 2047;
	ptrStream->ReadBytes(cbBytes,(Omega::byte_t*)szBuf);
	szBuf[cbBytes] = '\0';

	// Try async
	OTL::ObjectPtr<OTL::ObjectImpl<Callback> > ptrCallback = OTL::ObjectImpl<Callback>::CreateInstancePtr();

	ptrStream.Attach(Omega::IO::OpenStream(L"http://www.google.com",ptrCallback));
	TEST(ptrStream);
	
	ptrStream->WriteBytes((Omega::uint32_t)strlen(szRequest),(Omega::byte_t*)szRequest);

	Omega::HandleRequests(2000);

	cbBytes = 2047;
	ptrStream->ReadBytes(cbBytes,(Omega::byte_t*)szBuf);
	szBuf[cbBytes] = '\0';

	Omega::HandleRequests(2000);
	
	return true;
}
