#ifndef _OONET_TCPIP_MANAGER_H_INCLUDED_
#define _OONET_TCPIP_MANAGER_H_INCLUDED_

#include <ace/SOCK_Acceptor.h>

#include "../OOCore/OOCore.h"

#include "./TcpIp_Acceptor.h"

class TcpIp_Manager : 
	public ACE_Acceptor<TcpIp_Acceptor, ACE_SOCK_ACCEPTOR>,
	public OOUtil::Object_Impl<OOObject::Protocol>
{
public:
	TcpIp_Manager(void);
	virtual ~TcpIp_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

	void add_connection(const ACE_INET_Addr& addr, OOCore::Transport_Impl* trans);
	void remove_connection(OOCore::Transport_Impl* trans);

private:
	ACE_Thread_Mutex m_lock;
	std::map<ACE_INET_Addr,OOUtil::Object_Ptr<OOCore::Transport_Impl> > m_trans_map;

	static bool await_close(void* p);

// OOCore::Protocol members
public:
	OOObject::int32_t Connect(const OOObject::char_t* remote_addr, OOObject::Transport** ppTransport);
};

#endif // _OONET_TCPIP_MANAGER_H_INCLUDED_
