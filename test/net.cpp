#if 0

#include "../include/Omega/Omega.h"
#include "../include/OTL/OTL.h"
#include "../include/OONet/Http.h"
#include "Test.h"

BEGIN_PROCESS_OBJECT_MAP()
END_PROCESS_OBJECT_MAP()

class StreamNotify :
		public OTL::ObjectBase,
		public Omega::IO::IAsyncStreamNotify
{
public:
	StreamNotify() : m_state(0)
	{}

	bool wait_for_state(int state)
	{
		while (Omega::HandleRequest(15000) && m_state != state)
		{}

		return m_state != state;
	}

	BEGIN_INTERFACE_MAP(StreamNotify)
	INTERFACE_ENTRY(Omega::IO::IAsyncStreamNotify)
	END_INTERFACE_MAP()

private:
	int m_state;

// IAsyncStreamNotify members
public:
	void OnOpened();
	void OnRead(Omega::uint32_t cbBytes, const Omega::byte_t* pData);
	void OnWritten(Omega::uint32_t cbBytes);
	void OnError(Omega::IException* pE);
};

class HttpNotify :
		public OTL::ObjectBase,
		public Omega::Net::Http::IRequestNotify
{
public:
	BEGIN_INTERFACE_MAP(HttpNotify)
	INTERFACE_ENTRY(Omega::Net::Http::IRequestNotify)
	END_INTERFACE_MAP()

// IRequestNotify members
public:
	void OnResponseStart(Omega::uint16_t nCode, const Omega::string_t& strMsg);
	void OnResponseDataAvailable();
	void OnResponseComplete();
	void OnError(Omega::IException* pE);
};

void StreamNotify::OnOpened()
{
	m_state = 1;
}

void StreamNotify::OnRead(Omega::uint32_t cbBytes, const Omega::byte_t* pData)
{
	char szBuf[2048];
	size_t len = 2047;
	if (cbBytes < len)
		len = (size_t)cbBytes;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	strncpy_s(szBuf,(const char*)pData,len);
#else
	strncpy(szBuf,(const char*)pData,len);
#endif

	m_state = 2;
}

void StreamNotify::OnWritten(Omega::uint32_t /*cbBytes*/)
{
	m_state = 3;
}

void StreamNotify::OnError(Omega::IException* pE)
{
	output_exception(pE);
	m_state = 4;
}

static bool stream_tests()
{
	char szRequest[] = "GET / HTTP/1.1\r\nHost: www.google.co.uk\r\n\r\n";
	char szBuf[2048];

	OTL::ObjectPtr<Omega::IO::IStream> ptrStream;

	// Try sync
	ptrStream.Attach(Omega::IO::OpenStream(L"http://www.google.com"));
	TEST(ptrStream);

	ptrStream->WriteBytes(static_cast<Omega::uint32_t>(strlen(szRequest)),(Omega::byte_t*)szRequest);

	Omega::uint32_t cbBytes = 2047;
	ptrStream->ReadBytes(cbBytes,(Omega::byte_t*)szBuf);
	szBuf[cbBytes] = '\0';

	// Try async
	OTL::ObjectPtr<OTL::ObjectImpl<StreamNotify> > ptrNotify = OTL::ObjectImpl<StreamNotify>::CreateInstancePtr();

	ptrStream.Attach(Omega::IO::OpenStream(L"http://www.google.com",ptrNotify));
	TEST(ptrStream);

	// Wait for connect
	ptrNotify->wait_for_state(1);

	ptrStream->WriteBytes(static_cast<Omega::uint32_t>(strlen(szRequest)),(Omega::byte_t*)szRequest);

	ptrNotify->wait_for_state(3);

	cbBytes = 2047;
	ptrStream->ReadBytes(cbBytes,0);

	ptrNotify->wait_for_state(2);

	return true;
}

/*static bool http_tests_sync()
{
    OTL::ObjectPtr<Omega::Net::Http::IRequest> ptrRequest(Omega::Net::Http::OID_StdHttpRequest);

    ptrRequest->Open(L"GET",L"http://www.google.com");
    ptrRequest->Send();

    Omega::uint32_t cbBytes = 0;
    ptrRequest->ResponseBody(cbBytes,0);

    Omega::byte_t* pBuf = new Omega::byte_t[cbBytes+1];
    ptrRequest->ResponseBody(cbBytes,pBuf);

    pBuf[cbBytes] = '\0';
    //output("\n\n%s\n\n",(const char*)pBuf);

    delete [] pBuf;

    OTL::ObjectPtr<Omega::IO::IStream> ptrStream;
    ptrStream.Attach(ptrRequest->ResponseStream());

    for (;;)
    {
        Omega::byte_t szBuf[256];
        Omega::uint64_t cbBytes = 255;
        ptrStream->ReadBytes(cbBytes,szBuf);

        if (cbBytes == 0)
            break;

        szBuf[cbBytes] = '\0';
        //output("%s",(const char*)szBuf);
    }
    //output("\n\n");

    // This one will barf because it's https
    ptrRequest->Open(L"GET",L"https://www.tropicalstormsoftware.com/exchange");
    ptrRequest->Send();

    ptrStream.Attach(ptrRequest->ResponseStream());

    for (;;)
    {
        Omega::byte_t szBuf[256];
        Omega::uint64_t cbBytes = 255;
        ptrStream->ReadBytes(cbBytes,szBuf);

        if (cbBytes == 0)
            break;

        szBuf[cbBytes] = '\0';
        //output("%s",(const char*)szBuf);
    }
    //output("\n\n");

    return true;
}*/

void HttpNotify::OnResponseStart(Omega::uint16_t /*nCode*/, const Omega::string_t& /*strMsg*/)
{
}

void HttpNotify::OnResponseDataAvailable()
{
}

void HttpNotify::OnResponseComplete()
{
}

void HttpNotify::OnError(Omega::IException* pE)
{
	output_exception(pE);
}

/*static bool http_tests_async()
{
    OTL::ObjectPtr<OTL::ObjectImpl<HttpNotify> > ptrNotify = OTL::ObjectImpl<HttpNotify>::CreateInstancePtr();
    OTL::ObjectPtr<Omega::Net::Http::IRequest> ptrRequest(Omega::Net::Http::OID_StdHttpRequest);

    ptrRequest->Open(L"GET",L"http://www.google.com",ptrNotify);
    ptrRequest->Send();

    if (!ptrRequest->WaitForResponse(30000))
        return false;

    Omega::uint32_t cbBytes = 0;
    ptrRequest->ResponseBody(cbBytes,0);

    Omega::byte_t* pBuf = new Omega::byte_t[cbBytes+1];
    ptrRequest->ResponseBody(cbBytes,pBuf);

    pBuf[cbBytes] = '\0';
    //output("\n\n%s\n\n",(const char*)pBuf);

    delete [] pBuf;

    OTL::ObjectPtr<Omega::IO::IStream> ptrStream;
    ptrStream.Attach(ptrRequest->ResponseStream());

    for (;;)
    {
        Omega::byte_t szBuf[256];
        Omega::uint64_t cbBytes = 255;
        ptrStream->ReadBytes(cbBytes,szBuf);

        if (cbBytes == 0)
            break;

        szBuf[cbBytes] = '\0';
        //output("%s",(const char*)szBuf);
    }

    //output("\n\n");

    return true;
}*/

bool net_tests()
{
	TEST(stream_tests());
//  TEST(http_tests_sync());
//  TEST(http_tests_async());

	return true;
}

#endif
