#include "./OOClient.h"

#include <ace/Init_ACE.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./Connection_Manager.h"

int OOCore_Export OOClient_Init()
{
	int ret = 0;

	// Make sure ACE is loaded
	ret = (ACE::init()==-1 ? -1 : 0);
	if (ret==0)
	{
		// Open a client connection
		ret = OOCore_Connection_Manager::init();
		if (ret==0)
		{
			// Perform our initialisation here
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connected\n")));
		}
				
		if (ret!=0)
            ACE::fini();
	}

	return ret;
}

void OOCore_Export OOClient_Term()
{
	// Perform our termination here
	CONNECTION_MANAGER::instance()->close();
	CONNECTION_MANAGER::instance()->shutdown();

	ACE::fini();
}

int OOCore_Export OOClient_CreateObject(const OOObj::char_t* object, const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	return CONNECTION_MANAGER::instance()->create_object(object,iid,ppVal);
}