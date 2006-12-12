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

#include "./RootProtocol.h"
#include "./SpawnedProcess.h"

#include <ace/Asynch_IO.h>

class RootBase
{
public:
	virtual void enque_request(ACE_Message_Block& mb, ACE_HANDLE handle) = 0;
	virtual void connection_closed(SpawnedProcess::USERID key) = 0;
};

class RootConnection : public ACE_Service_Handler
{		
public:
	RootConnection(RootBase* pBase, SpawnedProcess::USERID key);
	virtual ~RootConnection();

	void open(ACE_HANDLE new_handle, ACE_Message_Block &message_block);
	void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
		
private:
	RootConnection(const RootConnection&) {}
	RootConnection& operator = (const RootConnection&) {}

	RootBase*							m_pBase;
	SpawnedProcess::USERID				m_id;
	RootProtocol::Header::Length		m_read_len;
	ACE_Asynch_Read_Stream				m_reader;

	void read();
};

#endif // OOSERVER_ROOT_CONNECTION_H_INCLUDED_
