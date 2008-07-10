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

#include "./OOServer_Root.h"
#include "./RootHttp.h"
#include "./RootManager.h"
#include "./Protocol.h"

Root::HttpConnection::HttpConnection(HttpAcceptor* pAcceptor) :
	m_pAcceptor(pAcceptor),
	m_conn_id(0),
	m_seq_no(0),
	m_refcount(0)
{
}

void Root::HttpConnection::release()
{
	if (--m_refcount == 0)
		delete this;
}

void Root::HttpConnection::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	m_stream = ACE_SOCK_Stream(new_handle);

	if (m_reader.open(*this,new_handle) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("reader.open() failed")));
		delete this;
		return;
	}

	++m_refcount;
	m_conn_id = m_pAcceptor->add_connection(this);
	if (!m_conn_id)
		release();
	else if (!read())
		m_pAcceptor->close_connection(m_conn_id);
}

void Root::HttpConnection::addresses(const ACE_INET_Addr& remote_address, const ACE_INET_Addr&)
{
	// Get the addresses
	ACE_TCHAR szBuf[1024];
	remote_address.addr_to_string(szBuf,1024,0);
	szBuf[1023] = '\0';
	m_strRemoteAddr = ACE_TEXT_ALWAYS_CHAR(szBuf);
}

void Root::HttpConnection::close()
{
	m_stream.close();
}

bool Root::HttpConnection::read(ACE_Message_Block* mb)
{
	if (!mb)
		ACE_NEW_RETURN(mb,ACE_Message_Block(ACE_OS::getpagesize()),false);
	else
	{
		mb = mb->duplicate();
		mb->reset();
	}

	// Start an async read
	++m_refcount;
	if (m_reader.read(*mb,mb->size()) != 0)
	{
		mb->release();
		release();
		return false;
	}

	return true;
}

void Root::HttpConnection::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	if (result.success() == 0)
		send_error(result.error());
	else
	{
		ACE_OutputCDR request;
		request << static_cast<Root::RootOpCode_t>(Root::HttpRecv);
		request.write_ushort(static_cast<ACE_CDR::UShort>(m_conn_id));
		request << (int)0;
		request.write_ulong(m_seq_no++);

		request.write_octet_array_mb(&mb);

		if (!request.good_bit() || !m_pAcceptor->sendrecv_sandbox(request))
			m_pAcceptor->close_connection(m_conn_id);
		else if (!read(&mb))
			send_error(ACE_OS::last_error());
	}

	mb.release();
	release();
}

void Root::HttpConnection::send_error(int err)
{
	if (err == 0)
		err = -1;

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::HttpRecv);
	request.write_ushort(static_cast<ACE_CDR::UShort>(m_conn_id));
	request << err;

	if (request.good_bit())
		m_pAcceptor->sendrecv_sandbox(request);

	m_pAcceptor->close_connection(m_conn_id);
}

void Root::HttpConnection::send(const ACE_Message_Block* mb)
{
	m_stream.send_n(mb);
}

Root::HttpAcceptor::HttpAcceptor() :
	ACE_Asynch_Acceptor<HttpConnection>(),
	m_pManager(0),
	m_nNextConnection(0)
{
}

bool Root::HttpAcceptor::open(Manager* pManager)
{
	// Get the local machine registry
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> reg_root = Manager::get_registry();

	// Get the port
	ACE_CString strAddr = "8901";
	ACE_INT64 key = 0;
	if (reg_root->open_key(key,"Server\\http",0) == 0)
		reg_root->get_string_value(key,"LocalAddress",0,strAddr);

	// Compose the address
	ACE_INET_Addr addr;
	if (addr.set(strAddr.c_str(),AF_INET) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("failed to resolve address")),false);

	// Stash the manager
	m_pManager = pManager;

	// Open the async connector
	if (ACE_Asynch_Acceptor<HttpConnection>::open(addr,0,1) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("failed to open socket")),false);

	return true;
}

void Root::HttpAcceptor::close()
{
	// Stop accepting anything more
	cancel();

	try
	{
		// Scope the lock
		bool bOk = false;
		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

			for (std::map<ACE_CDR::UShort,HttpConnection*>::iterator i=m_mapConnections.begin();i!=m_mapConnections.end();++i)
			{
				i->second->close();
			}

			if (m_mapConnections.empty())
				bOk = true;
		}

		// Spin till everyone is gone
		while (!bOk)
		{
			ACE_OS::sleep(ACE_Time_Value(0,1000));

			{
				ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

				bOk = m_mapConnections.empty();
			}
		}
	}
	catch (std::exception&)
	{
	}
}

Root::HttpConnection* Root::HttpAcceptor::make_handler()
{
	HttpConnection* handler = 0;
	ACE_NEW_RETURN(handler,HttpConnection(this),0);
	return handler;
}

ACE_CDR::UShort Root::HttpAcceptor::add_connection(HttpConnection* pHC)
{
	ACE_CDR::UShort conn_id = 0;
	try
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,0);

		conn_id = ++m_nNextConnection;
		while (!conn_id && m_mapConnections.find(conn_id) != m_mapConnections.end())
			conn_id = ++m_nNextConnection;

		// pHC is already AddRef'ed
		m_mapConnections.insert(std::map<ACE_CDR::UShort,HttpConnection*>::value_type(conn_id,pHC));
	}
	catch (std::exception& e)
	{
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: std::exception thrown %C\n",e.what()),0);
	}

	// Send the open message
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::HttpOpen);
	request.write_ushort(conn_id);
	request.write_string(pHC->m_strRemoteAddr);
	request.write_string("http://");

	bool bOk = false;
	if (request.good_bit())
	{
		ACE_InputCDR* response = 0;
		if (m_pManager->sendrecv_sandbox(request,0,response))
		{
			int err = 0;
			(*response) >> err;

			if (response->good_bit() && err == 0)
				bOk = true;

			delete response;
		}
	}

	if (!bOk)
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,0);

		m_mapConnections.erase(conn_id);

		return 0;
	}

	return conn_id;
}

void Root::HttpAcceptor::close_connection(ACE_CDR::UShort conn_id)
{
	try
	{
		ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

		std::map<ACE_CDR::UShort,HttpConnection*>::iterator i=m_mapConnections.find(conn_id);
		if (i != m_mapConnections.end())
		{
			i->second->close();
			i->second->release();
			m_mapConnections.erase(i);
		}
	}
	catch (std::exception& e)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
	}
}

void Root::HttpAcceptor::send_http(ACE_InputCDR& request)
{
	ACE_CDR::UShort conn_id;
	request.read_ushort(conn_id);

	if (request.good_bit())
	{
		try
		{
			ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

			std::map<ACE_CDR::UShort,HttpConnection*>::iterator i=m_mapConnections.find(conn_id);
			if (i != m_mapConnections.end())
				i->second->send(request.start());
		}
		catch (std::exception& e)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: std::exception thrown %C\n"),e.what()));
		}
	}
}

void Root::HttpAcceptor::close_http(ACE_InputCDR& request)
{
	ACE_CDR::UShort conn_id;
	request.read_ushort(conn_id);

	if (request.good_bit())
		close_connection(conn_id);
}

bool Root::HttpAcceptor::sendrecv_sandbox(const ACE_OutputCDR& request)
{
	ACE_InputCDR* response = 0;
	if (!m_pManager->sendrecv_sandbox(request,1,response))
		return false;

	return true;
}
