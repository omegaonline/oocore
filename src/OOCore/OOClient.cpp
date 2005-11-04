#include "./OOClient.h"

#include <ace/Init_ACE.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./Connection_Manager.h"
#include "./Engine.h"

int OOCore_Export OOClient_Init()
{
	int ret = 0;

	// Make sure ACE is loaded
	if ((ret = (ACE::init()==-1 ? -1 : 0)) == 0)
	{
		if ((ret = ENGINE::instance()->open()) == 0)
		{
			ret = OOCore_Connection_Manager::init();

			if (ret!=0)
				ENGINE::instance()->shutdown();
		}

		if (ret!=0)
			ACE::fini();
	}

	return ret;
}

void OOCore_Export OOClient_Term()
{
	CONNECTION_MANAGER::instance()->close();

	ENGINE::instance()->shutdown();

	ACE::fini();
}

int OOCore_Export OOClient_CreateObject(const OOObj::char_t* object, const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	return CONNECTION_MANAGER::instance()->create_object(object,iid,ppVal);
}
