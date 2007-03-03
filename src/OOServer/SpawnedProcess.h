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
	void Kill();

	static int ResolveTokenToUid(Session::TOKEN token, ACE_CString& uid);
	static int GetSandboxUid(ACE_CString& uid);

private:

#ifdef ACE_WIN32

	HANDLE	m_hToken;
	HANDLE	m_hProfile;
	HANDLE	m_hProcess;
	
	DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile);
	DWORD SpawnFromToken(HANDLE hToken, u_short uPort);
	static int LogonSandboxUser(HANDLE* phToken);

#else // !ACE_WIN32
	
#endif // ACE_WIN32

};

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
