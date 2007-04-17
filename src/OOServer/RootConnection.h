/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_CONNECTION_H_INCLUDED_
#define OOSERVER_ROOT_CONNECTION_H_INCLUDED_

class RootBase
{
public:
	virtual int enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle) = 0;
	virtual void root_connection_closed(const ACE_CString& key, ACE_HANDLE handle) = 0;
};

namespace Root
{

class Connection : public ACE_Service_Handler
{		
public:
	Connection(RootBase* pBase, const ACE_CString& key);
	virtual ~Connection();

	int open(ACE_HANDLE new_handle);
	void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
		
private:
	Connection(const Connection&) {}
	Connection& operator = (const Connection&) {}

	static const size_t			s_initial_read = 8;
	RootBase*					m_pBase;
	ACE_CString					m_id;
	ACE_CDR::ULong				m_read_len;
	ACE_Asynch_Read_Stream		m_reader;

	int read();
};

}

#endif // OOSERVER_ROOT_CONNECTION_H_INCLUDED_
