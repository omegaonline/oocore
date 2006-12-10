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

#ifndef OOSERVER_ROOT_MANAGER_H_INCLUDED_
#define OOSERVER_ROOT_MANAGER_H_INCLUDED_

#include "./LocalAcceptor.h"
#include "./SpawnedProcess.h"
#include "./ClientConnection.h"

#include <ace/Singleton.h>

#include <map>

class RootManager : public LocalAcceptor<ClientConnection>
{
public:
	typedef ACE_Singleton<RootManager, ACE_Recursive_Thread_Mutex > ROOT_MANAGER;

	RootManager();

	int init();
	void close();
	void term();

	void connect_client(const Session::Request& request, Session::Response& response);
	
private:
	RootManager(const RootManager&) {}
	RootManager& operator = (const RootManager&) {}

	ACE_Thread_Mutex	m_lock;
	ACE_HANDLE			m_config_file;

	struct UserProcess
	{

		u_short				uPort;
		SpawnedProcess*		pSpawn;
	};

	std::map<SpawnedProcess::USERID,UserProcess> m_mapSpawned;

	void spawn_client(const Session::Request& request, Session::Response& response, UserProcess& process);
};

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_