///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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

#ifndef OOSERVER_ROOT_HTTP_H_INCLUDED_
#define OOSERVER_ROOT_HTTP_H_INCLUDED_

#include "./OOServer_Root.h"

namespace Root
{
	class Manager;
	class HttpAcceptor;

	class HttpConnection : public ACE_Service_Handler
	{
	public:
		HttpConnection(HttpAcceptor* pAcceptor = 0);

		void open(ACE_HANDLE new_handle, ACE_Message_Block& message_block);
		void addresses(const ACE_INET_Addr& remote_address, const ACE_INET_Addr& local_address);
		void close();
		void send(const ACE_Message_Block* mb);
		void release();

		ACE_CString            m_strRemoteAddr;
		
	private:
		virtual ~HttpConnection() {}

		HttpAcceptor*          m_pAcceptor;
		ACE_SOCK_Stream        m_stream;
		ACE_Asynch_Read_Stream m_reader;
		ACE_CDR::UShort        m_conn_id;
		ACE_CDR::ULong         m_seq_no;

		ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long> m_refcount;

		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
		bool read(ACE_Message_Block* mb = 0);
		void send_error(int err);
	};

	class HttpAcceptor : public ACE_Asynch_Acceptor<HttpConnection>
	{
	friend class HttpConnection;

	public:
		HttpAcceptor();

		bool open(Manager* pManager);
		void close();
		void send_http(ACE_InputCDR& request);
		void close_http(ACE_InputCDR& request);
		
	private:
		ACE_Thread_Mutex m_lock;
		Manager*         m_pManager;
		ACE_CDR::UShort  m_nNextConnection;

		std::map<ACE_CDR::UShort,HttpConnection*> m_mapConnections;

		HttpConnection* make_handler();
		ACE_CDR::UShort add_connection(HttpConnection* pHC);
		void close_connection(ACE_CDR::UShort conn_id);
		bool sendrecv_sandbox(const ACE_OutputCDR& request);
	};
}

#endif // OOSERVER_ROOT_HTTP_H_INCLUDED_
