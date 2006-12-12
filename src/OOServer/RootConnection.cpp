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

#include "./RootConnection.h"
#include "./RootManager.h"

RootConnection::RootConnection(RootBase* pBase, SpawnedProcess::USERID key) : 
	ACE_Service_Handler(),
	m_pBase(pBase),
	m_id(key)
{
}

RootConnection::~RootConnection()
{
	if (handle() != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(handle());

	if (m_pBase)
		m_pBase->connection_closed(m_id);
}

void RootConnection::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	// Stash the handle
	this->handle(new_handle);

	// Open the reader
	if (m_reader.open(*this) != 0)
	{
        ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("RootConnection::open")));
		delete this;
		return;
	}
	
	read();
}

void RootConnection::read()
{
	// Recv the length of the request
	m_read_len = 0;
	ACE_Message_Block* mb;
	ACE_NEW_NORETURN(mb,ACE_Message_Block(1024));
	if (m_reader.read(*mb,sizeof(m_read_len)) != 0)
	{
		ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("RootConnection::read")));
		mb->release();
		delete this;
	}
}

void RootConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();
	
	bool bSuccess = false;
	if (result.success())
	{
		if (m_read_len==0)
		{
			// Read the header length
			if (result.bytes_transferred() == sizeof(m_read_len))
			{
				m_read_len = *reinterpret_cast<RootProtocol::Header::Length*>(mb.rd_ptr());
		
				// Resize the message block
				if (mb.size(m_read_len) == 0)
				{
					m_read_len -= sizeof(m_read_len);

					// Issue another read for the rest of the data
					bSuccess = (m_reader.read(mb,m_read_len) == 0);
				}
			}
		}
		else
		{
			// Check the header length
			if (result.bytes_transferred() == m_read_len)
			{
				bSuccess = true;

				// Push into the RootManager queue...
				m_pBase->enque_request(mb,handle());

				mb.release();

				// Start a new read
				read();
			}
		}
	}

	if (!bSuccess)
	{
		ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("RootConnection::handle_read_stream")));
		mb.release();
		delete this;
	}
}
