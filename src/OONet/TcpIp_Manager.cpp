#include "./TcpIp_Manager.h"

#include <ace/Service_Config.h>
#include <ace/Get_Opt.h>
#include <ace/SOCK_Connector.h>
#include <ace/Connector.h>

#include "./TcpIp_Connector.h"

#include "./OONet_export.h"

ACE_FACTORY_DEFINE(OONet,TcpIp_Manager)

TcpIp_Manager::TcpIp_Manager(void) :
	ACE_Acceptor<TcpIp_Acceptor, ACE_SOCK_ACCEPTOR>(OOCore::GetEngineReactor())
{
	// Artifically AddRef ourselves, because ACE_Svc_Config controls our lifetime
	AddRef();
}

TcpIp_Manager::~TcpIp_Manager(void)
{
}

int TcpIp_Manager::init(int argc, ACE_TCHAR *argv[])
{
	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":p:"),0);
	int option;
	u_short uPort = 0;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('p'):
			uPort = ACE_OS::atoi(cmd_opts.opt_arg());
			break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	ACE_INET_Addr port_addr(uPort);
	if (open(port_addr,OOCore::GetEngineReactor()) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	// Confirm we have a connection
	if (acceptor().get_local_addr(port_addr)==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);

	if (OOCore::RegisterProtocol("tcp",this) != 0)
	{
		close();
		return -1;
	}
	
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening on port %u for TCP/IP connections.\n"),port_addr.get_port_number()));
			
	return 0;
}

bool 
TcpIp_Manager::await_close(void* p)
{
	TcpIp_Manager* pThis = reinterpret_cast<TcpIp_Manager*>(p);

	return (pThis->RefCount()<=1);
}

int 
TcpIp_Manager::fini(void)
{
	// Stop accepting any more connections
	OOCore::UnregisterProtocol("tcp");
	close();

	// Close any open connections
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	for (std::map<ACE_INET_Addr,OOUtil::Object_Ptr<OOCore::Transport_Impl> >::iterator i = m_trans_map.begin(); i!= m_trans_map.end(); ++i)
	{
		i->second->RequestClose();
	}

	guard.release();
	
	// Wait for everyone to close
	ACE_Time_Value wait(DEFAULT_WAIT);
	OOCore::PumpRequests(&wait,await_close,this);
	
	return 0;
}

void 
TcpIp_Manager::add_connection(const ACE_INET_Addr& addr, OOCore::Transport_Impl* trans)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_INET_Addr,OOUtil::Object_Ptr<OOCore::Transport_Impl> >::iterator i=m_trans_map.find(addr);
	if (i==m_trans_map.end())
	{
		m_trans_map[addr] = trans;
	}
}

void 
TcpIp_Manager::remove_connection(OOCore::Transport_Impl* trans)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	for (std::map<ACE_INET_Addr,OOUtil::Object_Ptr<OOCore::Transport_Impl> >::iterator i=m_trans_map.begin();i!=m_trans_map.end();++i)
	{
		if (i->second == trans)
		{
			m_trans_map.erase(i);
			return;
		}
	}
}

OOObject::int32_t 
TcpIp_Manager::Connect(const OOObject::char_t* remote_addr, OOObject::Transport** ppTransport)
{
	// Sort out address
	ACE_INET_Addr addr;
	if (addr.set(remote_addr) != 0)
        ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to resolve address: tcp://%s - %m\n"),ACE_TEXT_CHAR_TO_TCHAR(remote_addr)),-1);

	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	std::map<ACE_INET_Addr,OOUtil::Object_Ptr<OOCore::Transport_Impl> >::iterator i=m_trans_map.find(addr);
	if (i!=m_trans_map.end())
	{
		*ppTransport = i->second;
		(*ppTransport)->AddRef();
		return 0;
	}

	guard.release();

	// Connect to the address
	TcpIp_Connector* conn = 0;
	ACE_Connector<TcpIp_Connector, ACE_SOCK_CONNECTOR> connector(OOCore::GetEngineReactor());
	if (connector.connect(conn,addr)!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to connect to address: tcp://%s - %m\n"),ACE_TEXT_CHAR_TO_TCHAR(remote_addr)),-1);
	
	conn->set_protocol(this);
	conn->AddRef();

	// Put in the map
	add_connection(addr,conn);
	
	if (conn->wait_for_open())
	{
		conn->Release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connecting timed out and gave up\n")),-1);
	}

	*ppTransport = conn;
		
	return 0;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<TcpIp_Manager,ACE_Thread_Mutex>;
template class ACE_Acceptor<TcpIp_Connection, ACE_SOCK_ACCEPTOR>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<TcpIp_Manager,ACE_Thread_Mutex>
#pragma instantiate ACE_Acceptor<TcpIp_Connection, ACE_SOCK_ACCEPTOR>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
