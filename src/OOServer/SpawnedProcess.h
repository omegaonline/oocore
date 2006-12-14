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

#ifndef OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
#define OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

#include "../OOCore/Session.h"

class SpawnedProcess
{
public:
	SpawnedProcess(void);
	~SpawnedProcess(void);

	int Spawn(Session::TOKEN id, u_short uPort);
	int SpawnSandbox();

	bool IsRunning();
	int Close(ACE_Time_Value* wait = 0);

#ifdef ACE_WIN32

	typedef ACE_TString USERID;

	static int ResolveTokenToUid(Session::TOKEN token, USERID& uid);

private:
	HANDLE	m_hToken;
	HANDLE	m_hProfile;
	HANDLE	m_hProcess;
	
	DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile);
	DWORD SpawnFromToken(HANDLE hToken, u_short uPort);

#else // !ACE_WIN32

	typedef uid_t USERID;
	static int ResolveTokenToUid(Session::TOKEN token, USERID& uid)
	{
		uid = token;
		return 0;
	}

private:
	
#endif // ACE_WIN32

};

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_