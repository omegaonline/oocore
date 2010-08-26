///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "Protocol.h"

namespace
{
	class TcpAcceptor : 
		public OOSvrBase::Acceptor<OOSvrBase::AsyncSocket>,
		public Root::Manager::ControlledObject
	{
	public:
		static TcpAcceptor* create(Root::Manager* pManager, Omega::uint32_t id, const std::string& strAddress, const std::string& strPort, int* perr);

		virtual ~TcpAcceptor() {}

	private:
		TcpAcceptor(const TcpAcceptor&);
		TcpAcceptor& operator = (const TcpAcceptor&);

		TcpAcceptor(Root::Manager* pManager, Omega::uint32_t id);

		Root::Manager* const             m_pManager;
		const Omega::uint32_t            m_id;
		OOBase::SmartPtr<OOBase::Socket> m_ptrSocket;

		bool on_accept(OOSvrBase::AsyncSocket* pSocket, const std::string& strAddress, int err);
	};

	class AsyncSocket : 
			public OOSvrBase::IOHandler,
			public Root::Manager::Socket
	{
		friend class TcpAcceptor;

	public:
		virtual ~AsyncSocket();

	private:
		AsyncSocket(Root::Manager* pManager, OOSvrBase::AsyncSocket* pSocket);

		AsyncSocket(const AsyncSocket&);
		AsyncSocket& operator = (const AsyncSocket&);

		int recv(Omega::uint32_t lenBytes, Omega::bool_t bRecvAll);
		int send(OOBase::Buffer* buffer, Omega::bool_t bReliable);

		void on_recv(OOSvrBase::AsyncSocket* pSocket, OOBase::Buffer* buffer, int err);
		void on_sent(OOSvrBase::AsyncSocket* pSocket, OOBase::Buffer* buffer, int err);
		void on_closed(OOSvrBase::AsyncSocket* pSocket);

		Root::Manager* const                     m_pManager;
		Omega::uint32_t                          m_id;
		OOBase::SmartPtr<OOSvrBase::AsyncSocket> m_ptrSocket;
	};
}

void Root::Manager::services_start(Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	size_t mark = response.buffer()->mark_wr_ptr();
	size_t count = 0;
	response.write(count);

	if (channel_id != m_sandbox_channel)
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
	else
	{
		// Now loop through the installed services, telling the sandbox to create them
		Omega::int64_t uKey = 0;
		int err = m_registry->open_key(0,uKey,"System/Services",0);
		if (err != 0)
		{
			if (err != ENOENT)
				LOG_ERROR(("Failed to open /System/Services key: %d",err));
		}
		else
		{
			std::set<std::string> setSubKeys;
			err = m_registry->enum_subkeys(uKey,0,setSubKeys);
			if (err != 0)
				LOG_ERROR(("Failed to enum subkeys of /System/Services key: %d",err));
			else
			{
				count = setSubKeys.size();
				
				for (std::set<std::string>::const_iterator i=setSubKeys.begin();i!=setSubKeys.end();++i)
				{
					// Open the subkey
					Omega::int64_t uSubKey = 0;
					err = m_registry->open_key(uKey,uSubKey,*i,0);
					if (err != 0)
					{
						--count;
						LOG_ERROR(("Failed to open /System/Services/%s: %d",i->c_str(),err));
						continue;
					}

					std::string strOid;
					err = m_registry->get_value(uSubKey,"OID",0,strOid);
					if (err != 0)
					{
						--count;
						LOG_ERROR(("Failed to get /System/Services/%s value OID: %d",i->c_str(),err));
						continue;
					}

					if (!response.write(*i) ||
						!response.write(strOid))
					{
						LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error()).c_str()));
						count = 0;
						break;
					}
				}
			}
		}
	}

	response.replace(count,mark);
}

void Root::Manager::get_service_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::int32_t err = 0;
	if (channel_id != m_sandbox_channel)
	{
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
		err = EACCES;
	}
	else
	{
		std::string strKey;
		if (!request.read(strKey))
		{
			LOG_ERROR(("get_service_key called with invalid key name"));
			err = response.last_error();
		}
		else
		{
			strKey = "System/Services/" + strKey;

			Omega::int64_t uKey = 0;
			err = m_registry->open_key(0,uKey,strKey,0);
			if (err != 0)
				LOG_ERROR(("Failed to open %s: %d",strKey.c_str(),err));			
			else
			{
				if (!response.write(err) ||
					!response.write(uKey) ||
					!response.write(strKey))
				{
					err = response.last_error();
					LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(err).c_str()));
					response.reset();
				}
			}
		}
	}

	if (err != 0)
		response.write(err);
}

void Root::Manager::listen_socket(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::int32_t err = 0;
	if (channel_id != m_sandbox_channel)
	{
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
		err = EACCES;
	}
	else
	{
		std::string strKey;
		Omega::uint32_t id = 0;
		if (!request.read(strKey) ||
			!request.read(id))
		{
			LOG_ERROR(("Failed to read request"));
			err = response.last_error();
		}
		else if (id == 0)
		{
			LOG_ERROR(("listen_socket called with invalid id"));
			err = EINVAL;
		}
		else
		{
			strKey = "System/Services/" + strKey;

			Omega::int64_t uKey = 0;
			err = m_registry->open_key(0,uKey,strKey,0);
			if (err != 0)
				LOG_ERROR(("Failed to open %s: %d",strKey.c_str(),err));
			else
			{
				// Now we want to get the networking key
				err = m_registry->open_key(uKey,uKey,"Network",0);
				if (err != 0)
					LOG_ERROR(("Failed to open %s/Network: %d",strKey.c_str(),err));
				else
				{
					std::string strProtocol;
					err = m_registry->get_value(uKey,"Protocol",0,strProtocol);
					if (err != 0)
						LOG_ERROR(("Failed to open %s/Network: %d",strKey.c_str(),err));
					else if (strProtocol.empty())
					{
						LOG_ERROR(("%s/Network Protocol value is empty",strKey.c_str()));
						err = EINVAL;
					}
					else
					{
						// These are allowed to be missing...
						std::string strAddress, strPort;
						m_registry->get_value(uKey,"Address",0,strAddress);
						m_registry->get_value(uKey,"Port",0,strPort);

						err = create_service_listener(id,strProtocol,strAddress,strPort);
					}
				}
			}
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error()).c_str()));
}

int Root::Manager::create_service_listener(Omega::uint32_t id, const std::string& strProtocol, const std::string& strAddress, const std::string& strPort)
{
	int err = 0;
	OOBase::SmartPtr<ControlledObject> ptrService;

	// This is where we have a list of supported protocols...
	if (strProtocol == "tcp")
		ptrService = TcpAcceptor::create(this,id,strAddress,strPort,&err);
	
	/*else if (strProtocol == "udp")
	{
	}*/
	else
	{
		err = EINVAL;
		LOG_ERROR(("Unsupported service protocol: %s",strProtocol.c_str()));
	}

	if (ptrService)
	{
		try
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			std::pair<std::map<Omega::uint32_t,OOBase::SmartPtr<ControlledObject> >::iterator,bool> p = m_mapListeners.insert(std::map<Omega::uint32_t,OOBase::SmartPtr<ControlledObject> >::value_type(id,ptrService));
			if (!p.second)
			{
				LOG_ERROR(("Duplicate service id: %d",id));
				err = EINVAL;
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("Exception thrown: %s",e.what()));
			err = ENOMEM;
		}
	}
	
	return err;
}

void Root::Manager::stop_services()
{
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		std::map<Omega::uint32_t,OOBase::SmartPtr<ControlledObject> > mapObjects(m_mapListeners);
		m_mapListeners.clear();

		guard.release();

		mapObjects.clear();

		guard.acquire();

		std::map<Omega::uint32_t,OOBase::SmartPtr<Socket> > mapSockets(m_mapSockets);
		m_mapSockets.clear();

		guard.release();

		mapSockets.clear();
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("Exception thrown: %s",e.what()));
	}
}

Omega::uint32_t Root::Manager::add_socket(Omega::uint32_t acceptor_id, Socket* pSocket)
{
	Omega::uint32_t id = 0;
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		while (!id || m_mapSockets.find(id) != m_mapSockets.end())
			id = m_uNextSocketId++;
		
		m_mapSockets.insert(std::map<Omega::uint32_t,OOBase::SmartPtr<Socket> >::value_type(id,pSocket));
	}
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("Exception thrown: %s",e.what()),0);
	}

	// Send message on to sandbox
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketAccept));
	request.write(acceptor_id);
	request.write(id);
	if (request.last_error() != 0)
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()),0);
	}

	OOBase::SmartPtr<OOBase::CDRStream> response;
	sendrecv_sandbox(request,response,0,0);
	if (!response)
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()),0);
	}

	Omega::int32_t err = 0;
	if (!response->read(err))
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to read response data: %s",OOBase::system_error_text(response->last_error()).c_str()),0);
	}
	else if (err != 0)
	{
		remove_socket(id);
		LOG_DEBUG(("Sandbox accept failed: %s",OOBase::system_error_text(err).c_str()));
		return 0;
	}

	return id;
}

void Root::Manager::remove_socket(Omega::uint32_t id)
{
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapSockets.erase(id);
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("Exception thrown: %s",e.what()));
	}
}

void Root::Manager::remove_listener(Omega::uint32_t id)
{
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapListeners.erase(id);
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("Exception thrown: %s",e.what()));
	}
}

void Root::Manager::socket_recv(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::int32_t err = 0;
	if (channel_id != m_sandbox_channel)
	{
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
		err = EACCES;
	}
	else
	{
		Omega::uint32_t id = 0;
		Omega::uint32_t lenBytes = 0;
		Omega::bool_t   bRecvAll = false;

		if (!request.read(id) ||
			!request.read(lenBytes) ||
			!request.read(bRecvAll))
		{
			LOG_ERROR(("Failed to read request"));
			err = response.last_error();
		}
		else if (id == 0)
		{
			LOG_ERROR(("socket_recv called with invalid id"));
			err = EINVAL;
		}
		else
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			try
			{
				std::map<Omega::uint32_t,OOBase::SmartPtr<Socket> >::iterator i=m_mapSockets.find(id);
				if (i == m_mapSockets.end())
					err = EINVAL;
				else
					err = i->second->recv(lenBytes,bRecvAll);			
			}
			catch (std::exception& e)
			{
				LOG_ERROR(("Exception thrown: %s",e.what()));
				err = EINVAL;
			}
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error()).c_str()));
}

void Root::Manager::socket_send(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::int32_t err = 0;
	if (channel_id != m_sandbox_channel)
	{
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
		err = EACCES;
	}
	else
	{
		Omega::uint32_t id = 0;
		Omega::bool_t bReliable = true;
		if (!request.read(id) ||
			!request.read(bReliable))
		{
			LOG_ERROR(("Failed to read request"));
			err = response.last_error();
		}
		else if (id == 0)
		{
			LOG_ERROR(("socket_recv called with invalid id"));
			err = EINVAL;
		}
		else if (request.buffer()->length())
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			try
			{
				std::map<Omega::uint32_t,OOBase::SmartPtr<Socket> >::iterator i=m_mapSockets.find(id);
				if (i == m_mapSockets.end())
					err = EINVAL;
				else
					err = i->second->send(request.buffer(),bReliable);
			}
			catch (std::exception& e)
			{
				LOG_ERROR(("Exception thrown: %s",e.what()));
				err = EINVAL;
			}
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error()).c_str()));
}

void Root::Manager::socket_close(Omega::uint32_t channel_id, OOBase::CDRStream& request)
{
	Omega::int32_t err = 0;
	if (channel_id != m_sandbox_channel)
	{
		LOG_ERROR(("Service opcode sent by non-sandbox process"));
		err = EACCES;
	}
	else
	{
		Omega::uint32_t id = 0;
		if (!request.read(id))
		{
			LOG_ERROR(("Failed to read request"));
		}
		else
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			try
			{
				m_mapSockets.erase(id);
			}
			catch (std::exception& e)
			{
				LOG_ERROR(("Exception thrown: %s",e.what()));
			}
		}
	}
}

TcpAcceptor::TcpAcceptor(Root::Manager* pManager, Omega::uint32_t id) :
		m_pManager(pManager),
		m_id(id)
{
	assert(m_pManager);
	assert(m_id);
}

TcpAcceptor* TcpAcceptor::create(Root::Manager* pManager, Omega::uint32_t id, const std::string& strAddress, const std::string& strPort, int* perr)
{
	TcpAcceptor* pService = 0;
	OOBASE_NEW(pService,TcpAcceptor(pManager,id));
	if (!pService)
	{
		*perr = ENOMEM;
		LOG_ERROR_RETURN(("Out of memory"),(TcpAcceptor*)0);
	}

	pService->m_ptrSocket = Root::Proactor::instance()->accept_remote(pService,strAddress,strPort,perr);
	if (*perr != 0)
	{
		delete pService;
		LOG_ERROR_RETURN(("accept_remote failed: %s",OOBase::system_error_text(*perr).c_str()),(TcpAcceptor*)0);
	}

	OOSvrBase::Logger::log(OOSvrBase::Logger::Debug,"Listening on %s:%s",strAddress.empty() ? "localhost" : strAddress.c_str(),strPort.c_str());

	*perr = 0;
	return pService;
}

bool TcpAcceptor::on_accept(OOSvrBase::AsyncSocket* pSocket, const std::string& /*strAddress*/, int err)
{
	if (err)
	{
		LOG_ERROR(("on_accept failed: %s",OOBase::system_error_text(err).c_str()));
		m_pManager->remove_listener(m_id);
		return false;
	}

	AsyncSocket* pAsyncSocket = 0;
	OOBASE_NEW(pAsyncSocket,AsyncSocket(m_pManager,pSocket));
	if (!pAsyncSocket)
	{
		delete pSocket;
		LOG_ERROR_RETURN(("Out of memory"),true);
	}

	pAsyncSocket->m_id = m_pManager->add_socket(m_id,pAsyncSocket);
	if (!pAsyncSocket->m_id)
	{
		delete pAsyncSocket;
		m_pManager->remove_listener(m_id);
		return false;
	}
	
	return true;
}

AsyncSocket::AsyncSocket(Root::Manager* pManager, OOSvrBase::AsyncSocket* pSocket) :
		m_pManager(pManager),
		m_id(0),
		m_ptrSocket(pSocket)
{
	m_ptrSocket->bind_handler(this);
}

AsyncSocket::~AsyncSocket()
{
	if (m_id)
		m_pManager->remove_socket(m_id);
}

int AsyncSocket::recv(Omega::uint32_t lenBytes, Omega::bool_t bRecvAll)
{
	// We know we are going to pass this buffer along, so we preallocate the header we use later,
	// and read the data behind it...

	OOBase::Buffer* buffer = 0;
	OOBASE_NEW(buffer,OOBase::Buffer(12 + lenBytes,OOBase::CDRStream::MaxAlignment));
	if (!buffer)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Move the ptr forwards
	buffer->rd_ptr(12);
	buffer->wr_ptr(12);
	
	int err = m_ptrSocket->async_recv(buffer,bRecvAll ? lenBytes : 0);
	
	buffer->release();
	
	if (err != 0)
		LOG_ERROR(("async_recv failed: %s",OOBase::system_error_text(err).c_str()));
	
	return err;
}

void AsyncSocket::on_recv(OOSvrBase::AsyncSocket* /*pSocket*/, OOBase::Buffer* buffer, int err)
{
	if (buffer && buffer->length() > 12)
	{
		// If we have data, then skip the wr_ptr back to the beginning
		size_t mark = buffer->mark_wr_ptr();

		buffer->mark_wr_ptr(0);
		buffer->mark_rd_ptr(0);
	
		// Send message on to sandbox
		OOBase::CDRStream request(buffer);
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketRecv));
		request.write(m_id);
		request.write(Omega::int32_t(err));
		if (request.last_error() != 0)
		{
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
			return;
		}

		// Move the mark back to where it was
		buffer->mark_wr_ptr(mark);

		// Just forward on...
		OOBase::SmartPtr<OOBase::CDRStream> response;
		m_pManager->sendrecv_sandbox(request,response,0,1);
	}
	else if (err)
	{
		// Send message on to sandbox
		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketRecv));
		request.write(m_id);
		request.write(Omega::int32_t(err));
		if (request.last_error() != 0)
		{
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
			return;
		}

		// Just forward on...
		OOBase::SmartPtr<OOBase::CDRStream> response;
		m_pManager->sendrecv_sandbox(request,response,0,1);
	}	
}

int AsyncSocket::send(OOBase::Buffer* buffer, Omega::bool_t /*bReliable*/)
{
	int err = m_ptrSocket->async_send(buffer);
	if (err != 0)
		LOG_ERROR(("async_send failed: %s",OOBase::system_error_text(err).c_str()));
	
	return err;
}

void AsyncSocket::on_sent(OOSvrBase::AsyncSocket* /*pSocket*/, OOBase::Buffer* buffer, int err)
{
	if (buffer)
	{
		// If we have data, then skip the wr_ptr back to the beginning
		size_t mark = buffer->mark_wr_ptr();
		buffer->mark_wr_ptr(0);
		buffer->mark_rd_ptr(0);
	
		// Send message on to sandbox
		OOBase::CDRStream request(buffer);
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketSent));
		request.write(m_id);
		request.write(Omega::int32_t(err));
		if (request.last_error() != 0)
		{
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
			return;
		}

		// Move the mark back to where it was
		buffer->mark_wr_ptr(mark);

		// Just forward on...
		OOBase::SmartPtr<OOBase::CDRStream> response;
		m_pManager->sendrecv_sandbox(request,response,0,1);
	}
	else
	{
		// Send message on to sandbox
		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketRecv));
		request.write(m_id);
		request.write(Omega::int32_t(err));
		if (request.last_error() != 0)
		{
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
			return;
		}

		// Just forward on...
		OOBase::SmartPtr<OOBase::CDRStream> response;
		m_pManager->sendrecv_sandbox(request,response,0,1);
	}
}

void AsyncSocket::on_closed(OOSvrBase::AsyncSocket* /*pSocket*/)
{
	// Send message on to sandbox
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketClose));
	request.write(m_id);
	if (request.last_error() != 0)
		LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
	else
	{
		OOBase::SmartPtr<OOBase::CDRStream> response;
		m_pManager->sendrecv_sandbox(request,response,0,1);
	}
}

