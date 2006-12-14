/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#include "./SpawnedProcess.h"

#ifndef ACE_WIN32

#include "./UserSession.h"

SpawnedProcess::SpawnedProcess(void)
{
}

SpawnedProcess::~SpawnedProcess(void)
{
}

int SpawnedProcess::Spawn(Session::USERID id, u_short uPort)
{
#error FIX ME!

	pid_t child_id = ACE_OS::fork();
	if (child_id==-1)
	{
		// Error
		return ACE_OS::last_error();
	}
	else if (child_id == 0)
	{
		// We are now the child...
		ACE_OS::setreuid();
		ACE_OS::setregid();
		init_groups();

		// Do something with environment...
		getpwuid();

		return UserMain(uPort);
	}
	else
	{
		// We are the parent...
		return 0;
	}
}

#endif // !ACE_WIN32