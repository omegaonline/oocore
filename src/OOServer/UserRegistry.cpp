#include "OOServer.h"
#include ".\UserRegistry.h"
#include ".\UserManager.h"

using namespace Omega;
using namespace OTL;

void UserRegistry::Init(UserManager* pManager)
{
	m_ptrRootReg = ObjectImpl<UserRootRegistry>::CreateObjectPtr();
	m_ptrRootReg->Init(pManager);
}
