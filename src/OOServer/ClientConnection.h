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

#ifndef OOSERVER_CLIENT_CONNECTION_H_INCLUDED_
#define OOSERVER_CLIENT_CONNECTION_H_INCLUDED_

#include "../OOCore/Session.h"

namespace Root
{

class Manager;

class ClientConnection : public ACE_Service_Handler
{
public:
	ClientConnection() : ACE_Service_Handler()
	{}

	virtual ~ClientConnection();

	void open(ACE_HANDLE new_handle, ACE_Message_Block &message_block);
	void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
	void handle_write_stream(const ACE_Asynch_Write_Stream::Result& result);

private:
	ClientConnection(const ClientConnection&) {}
	ClientConnection& operator = (const ClientConnection&) { return *this; }

	ACE_UINT16					m_header_len;
	ACE_Asynch_Read_Stream		m_reader;
	ACE_Asynch_Write_Stream		m_writer;
};

}

#endif // OOSERVER_CLIENT_CONNECTION_H_INCLUDED_
