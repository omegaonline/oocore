#include "./OOCore.h"
//#include "./OOCore_Impl.h"
#include "./Register.h"
#include "./Binding.h"

DEFINE_IID(OOObject::Object,45F040A3-5386-413e-AB21-7FA35EFCB7DD);
DEFINE_IID(OOCore::Stub,D8B1513D-967B-429e-8403-31650213DA21);
DEFINE_IID(OOCore::InputStream,86F468DB-953F-4be0-A8EB-D9A344C104E3);
DEFINE_IID(OOCore::OutputStream,0FA60065-8C8A-463b-9B01-D080E03EF39F);
DEFINE_IID(OOCore::Transport,33EE56E9-9748-43ce-A71C-516ACE28925C);
DEFINE_IID(OOCore::ProxyStubManager,F3EB63E5-602A-4155-8F52-F11FF502EFE5);
DEFINE_IID(OOCore::RemoteObjectFactory,B1BC71BE-4DCC-4f0f-8483-A75D35126D2A);
DEFINE_IID(OOCore::Server,B4B5BF71-58DF-4001-BD0B-72496463E3C3);

REGISTER_PROXYSTUB(OOCore,RemoteObjectFactory,OOCore)
REGISTER_PROXYSTUB(OOCore,Server,OOCore)

#ifdef _DEBUG
#include "./Test.h"
DEFINE_IID(OOCore::Test,6AAE8C33-699A-4414-AF84-25E74E693207);
REGISTER_PROXYSTUB(OOCore,Test,OOCore)
#endif

OOCore_Export int 
OOCore::InitAsServer()
{
	return Impl::BINDING::instance()->launch(true);
}
