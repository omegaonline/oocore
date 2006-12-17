#ifndef OOSERVER_USER_CONNECTION_H_INCLUDED_
#define OOSERVER_USER_CONNECTION_H_INCLUDED_

class UserConnection : public ACE_Service_Handler
{		
public:
	UserConnection();
	virtual ~UserConnection();

	void open(ACE_HANDLE new_handle, ACE_Message_Block& mb);
	void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
		
private:
	UserConnection(const UserConnection&) {}
	UserConnection& operator = (const UserConnection&) {}

	static const size_t			s_initial_read = 8;
	ACE_CDR::ULong				m_read_len;
	ACE_Asynch_Read_Stream		m_reader;

	int read();
};

#endif // OOSERVER_USER_CONNECTION_H_INCLUDED_
