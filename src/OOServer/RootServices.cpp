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
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "Protocol.h"

#if defined(_WIN32)
#include <Ws2tcpip.h>
#else
#include <netdb.h>
#endif

namespace
{
	class TcpAcceptor :
		public Root::Manager::ControlledObject
	{
	public:
		static TcpAcceptor* create(Root::Manager* pManager, Omega::uint32_t id, const OOBase::LocalString& strAddress, const OOBase::LocalString& strPort, int& err);

		virtual ~TcpAcceptor() {}

	private:
		TcpAcceptor(const TcpAcceptor&);
		TcpAcceptor& operator = (const TcpAcceptor&);

		TcpAcceptor(Root::Manager* pManager, Omega::uint32_t id);

		Root::Manager* const                m_pManager;
		const Omega::uint32_t               m_id;
		OOBase::RefPtr<OOSvrBase::Acceptor> m_ptrAcceptor;

		static void on_accept(void* pThis, OOSvrBase::AsyncSocket* pSocket, const sockaddr* addr, socklen_t addr_len, int err);
		void on_accept_i(OOSvrBase::AsyncSocket* pSocket, int err);
	};

	class AsyncSocket :
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

		void on_recv(OOBase::Buffer* buffer, int err);
		void on_sent(int err);
		void on_closed();

		Root::Manager* const                   m_pManager;
		Omega::uint32_t                        m_id;
		OOBase::RefPtr<OOSvrBase::AsyncSocket> m_ptrSocket;
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
			Db::Hive::registry_set_t setSubKeys;
			if ((err = m_registry->enum_subkeys(uKey,0,setSubKeys)) != 0)
				LOG_ERROR(("Failed to enum subkeys of /System/Services key: %d",err));
			else
			{
				for (OOBase::String i;setSubKeys.pop(&i);)
				{
					// Open the subkey
					Omega::int64_t uSubKey = 0;
					if ((err = m_registry->open_key(uKey,uSubKey,i.c_str(),0)) != 0)
					{
						--count;
						LOG_ERROR(("Failed to open /System/Services/%s: %d",i.c_str(),err));
						continue;
					}

					OOBase::LocalString strOid;
					if ((err = m_registry->get_value(uSubKey,"OID",0,strOid)) != 0)
					{
						--count;
						LOG_ERROR(("Failed to get /System/Services/%s value OID: %d",i.c_str(),err));
						continue;
					}

					if (!response.write(i.c_str()) ||
						!response.write(strOid.c_str()))
					{
						LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
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
		OOBase::LocalString strKeyEnd;
		if (!request.read(strKeyEnd))
		{
			LOG_ERROR(("get_service_key called with invalid key name"));
			err = response.last_error();
		}
		else
		{
			OOBase::LocalString strKey;
			if ((err = strKey.concat("System/Services/",strKeyEnd.c_str())) != 0)
				LOG_ERROR(("Failed to construct string: %s",OOBase::system_error_text(err)));
			
			if (!strKey.empty())
			{
				Omega::int64_t uKey = 0;
				if ((err = m_registry->open_key(0,uKey,strKey.c_str(),0)) != 0)
					LOG_ERROR(("Failed to open %s: %d",strKey.c_str(),err));
				else
				{
					if (!response.write(err) ||
						!response.write(uKey) ||
						!response.write(strKey.c_str()))
					{
						err = response.last_error();
						LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(err)));
						response.reset();
					}
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
		OOBase::LocalString strKeyEnd;
		Omega::uint32_t id = 0;
		if (!request.read(strKeyEnd) ||
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
			OOBase::LocalString strKey;
			if ((err = strKey.concat("System/Services/",strKeyEnd.c_str())) != 0)
				LOG_ERROR(("Failed to construct string: %s",OOBase::system_error_text(err)));

			if (!strKey.empty())
			{
				Omega::int64_t uKey = 0;
				if ((err = m_registry->open_key(0,uKey,strKey.c_str(),0)) != 0)
					LOG_ERROR(("Failed to open %s: %d",strKey.c_str(),err));
				else
				{
					// Now we want to get the networking key
					if ((err = m_registry->open_key(uKey,uKey,"Network",0)) != 0)
						LOG_ERROR(("Failed to open %s/Network: %d",strKey.c_str(),err));
					else
					{
						OOBase::LocalString strProtocol;
						if ((err = m_registry->get_value(uKey,"Protocol",0,strProtocol)) != 0)
							LOG_ERROR(("Failed to open %s/Network: %d",strKey.c_str(),err));
						else if (strProtocol.empty())
						{
							LOG_ERROR(("%s/Network Protocol value is empty",strKey.c_str()));
							err = EINVAL;
						}
						else
						{
							// These are allowed to be missing...
							OOBase::LocalString strAddress, strPort;
							m_registry->get_value(uKey,"Address",0,strAddress);
							m_registry->get_value(uKey,"Port",0,strPort);

							err = create_service_listener(id,strProtocol,strAddress,strPort);
						}
					}
				}
			}
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

int Root::Manager::create_service_listener(Omega::uint32_t id, const OOBase::LocalString& strProtocol, const OOBase::LocalString& strAddress, const OOBase::LocalString& strPort)
{
	int err = 0;
	OOBase::SmartPtr<ControlledObject> ptrService;

	// This is where we have a list of supported protocols...
	if (strProtocol == "tcp")
		ptrService = TcpAcceptor::create(this,id,strAddress,strPort,err);

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
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		err = m_mapListeners.insert(id,ptrService);
	}

	return err;
}

void Root::Manager::stop_services()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	OOBase::SmartPtr<ControlledObject> ptrObj;
	while (m_mapListeners.pop(NULL,&ptrObj))
	{
		guard.release();

		ptrObj = NULL;

		guard.acquire();
	}

	OOBase::SmartPtr<Socket> ptrSock;
	while (m_mapSockets.pop(NULL,&ptrSock))
	{
		guard.release();

		ptrSock = NULL;

		guard.acquire();
	}
}

Omega::uint32_t Root::Manager::add_socket(Omega::uint32_t acceptor_id, Socket* pSocket)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Omega::uint32_t id = 0;
	Omega::int32_t err = m_mapSockets.insert(OOBase::SmartPtr<Socket>(pSocket),id,1,0xFFFFFFFF);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to add socket to table: %s",OOBase::system_error_text(err)),0);

	guard.release();
	
	// Send message on to sandbox
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketAccept));
	request.write(acceptor_id);
	request.write(id);
	if (request.last_error() != 0)
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())),0);
	}

	OOBase::CDRStream response;
	if (sendrecv_sandbox(request,&response,0) != io_result::success)
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to send request to sandbox"),0);
	}

	if (!response.read(err))
	{
		remove_socket(id);
		LOG_ERROR_RETURN(("Failed to read response data: %s",OOBase::system_error_text(response.last_error())),0);
	}
	else if (err != 0)
	{
		remove_socket(id);
		LOG_DEBUG(("Sandbox accept failed: %s",OOBase::system_error_text(err)));
		return 0;
	}

	return id;
}

void Root::Manager::remove_socket(Omega::uint32_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapSockets.remove(id);
}

void Root::Manager::remove_listener(Omega::uint32_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapListeners.remove(id);
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

			OOBase::SmartPtr<Socket> ptrSock;
			if (!m_mapSockets.find(id,ptrSock))
				err = EINVAL;
			else
				err = ptrSock->recv(lenBytes,bRecvAll);
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
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

			OOBase::SmartPtr<Socket> ptrSock;
			if (!m_mapSockets.find(id,ptrSock))
				err = EINVAL;
			else
				err = ptrSock->send(request.buffer(),bReliable);
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
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
			remove_socket(id);
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

TcpAcceptor* TcpAcceptor::create(Root::Manager* pManager, Omega::uint32_t id, const OOBase::LocalString& strAddress, const OOBase::LocalString& strPort, int& err)
{
	err = 0;

	// Resolve the passed in addresses...
	addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* pResults = NULL;
	if (getaddrinfo(strAddress.c_str(),strPort.c_str(),&hints,&pResults) != 0)
	{
#if defined(_WIN32)
		err = WSAGetLastError();
#else
		err = errno;
#endif
		return NULL;
	}

	sockaddr_storage addr = {0};
	socklen_t addr_len = (socklen_t)pResults->ai_addrlen;
	memcpy(&addr,pResults->ai_addr,addr_len);
	
	// Done with address info
	freeaddrinfo(pResults);

	TcpAcceptor* pService = new (std::nothrow) TcpAcceptor(pManager,id);
	if (!pService)
	{
		err = ENOMEM;
		LOG_ERROR_RETURN(("Out of memory"),(TcpAcceptor*)NULL);
	}

	pService->m_ptrAcceptor = Root::Proactor::instance().accept_remote(pService,&on_accept,(sockaddr*)&addr,addr_len,err);
	if (err != 0)
	{
		delete pService;
		LOG_ERROR_RETURN(("accept_remote failed: %s",OOBase::system_error_text(err)),(TcpAcceptor*)NULL);
	}

	OOBase::Logger::log(OOBase::Logger::Debug,"Listening on %s:%s",strAddress.empty() ? "localhost" : strAddress.c_str(),strPort.c_str());

	return pService;
}

void TcpAcceptor::on_accept(void* pThis, OOSvrBase::AsyncSocket* pSocket, const sockaddr* /*addr*/, socklen_t /*addr_len*/, int err)
{
	static_cast<TcpAcceptor*>(pThis)->on_accept_i(pSocket,err);
}

void TcpAcceptor::on_accept_i(OOSvrBase::AsyncSocket* pSocket, int err)
{
	if (err)
	{
		LOG_ERROR(("on_accept failed: %s",OOBase::system_error_text(err)));
		m_pManager->remove_listener(m_id);
		return;
	}

	AsyncSocket* pAsyncSocket = new (std::nothrow) AsyncSocket(m_pManager,pSocket);
	if (!pAsyncSocket)
	{
		LOG_ERROR(("Out of memory"));
		return;
	}
	
	pAsyncSocket->m_id = m_pManager->add_socket(m_id,pAsyncSocket);
	if (!pAsyncSocket->m_id)
	{
		delete pAsyncSocket;
		m_pManager->remove_listener(m_id);
	}
}

AsyncSocket::AsyncSocket(Root::Manager* pManager, OOSvrBase::AsyncSocket* pSocket) :
		m_pManager(pManager),
		m_id(0),
		m_ptrSocket(pSocket)
{
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

	OOBase::RefPtr<OOBase::Buffer> buffer = new (std::nothrow) OOBase::Buffer(12 + lenBytes,OOBase::CDRStream::MaxAlignment);
	if (!buffer)
		LOG_ERROR_RETURN(("Out of memory"),false);

	// Move the ptr forwards
	buffer->rd_ptr(12);
	buffer->wr_ptr(12);

	int err = m_ptrSocket->recv(this,&AsyncSocket::on_recv,buffer,bRecvAll ? lenBytes : 0);
	if (err != 0)
		LOG_ERROR(("recv failed: %s",OOBase::system_error_text(err)));

	return err;
}

void AsyncSocket::on_recv(OOBase::Buffer* buffer, int err)
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
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
			return;
		}

		// Move the mark back to where it was
		buffer->mark_wr_ptr(mark);

		// Just forward on...
		m_pManager->sendrecv_sandbox(request,NULL,1);
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
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
			return;
		}

		// Just forward on...
		m_pManager->sendrecv_sandbox(request,NULL,1);
	}
}

int AsyncSocket::send(OOBase::Buffer* buffer, Omega::bool_t /*bReliable*/)
{
	int err = m_ptrSocket->send(this,&AsyncSocket::on_sent,buffer);
	if (err != 0)
		LOG_ERROR(("async_send failed: %s",OOBase::system_error_text(err)));

	return err;
}

void AsyncSocket::on_sent(int /*err*/)
{
	/*if (buffer)
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
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
			return;
		}

		// Move the mark back to where it was
		buffer->mark_wr_ptr(mark);

		// Just forward on...
		m_pManager->sendrecv_sandbox(request,NULL,1);
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
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
			return;
		}

		// Just forward on...
		m_pManager->sendrecv_sandbox(request,NULL,1);
	}*/
}

void AsyncSocket::on_closed()
{
	// Send message on to sandbox
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OnSocketClose));
	request.write(m_id);
	if (request.last_error() != 0)
		LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
	else
		m_pManager->sendrecv_sandbox(request,NULL,1);
}

