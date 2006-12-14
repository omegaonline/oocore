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

RootConnection::RootConnection(RootBase* pBase, const SpawnedProcess::USERID& key) : 
	ACE_Service_Handler(),
	m_pBase(pBase),
	m_id(key)
{
}

RootConnection::~RootConnection()
{
	ACE_HANDLE my_handle = handle();
	
	if (m_pBase)
		m_pBase->root_connection_closed(m_id,my_handle);

	if (my_handle != ACE_INVALID_HANDLE)
		ACE_OS::closesocket(my_handle);
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

	// Align the message block for CDR
	ACE_CDR::mb_align(mb);

	// Start an async read
	if (m_reader.read(*mb,sizeof(m_read_len)+sizeof(ACE_CDR::Octet)) != 0)
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
			if (result.bytes_transferred() == sizeof(m_read_len)+sizeof(ACE_CDR::Boolean))
			{
				// Create a temp input CDR
				ACE_InputCDR input(mb.data_block(),ACE_Message_Block::DONT_DELETE);
				input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

				// Read and set the byte order
				ACE_CDR::Octet byte_order;
				if (input.read_octet(byte_order))
				{
					input.reset_byte_order(byte_order);

					// Read the length
					input >> m_read_len;
					if (input.good_bit())
					{
						// Resize the message block
						if (mb.size(m_read_len + mb.length()) == 0)
						{
							// Issue another read for the rest of the data
							bSuccess = (m_reader.read(mb,m_read_len) == 0);
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
				ACE_NEW_NORETURN(input,ACE_InputCDR(mb.replace_data_block(0),0));
				if (input)
				{
					input->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Read and set the byte order
					ACE_CDR::Octet byte_order;
					if (input->read_octet(byte_order))
					{
						input->reset_byte_order(byte_order);

						// Read the length
						ACE_CDR::ULong read_len;
						(*input) >> read_len;
						if (input->good_bit() && read_len == m_read_len)
						{
							// Push into the RootBase queue...
							if (m_pBase->enque_root_request(input,handle()) > 0)
							{
								// Start a new read
								read();

								bSuccess = true;
							}
						}
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
		ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("RootConnection::handle_read_stream")));
		delete this;
	}
}
