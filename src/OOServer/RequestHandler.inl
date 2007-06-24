/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

template <class REQUEST>
int RequestHandler<REQUEST>::start(int threads)
{
	ThreadParams* params;
	ACE_NEW_RETURN(params,ThreadParams,-1);

	params->pThis = this;
	params->deadline = ACE_Time_Value::zero;
	params->refcount = threads;

	// Spawn off the request threads
	m_req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn,params);
	if (m_req_thrd_grp_id == -1)
	{
		delete params;
		return -1;
	}

	return 0;
}

template <class REQUEST>
ACE_THR_FUNC_RETURN RequestHandler<REQUEST>::request_worker_fn(void* param)
{
	ThreadParams* params = static_cast<ThreadParams*>(param);

	RequestHandler* pThis = params->pThis;
	ACE_Time_Value v = params->deadline;
	ACE_Time_Value* deadline = 0;
	if (v != ACE_Time_Value::zero)
		deadline = &v;

	if (--params->refcount == 0)
		delete params;

	++pThis->m_thread_count;

	pThis->pump_requests(deadline);

	--pThis->m_thread_count;

	return 0;
}

template <class REQUEST>
void RequestHandler<REQUEST>::stop()
{
	// Stop the message queue
	m_msg_queue.close();

    // Wait for all the request threads to finish
	if (m_req_thrd_grp_id != -1)
		ACE_Thread_Manager::instance()->wait_grp(m_req_thrd_grp_id);
}

template <class REQUEST>
bool RequestHandler<REQUEST>::enqueue_request(REQUEST* req)
{
	int nInQueue = m_msg_queue.enqueue_prio(req);

	// Thread pooling maybe?
	if (nInQueue > m_thread_count.value()+1)
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_spawn_lock);
		if (guard.locked () != 0 && nInQueue > m_thread_count.value()+1)
		{
			// Spawn an extra thread to take the strain
			ThreadParams* params;
			ACE_NEW_NORETURN(params,ThreadParams);

			if (params)
			{
				params->pThis = this;
				params->deadline = ACE_Time_Value(60);
				params->refcount = 1;

				// Spawn off the request threads
				int ret = ACE_Thread_Manager::instance()->spawn(request_worker_fn,params,THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,0,0,ACE_DEFAULT_THREAD_PRIORITY,m_req_thrd_grp_id);
				if (ret == -1)
				{
					delete params;
				}
			}
		}
	}

	return (nInQueue > 0);
}

template <class REQUEST>
bool RequestHandler<REQUEST>::wait_for_response(ACE_CDR::ULong trans_id, REQUEST*& response, const ACE_Time_Value* deadline)
{
	for (;;)
	{
		// Get the next message
		REQUEST* req;
		int ret = m_msg_queue.dequeue_prio(req,const_cast<ACE_Time_Value*>(deadline));
		if (ret == -1)
			return false;

		// Stash the current rd_ptr()
		char* rd_ptr_start = req->input()->rd_ptr();

		// Clone the input
		ACE_InputCDR input(*req->input());

		// Read and set the byte order
		ACE_CDR::Octet byte_order;
		ACE_CDR::Octet version = 0;
		if (input.read_octet(byte_order) && input.read_octet(version) && version == 1)
		{
			input.reset_byte_order(byte_order);

			// Read the header
			ACE_CDR::ULong msg_len;
			ACE_CDR::ULong request_trans_id;
			ACE_CDR::Boolean bIsRequest = false;
			ACE_CDR::UShort dest_channel_id;
			ACE_CDR::ULong req_dline_secs;
			ACE_CDR::ULong req_dline_usecs;

			input >> msg_len;
			input >> request_trans_id;
			input >> dest_channel_id;
			input.read_boolean(bIsRequest);

			input >> req_dline_secs;
			input >> req_dline_usecs;
			ACE_Time_Value request_deadline(static_cast<time_t>(req_dline_secs), static_cast<suseconds_t>(req_dline_usecs));

			ACE_CDR::UShort src_channel_id = 0;
			if (bIsRequest)
				input >> src_channel_id;

			if (input.good_bit() && request_deadline > ACE_OS::gettimeofday())
			{
				// See if we want to process it...
				if (bIsRequest)
				{
					// Rest of data is aligned on next boundary
					input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

					// Skip the actual input to where we are at
					req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

					// Process the message...
					process_request(req,dest_channel_id,src_channel_id,request_trans_id,request_deadline);

					// process_request() is expected to delete req;
					req = 0;
				}
				else
				{
					if (request_trans_id == trans_id)
					{
						// Its the request we have been waiting for...

						// Rest of data is aligned on next boundary
						input.align_read_ptr(ACE_CDR::MAX_ALIGNMENT);

						// Skip the actual input to where we are at
						req->input()->skip_bytes(static_cast<size_t>(input.rd_ptr() - rd_ptr_start));

						response = req;
						return true;
					}
					else
					{
						// Put the response back in the queue, its not for us...
						ret = m_msg_queue.enqueue_head(req,const_cast<ACE_Time_Value*>(deadline));
						if (ret < 0)
						{
							delete req;
							return false;
						}

						// Don't delete the request
						req = 0;
					}
				}
			}
		}

		// Done with this request
		delete req;
	}
}

template <class REQUEST>
bool RequestHandler<REQUEST>::pump_requests(const ACE_Time_Value* deadline)
{
	REQUEST* response = 0;
	bool bRet = wait_for_response(0,response,deadline);
	if (response)
		delete response;
	return bRet;
}

template <class REQUEST>
bool RequestHandler<REQUEST>::send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, const ACE_Message_Block* mb, REQUEST*& response, const ACE_Time_Value& deadline)
{
	// Generate next transaction id
	long trans = 0;
	while (trans == 0)
	{
		trans = m_next_trans_id++;
	}

	ACE_CDR::ULong trans_id = static_cast<ACE_CDR::ULong>(trans);

	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,src_channel_id,trans_id,header,mb,deadline))
		return false;

	bool bRet = false;
	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait > ACE_Time_Value::zero)
	{
		// Send to the handle
		size_t sent = 0;
		ssize_t res = ACE::send_n(handle,header.begin(),&wait,&sent);
		if (res != -1 && sent == header.total_length())
		{
			// Wait for response...
			bRet = wait_for_response(trans_id,response,&deadline);
		}
	}
	else
	{
		ACE_OS::last_error(ETIMEDOUT);
	}

	return bRet;
}

template <class REQUEST>
bool RequestHandler<REQUEST>::send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	if (!build_header(dest_channel_id,src_channel_id,0,header,mb,deadline))
		return false;

	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return false;

	return true;
}

static bool ACE_OutputCDR_replace(ACE_OutputCDR& stream, char* msg_len_point)
{
#if ACE_MAJOR_VERSION <= 5 && ACE_MINOR_VERSION <= 5 && ACE_BETA_VERSION == 0

    ACE_CDR::Long len = static_cast<ACE_CDR::Long>(stream.total_length());

#if !defined (ACE_ENABLE_SWAP_ON_WRITE)
    *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
#else
    if (!stream.do_byte_swap())
        *reinterpret_cast<ACE_CDR::Long*>(msg_len_point) = len;
    else
        ACE_CDR::swap_4(reinterpret_cast<const char*>(len),msg_len_point);
#endif

    return true;
#else
    return stream.replace(static_cast<ACE_CDR::Long>(stream.total_length()),msg_len_point);
#endif
}

template <class REQUEST>
bool RequestHandler<REQUEST>::build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return false;

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write the common header
	header << trans_id;
	header << dest_channel_id;
	header.write_boolean(true);	// Is request

	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);
	header << src_channel_id;

	if (!header.good_bit())
		return false;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	return true;
}

template <class REQUEST>
bool RequestHandler<REQUEST>::send_response(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline)
{
	// Check the size
	if (mb->total_length() > ACE_INT32_MAX)
	{
		ACE_OS::last_error(E2BIG);
		return false;
	}

	// Write the header info
	ACE_OutputCDR header(40 + ACE_DEFAULT_CDR_MEMCPY_TRADEOFF);
	header.write_octet(static_cast<ACE_CDR::Octet>(header.byte_order()));
	header.write_octet(1);	// version
	if (!header.good_bit())
		return false;

	// Write out the header length and remember where we wrote it
	header.write_ulong(0);
	char* msg_len_point = header.current()->wr_ptr() - ACE_CDR::LONG_SIZE;

	// Write out common header
	header << trans_id;
	header << dest_channel_id;
	header.write_boolean(false);	// Response

	header.write_ulong(static_cast<const timeval*>(deadline)->tv_sec);
	header.write_ulong(static_cast<const timeval*>(deadline)->tv_usec);

	if (!header.good_bit())
		return false;

	// Align the buffer
	header.align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

	// Write the request stream
	header.write_octet_array_mb(mb);
	if (!header.good_bit())
		return false;

	// Update the total length
	if (!ACE_OutputCDR_replace(header,msg_len_point))
		return false;

	ACE_Time_Value wait = deadline - ACE_OS::gettimeofday();
	if (wait <= ACE_Time_Value::zero)
	{
		ACE_OS::last_error(ETIMEDOUT);
		return false;
	}

	// Send to the handle
	size_t sent = 0;
	ssize_t res = ACE::send_n(handle,header.begin(),&wait,&sent);
	if (res == -1 || sent < header.total_length())
		return false;

	return true;
}
