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

#include "./OOServer_Root.h"
#include "./ClientConnection.h"
#include "./RootManager.h"

Root::ClientConnection::~ClientConnection()
{
	if (handle() != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(handle());
}

void Root::ClientConnection::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	// Stash the handle
	this->handle(new_handle);

	// Open the reader and writer
	if (m_reader.open(*this) != 0 || m_writer.open(*this) != 0)
	{
        ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::ClientConnection::open")));
		delete this;
	}
	else
	{
		// Recv the length of the request
		m_header_len = 0;
		ACE_Message_Block* mb;
		ACE_NEW_NORETURN(mb,ACE_Message_Block(16));
		if (m_reader.read(*mb,sizeof(m_header_len)) != 0)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::ClientConnection::open")));
			mb->release();
			delete this;
		}
	}
}

void Root::ClientConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = false;
	if (result.success())
	{
		if (m_header_len==0)
		{
			// Read the header length
			if (result.bytes_transferred() == sizeof(m_header_len))
			{
				m_header_len = *reinterpret_cast<ACE_UINT16*>(mb.rd_ptr());
			
				// Check the request size
				if (m_header_len == sizeof(ACE_UINT16) + sizeof(uid_t))
				{
					// Resize the message block
					if (mb.size(m_header_len) == 0)
					{
						m_header_len -= sizeof(m_header_len);

						// Issue another read for the rest of the data
						bSuccess = (m_reader.read(mb,m_header_len) == 0);
					}
				}
			}
		}
		else
		{
			// Check the header length
			if (result.bytes_transferred() == m_header_len)
			{
				// Check the request
				mb.rd_ptr(sizeof(m_header_len));
				
				// Ask the root manager for a response...
				u_short uNewPort = 0;
				ACE_CDR::ULong err = 0;
				ACE_CString strSource;
				if (!Manager::connect_client(*reinterpret_cast<const uid_t*>(mb.rd_ptr()),uNewPort,strSource))
					err = ACE_OS::last_error();
			
				// Try to send the response, reusing mb
				mb.reset();
				bSuccess = (mb.size(1200) != -1 && mb.copy(reinterpret_cast<const char*>(&err),sizeof(err)) != -1);
				if (bSuccess)
				{
					if (err == 0)
					{
						bSuccess = (mb.copy(reinterpret_cast<const char*>(&uNewPort),sizeof(uNewPort)) != -1);
					}
					else
					{
						ACE_UINT16 len = static_cast<ACE_UINT16>(strSource.length());
						if (len > 1023)
							len = 1023;
						bSuccess = (mb.copy(reinterpret_cast<const char*>(&len),sizeof(len)) != -1);
						if (bSuccess)
							bSuccess = (mb.copy(strSource.c_str(),len) != -1);
					}
				}		

				if (bSuccess)
					bSuccess = (m_writer.write(mb,mb.length()) == 0);
			}
		}
	}

	if (!bSuccess)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::ClientConnection::handle_read_stream")));
		mb.release();
		delete this;
	}
}

void Root::ClientConnection::handle_write_stream(const ACE_Asynch_Write_Stream::Result& result)
{
	// All done, close
	result.message_block().release();
	delete this;
}
