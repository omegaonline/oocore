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

#include "./RootConnection.h"
#include "../OOCore/Session.h"

#include <map>

class SpawnedProcess;

class RootManager : public ACE_Acceptor<RootConnection, ACE_SOCK_ACCEPTOR>
{
public:
	typedef ACE_Singleton<RootManager, ACE_Recursive_Thread_Mutex > ROOT_MANAGER;

	RootManager();

	int open();
	int close();

	void connect_client(const Session::Request& request, Session::Response& response);
	
private:
	RootManager(const RootManager&) {}
	RootManager& operator = (const RootManager&) {}

	ACE_HANDLE	m_config_file;
	std::map<Session::USERID,std::pair<u_short,SpawnedProcess*> > m_mapSpawned;
};

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_