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
#include "./RootConnection.h"
#include "./RootManager.h"

Root::Connection::Connection(HandlerBase* pBase, const ACE_CString& key) :
	ACE_Service_Handler(),
	m_pBase(pBase),
	m_id(key)
{
}

Root::Connection::~Connection()
{
	ACE_HANDLE my_handle = handle();

	if (m_pBase)
		m_pBase->root_connection_closed(m_id,my_handle);

	if (my_handle != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(my_handle);
}

bool Root::Connection::open(ACE_HANDLE new_handle)
{
	// Stash the handle
	this->handle(new_handle);

	// Open the reader
	if (m_reader.open(*this) != 0)
	    ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Connection::open")),false);

	return read();
}

bool Root::Connection::read()
{
	// Recv the length of the request
	m_read_len = 0;
	ACE_Message_Block* mb;
	ACE_NEW_RETURN(mb,ACE_Message_Block(1024),false);

	// Align the message block for CDR
	ACE_CDR::mb_align(mb);

	// Start an async read
	if (m_reader.read(*mb,s_initial_read) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Connection::read")));
		mb->release();
		return false;
	}

	return true;
}

void Root::Connection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = false;
	if (result.success())
	{
		if (m_read_len==0)
		{
			// Read the header length
			if (result.bytes_transferred() == s_initial_read)
			{
				// Create a temp input CDR
				ACE_InputCDR input(mb.data_block(),ACE_Message_Block::DONT_DELETE,static_cast<size_t>(mb.rd_ptr() - mb.base()),static_cast<size_t>(mb.wr_ptr() - mb.base()));
				input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

				// Read and set the byte order
				ACE_CDR::Octet byte_order;
				ACE_CDR::Octet version;
				if (input.read_octet(byte_order) && input.read_octet(version))
				{
					input.reset_byte_order(byte_order);

					// Read the length
					input >> m_read_len;
					if (input.good_bit())
					{
						// Resize the message block
						if (mb.size(m_read_len) == 0)
						{
							// Subtract what we have already read
							m_read_len -= static_cast<ACE_CDR::ULong>(mb.length());

							// Issue another read for the rest of the data
							bSuccess = (m_reader.read(mb,m_read_len) == 0);
							if (bSuccess)
								return;
						}
					}
				}
			}
		}
		else
		{
			// Check the header length
			if (result.bytes_transferred() == m_read_len)
			{
				// Create a new input CDR
				ACE_InputCDR* input = 0;
				size_t rd_ptr = static_cast<size_t>(mb.rd_ptr() - mb.base());
				size_t wr_ptr = static_cast<size_t>(mb.wr_ptr() - mb.base());
				ACE_NEW_NORETURN(input,ACE_InputCDR(mb.replace_data_block(0),0,rd_ptr,wr_ptr));
				if (input)
				{
					input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Push into the HandlerBase queue...
					if (m_pBase->enqueue_root_request(input,handle()))
					{
						// Start a new read
						bSuccess = read();
					}

					if (!bSuccess)
						delete input;
				}
			}
		}
	}

	mb.release();

	if (!bSuccess)
	{
#if defined(ACE_WIN32)
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_IO_PENDING && dwErr != ERROR_SUCCESS && dwErr != WSAENOTSOCK)
#endif
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Root::Connection::handle_read_stream")));
		delete this;
	}
}
