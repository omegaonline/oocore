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

#ifndef OOSERVER_ROOT_CONNECTION_H_INCLUDED_
#define OOSERVER_ROOT_CONNECTION_H_INCLUDED_

#include <ace/Asynch_IO.h>

#include "../OOCore/Session.h"

class RootManager;

class RootConnection : public ACE_Service_Handler
{		
public:
	RootConnection() : ACE_Service_Handler()
	{}

	virtual ~RootConnection();

	void open(ACE_HANDLE new_handle, ACE_Message_Block &message_block);
	void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
	void handle_write_stream(const ACE_Asynch_Write_Stream::Result& result);
	
private:
	RootConnection(const RootConnection&) {}
	RootConnection& operator = (const RootConnection&) {}

	Session::Request::Length	m_header_len;
	ACE_Asynch_Read_Stream		m_reader;
	ACE_Asynch_Write_Stream		m_writer;
};

#endif // OOSERVER_ROOTt_CONNECTION_H_INCLUDED_
