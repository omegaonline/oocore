#include "OOServer.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

namespace User
{
	// OTL requires this...
	class OOServerModule : public ProcessModule
	{
	public:
	};
}

BEGIN_PROCESS_OBJECT_MAP_EX(User,OOServerModule)
END_PROCESS_OBJECT_MAP()
