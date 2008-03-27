#include <OTL/OTL.h>
#include "Test.h"

bool net_tests()
{
	OTL::ObjectPtr<Omega::IO::IStream> ptrStream;
	ptrStream.Attach(Omega::IO::OpenStream(L"http://www.google.com"));

	TEST(ptrStream);

	return true;
}
