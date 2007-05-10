/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_LOCAL_ACCEPTOR_H_INCLUDED_
#define OOSERVER_LOCAL_ACCEPTOR_H_INCLUDED_

template <class HANDLER>
class LocalAcceptor : public ACE_Asynch_Acceptor<HANDLER>
{
public:
	virtual int open(const ACE_INET_Addr &address,
                    size_t bytes_to_read = 0,
                    int pass_addresses = 1,
                    int backlog = ACE_DEFAULT_ASYNCH_BACKLOG,
                    int reuse_addr = 1,
                    ACE_Proactor *proactor = 0,
                    int validate_new_connection = 1,
                    int reissue_accept = 1,
                    int number_of_initial_accepts = -1)
	{ 
		return ACE_Asynch_Acceptor<HANDLER>::open(address,bytes_to_read,pass_addresses,backlog,reuse_addr,proactor,validate_new_connection,reissue_accept,number_of_initial_accepts);
	}

	virtual int validate_connection(const ACE_Asynch_Accept::Result& /*result*/, const ACE_INET_Addr& remote, const ACE_INET_Addr& /*local*/)
	{
		return remote.is_loopback() ? 0 : -1;
	}
};

#endif // OOSERVER_LOCAL_ACCEPTOR_H_INCLUDED_
