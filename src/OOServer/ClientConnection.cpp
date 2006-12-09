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

#include "./ClientConnection.h"
#include "./RootManager.h"

ClientConnection::~ClientConnection()
{
	if (handle() != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(handle());
}

void ClientConnection::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	// Stash the handle
	this->handle(new_handle);

	// Open the reader and writer
	if (m_reader.open(*this) != 0 || m_writer.open(*this) != 0)
	{
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) open failed!\n")));
		delete this;
	}
	else
	{
		// Recv the length of the request
		m_header_len = 0;
		ACE_Message_Block* mb;
		ACE_NEW_NORETURN(mb,ACE_Message_Block(1024));
		if (m_reader.read(*mb,sizeof(m_header_len)) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"),ACE_TEXT("reader.read")));
			mb->release();
			delete this;
		}
	}
}

void ClientConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	if (!result.success())
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connection closed\n")));
		mb.release();
		delete this;
		return;
	}

	if (m_header_len==0)
	{
		// Read the header length
		if (result.bytes_transferred() < sizeof(m_header_len))
		{
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid data\n")));
			mb.release();
			delete this;
			return;
		}
	
		m_header_len = *reinterpret_cast<Session::Request::Length*>(mb.rd_ptr());
		
		// Check the request size
		if (m_header_len < sizeof(Session::Request))
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("Invalid request\n")));
			mb.release();
			delete this;
			return;
		}

		// Resize the message block
		if (mb.size(m_header_len) != 0)
		{
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("%p"), ACE_TEXT("data reading\n")));
			mb.release();
			delete this;
			return;
		}

		m_header_len -= sizeof(m_header_len);

		// Issue another read for the rest of the data
		if (m_reader.read(mb,m_header_len) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"),ACE_TEXT("reader.read")));
			mb.release();
			delete this;
		}

		return;
	}
	else
	{
		// Check the header length
		if (result.bytes_transferred() < m_header_len)
		{
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid data\n")));
			mb.release();
			delete this;
			return;
		}

		// Check the request
		Session::Request* pRequest = reinterpret_cast<Session::Request*>(mb.rd_ptr());

		// Ask the root manager for a response...
		Session::Response response = {0};
		RootManager::ROOT_MANAGER::instance()->connect_client(*pRequest,response);
	
		// Try to send the response, reusing mb
		mb.reset();
		if (mb.size(response.cbSize) != 0)
		{
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("%p"), ACE_TEXT("data reading\n")));
			mb.release();
			delete this;
			return;
		}

		if (m_writer.write(mb,response.cbSize) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"),ACE_TEXT("write")));
			mb.release();
			delete this;
		}
	}
}

void ClientConnection::handle_write_stream(const ACE_Asynch_Write_Stream::Result& result)
{
	// All done, close
	result.message_block().release();
	delete this;
}
