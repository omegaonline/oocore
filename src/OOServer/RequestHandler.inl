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

template <class REQUEST>
int RequestHandler<REQUEST>::enqueue_request(REQUEST* req)
{
	return m_msg_queue.enqueue_prio(req);
}

template <class REQUEST>
void RequestHandler<REQUEST>::stop()
{
	// Stop the message queue
	m_msg_queue.close();
}

template <class REQUEST>
int RequestHandler<REQUEST>::wait_for_response(ACE_HANDLE handle, ACE_CDR::ULong trans_id, REQUEST*& response, ACE_Time_Value* deadline)
{
	for (;;)
	{
		// Get the next message
		REQUEST* req;
		int nLeft = m_msg_queue.dequeue_prio(req,deadline);
		if (nLeft == -1)
			return -1;

		ACE_InputCDR& input = *req->input();

		for (int i=0;i<2;++i)
		{
			// Read and set the byte order
			ACE_CDR::Octet byte_order;
			if (input.read_octet(byte_order))
			{
				input.reset_byte_order(byte_order);

				// Read the header
				ACE_CDR::ULong req_dline_secs;
				ACE_CDR::ULong req_dline_usecs;
				ACE_CDR::ULong msg_len;
				ACE_CDR::ULong request_trans_id;
				ACE_CDR::Boolean bIsRequest;

				input >> msg_len;
				input >> req_dline_secs;
				input >> req_dline_usecs;
				input >> request_trans_id;
				input.read_boolean(bIsRequest);

				if (input.good_bit())
				{
					// Check the timeout value...
					ACE_Time_Value request_deadline(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));
					
					// See if we want to process it...
					if (bIsRequest)
					{
						// Process the message...
						process_request(req,request_trans_id,&request_deadline);

						// process_request is expected to delete req;
						req = 0;
					}
					else if (!bIsRequest && 
							!expired_request(request_trans_id) && 
							request_trans_id == trans_id &&
							handle == req->handle())
					{
						void* TODO; // Do something with the deadline!

						// Its the request we have been waiting for...
						response = req;
						return 0;
					}
					else
					{
						// If there is still more in the queue
						REQUEST* next_req = 0;
						if (nLeft > 0)
							m_msg_queue.dequeue_prio(next_req,deadline);
						
						// Put the original back in the queue, its not for us...
						int ret = m_msg_queue.enqueue_head(req,deadline);
						if (ret < 0)
						{
							if (next_req)
								delete next_req;
							
							delete req;
							return -1;
						}

						if (next_req)
						{
							req = next_req;
							continue;
						}
					}
				}
			}
			// If we get here then we have finished with req
			break;
		}

		// Done with this request
		delete req;
	}
}

template <class REQUEST>
int RequestHandler<REQUEST>::pump_requests(ACE_Time_Value* deadline)
{
	REQUEST* response = 0;
	int ret = wait_for_response(ACE_INVALID_HANDLE,0,response,deadline);
	if (response)
		delete response;
	return ret;
}

template <class REQUEST>
int RequestHandler<REQUEST>::send_synch(ACE_HANDLE handle, const ACE_OutputCDR& request, REQUEST*& response, ACE_Time_Value* wait)
{
	// Set the deadline
	ACE_Time_Value deadline(ACE_OS::gettimeofday() + *wait);

	// Check the size
	if (request.total_length() + header_padding > ACE_UINT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	// Write the header info
	ACE_OutputCDR header(1024);
	ACE_CDR::ULong trans_id = next_trans_id();
	if (build_header(trans_id,header,request,deadline) != 0)
	{
		cancel_trans_id(trans_id);
		return -1;
	}

	// TODO RICK! Need to work out what header_padding is...
	size_t request_len = request.total_length();
	size_t header_len = header.total_length();

	::DebugBreak();

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),wait,&sent);
	if (res == -1 || sent < header.total_length())
	{
		cancel_trans_id(trans_id);
		return -1;
	}
	
	// Wait for response...
	int ret = wait_for_response(handle,trans_id,response,&deadline);
	if (ret != 0)
		cancel_trans_id(trans_id);

	return ret;
}

template <class REQUEST>
int RequestHandler<REQUEST>::send_asynch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_Time_Value* wait)
{
	// Set the deadline
	ACE_Time_Value deadline(ACE_OS::gettimeofday() + *wait);

	// Check the size
	if (request.total_length() + header_padding > ACE_UINT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return -1;
	}

	// Write the header info
	ACE_OutputCDR header(1024);
	if (build_header(0,header,request,deadline) == -1)
		return -1;

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),wait,&sent);
	if (res == -1 || sent < header.total_length())
		return -1;
		
	return 0;
}

template <class REQUEST>
int RequestHandler<REQUEST>::build_header(ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_OutputCDR& request, const ACE_Time_Value& deadline)
{
	ACE_CDR::Octet byte_order = static_cast<ACE_CDR::Octet>(header.byte_order());
	header << byte_order;
	if (!header.good_bit())
		return -1;

	ACE_CDR::Boolean bIsRequest = true;
	ACE_CDR::ULong msg_len = static_cast<ACE_CDR::ULong>(request.total_length() + header_padding);
	header << msg_len; 
	header << static_cast<const timeval*>(deadline)->tv_sec;
	header << static_cast<const timeval*>(deadline)->tv_usec;
	header << trans_id;
	header.write_boolean(bIsRequest);

	if (!header.good_bit())
		return -1;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream	
	header.write_octet_array_mb(request.begin());
	if (!header.good_bit())
		return -1;

	return 0;
}

template <class REQUEST>
bool RequestHandler<REQUEST>::expired_request(ACE_CDR::ULong trans_id)
{
	void* TODO;
	return false;
}
