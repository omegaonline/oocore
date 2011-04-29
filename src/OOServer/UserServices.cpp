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

#include "OOServer_User.h"
#include "UserManager.h"
#include "UserRegistry.h"

using namespace Omega;
using namespace OTL;

namespace
{
	class AsyncSocket :
			public ObjectBase,
			public Net::IAsyncSocket,
			public OOSvrBase::IOHandler
	{
	public:
		AsyncSocket();
		virtual ~AsyncSocket();

		void Init(User::Manager* pManager, uint32_t id);

		BEGIN_INTERFACE_MAP(AsyncSocket)
			INTERFACE_ENTRY(Net::IAsyncSocketBase)
			INTERFACE_ENTRY(Net::IAsyncSocket)
		END_INTERFACE_MAP()

	private:
		OOBase::SpinLock m_lock;
		User::Manager*   m_pManager;
		uint32_t         m_id;

		ObjectPtr<Net::IAsyncSocketNotify> m_ptrNotify;
		Net::IAsyncSocket::BindFlags_t     m_flags;

		void on_recv(OOBase::Buffer* buffer, int err);
		void on_sent(OOBase::Buffer* buffer, int err);
		void on_closed();

	// Net::IAsyncSocketBase members
	public:
		void Recv(uint32_t lenBytes, bool_t bRecvAll);
		void Send(uint32_t lenBytes, const byte_t* bytes, bool_t bReliable);

	// Net::IAsyncSocket members
	public:
		Net::IAsyncSocketNotify* Bind(Net::IAsyncSocketNotify* pNotify, Net::IAsyncSocket::BindFlags_t flags);
	};
}

bool User::Manager::start_services()
{
	// Only valid for the sandbox process
	if (!m_bIsSandbox)
		return true;

	// Tell the root process that we are ready to receive service start requests...
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::ServicesStart));
	if (request.last_error() != 0)
		LOG_ERROR_RETURN(("Failed to write service data: %s",OOBase::system_error_text(request.last_error())),false);

	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	try
	{
		response = sendrecv_root(request,TypeInfo::Synchronous);
	}
	catch (IException* pE)
	{
		LOG_ERROR(("Sending message to root failed: %ls",pE->GetDescription()));
		pE->Release();
		return false;
	}

	if (!response)
		LOG_ERROR_RETURN(("No response from root"),false);

	size_t count = 0;
	if (!response->read(count))
		LOG_ERROR_RETURN(("Failed to read root response: %d",response->last_error()),false);

	// Loop through returned services, starting each one...
	for (;count > 0;--count)
	{
		OOBase::LocalString strKey;
		OOBase::LocalString strOid;
		if (!response->read(strKey) ||
			!response->read(strOid))
		{
			LOG_ERROR_RETURN(("Failed to read root response: %d",response->last_error()),false);
		}

		start_service(strKey.c_str(),strOid.c_str());
	}

	// Now start all the network services listening...
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

	for (size_t i=m_mapServices.begin();i!=m_mapServices.npos;i=m_mapServices.next(i))
	{
		Service* pServ = m_mapServices.at(i);

		guard.release();

		try
		{
			ObjectPtr<System::INetworkService> ptrNS = pServ->ptrService;
			if (ptrNS)
			{
				// Call the root, asking to start the async stuff, passing the id of the service...
				listen_service_socket(pServ->strKey.c_str(),*m_mapServices.key_at(i),ptrNS);
			}
		}
		catch (IException* pE)
		{
			LOG_ERROR(("Failed to start network service %s: %ls",pServ->strKey.c_str(),pE->GetDescription()));
		}

		guard.acquire();
	}
	
	return true;
}

void User::Manager::start_service(const char* pszKey, const char* pszOid)
{
	try
	{
		ObjectPtr<System::IService> ptrService(string_t(pszOid,true),Activation::OutOfProcess);

		// Get the service's source channel
		uint32_t src = Remoting::GetSource(ptrService);
		if (classify_channel(src) != 2)
			OMEGA_THROW("Service has activated in an unusual context");

		ObjectPtr<Omega::Registry::IKey> ptrKey = get_service_key(pszKey);
		ptrService->Start(ptrKey);

		ObjectPtr<System::INetworkService> ptrNetService;

		System::INetworkService* pNS = ptrService.QueryInterface<System::INetworkService>();
		if (pNS)
			ptrNetService.Attach(pNS);

		OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

		// If we add the derived interface then the proxy QI will be *much* quicker elsewhere
		Service svc;
		
		int err = svc.strKey.assign(pszKey);
		if (err != 0)
			OMEGA_THROW(err);
		
		if (pNS)
			svc.ptrService = pNS;
		else
			svc.ptrService = ptrService;

		uint32_t nServiceId = 0;
		err = m_mapServices.insert(svc,nServiceId,1,UINT_MAX);
		if (err != 0)
			OMEGA_THROW(err);	
	}
	catch (IException* pE)
	{
		LOG_ERROR(("Failed to start service %s: %ls",pszKey,pE->GetDescription().c_str()));
		pE->Release();
	}
}

ObjectPtr<Registry::IKey> User::Manager::get_service_key(const char* pszKey)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetServiceKey));
	request.write(pszKey);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response = sendrecv_root(request,TypeInfo::Synchronous);
	if (!response)
		OMEGA_THROW("No response from root");

	int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
		OMEGA_THROW(err);

	int64_t uKey = 0;
	OOBase::LocalString strKeyPath;
	if (!response->read(uKey) ||
		!response->read(strKeyPath))
	{
		OMEGA_THROW(response->last_error());
	}

	ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
	ptrKey->Init(this,string_t(strKeyPath.c_str(),true),uKey,0);

	ObjectPtr<Omega::Registry::IKey> ptrRet = ptrKey;
	return ptrRet;
}

void User::Manager::stop_services()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

	Service svc;
	while (m_mapServices.pop(NULL,&svc))
	{
		guard.release();

		try
		{
			svc.ptrService->Stop();
		}
		catch (IException* pE)
		{
			// ignore stop errors
			LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_str()));
			pE->Release();
		}

		guard.acquire();
	}
}

void User::Manager::listen_service_socket(const char* pszKey, uint32_t nServiceId, ObjectPtr<System::INetworkService> ptrNetService)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::ListenSocket));
	request.write(pszKey);
	request.write(nServiceId);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response = sendrecv_root(request,TypeInfo::Synchronous);
	if (!response)
		OMEGA_THROW("No response from root");

	int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
		OMEGA_THROW(err);
}

void User::Manager::on_socket_accept(OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	int32_t err = 0;
	uint32_t service_id = 0;
	uint32_t id = 0;

	if (!request.read(service_id) ||
		!request.read(id))
	{
		err = response.last_error();
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(err)));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		err = ENOENT;
		Service svc;
		if (m_mapServices.find(service_id,svc) && svc.ptrService)
		{
			try
			{
				ObjectPtr<System::INetworkService> ptrService = svc.ptrService;
				
				guard.release();

				// Create a socket
				ObjectPtr<ObjectImpl<AsyncSocket> > ptrSocket = ObjectImpl<AsyncSocket>::CreateInstancePtr();
				ptrSocket->Init(this,id);

				// Add to the map...
				OOBase::Guard<OOBase::RWMutex> guard2(m_service_lock);

				// Force an overwriting insert
				err = m_mapSockets.replace(id,ptrSocket);
				if (err != 0)
					LOG_ERROR(("Error adding socket: %s",OOBase::system_error_text(err)));
				else
				{
					guard2.release();

					// Let the service know...
					ptrService->OnAccept(ptrSocket);
				}
			}
			catch (IException* pE)
			{
				LOG_ERROR(("Sending message to root failed: %ls",pE->GetDescription().c_str()));
				pE->Release();
				err = EINVAL;
			}
		}
	}

	response.write(err);
}

void User::Manager::close_socket(Omega::uint32_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

	if (m_mapSockets.erase(id))
	{
		guard.release();

		// Send the close message to the root
		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SocketClose));
		request.write(id);
		if (request.last_error() != 0)
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())));
		else
			sendrecv_root(request,TypeInfo::Asynchronous);
	}
}

void User::Manager::on_socket_recv(OOBase::CDRStream& request)
{
	int32_t err = 0;
	uint32_t id = 0;

	if (!request.read(id) ||
		!request.read(err))
	{
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error())));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		OOSvrBase::IOHandler* handler = NULL;
		if (m_mapSockets.find(id,handler))
		{
			guard.release();

			try
			{
				handler->on_recv(request.buffer(),err);
			}
			catch (IException* pE)
			{
				LOG_ERROR(("on_recv failed: %ls",pE->GetDescription().c_str()));
				pE->Release();
			}
		}
	}
}

void User::Manager::on_socket_sent(OOBase::CDRStream& request)
{
	int32_t err = 0;
	uint32_t id = 0;

	if (!request.read(id) ||
		!request.read(err))
	{
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error())));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		OOSvrBase::IOHandler* handler = NULL;
		if (m_mapSockets.find(id,handler))
		{
			guard.release();

			try
			{
				handler->on_sent(request.buffer(),err);
			}
			catch (IException* pE)
			{
				LOG_ERROR(("on_sent failed: %ls",pE->GetDescription().c_str()));
				pE->Release();
			}
		}
	}
}

void User::Manager::on_socket_close(OOBase::CDRStream& request)
{
	uint32_t id = 0;

	if (!request.read(id))
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error())));
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		OOSvrBase::IOHandler* handler = NULL;
		if (m_mapSockets.find(id,handler))
		{
			guard.release();

			try
			{
				handler->on_closed();
			}
			catch (IException* pE)
			{
				LOG_ERROR(("on_close failed: %ls",pE->GetDescription().c_str()));
				pE->Release();
			}
		}
	}
}

AsyncSocket::AsyncSocket() :
		m_pManager(0),
		m_id(0),
		m_flags(Net::IAsyncSocket::Default)
{
}

AsyncSocket::~AsyncSocket()
{
	try
	{
		if (m_pManager && m_id)
			m_pManager->close_socket(m_id);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{ }
}

void AsyncSocket::Init(User::Manager* pManager, uint32_t id)
{
	m_pManager = pManager;
	m_id = id;
}

void AsyncSocket::Recv(uint32_t lenBytes, bool_t bRecvAll)
{
	// Zero recv is a no-op
	if (lenBytes == 0)
		return;

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SocketRecv));
	request.write(m_id);
	request.write(lenBytes);
	request.write(bRecvAll);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
		OMEGA_THROW(err);
}

void AsyncSocket::Send(uint32_t lenBytes, const byte_t* bytes, bool_t bReliable)
{
	OOBase::CDRStream request(lenBytes + 12);
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SocketSend));
	request.write(m_id);
	request.write(bReliable);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	if (lenBytes)
	{
		// Make room for the data
		int err = request.buffer()->space(lenBytes);
		if (err != 0)
			OMEGA_THROW(err);

		// Copy the data
		memcpy(request.buffer()->wr_ptr(),bytes,lenBytes);
		request.buffer()->wr_ptr(lenBytes);
	}

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
		OMEGA_THROW(err);
}

Net::IAsyncSocketNotify* AsyncSocket::Bind(Net::IAsyncSocketNotify* pNotify, Net::IAsyncSocket::BindFlags_t flags)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Net::IAsyncSocketNotify> ptrRet = m_ptrNotify;
	m_ptrNotify = pNotify;
	m_flags = flags;

	return ptrRet.AddRef();
}

void AsyncSocket::on_recv(OOBase::Buffer* buffer, int err)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrNotify)
		return;

	ObjectPtr<Net::IAsyncSocketNotify> ptrNotify = m_ptrNotify;

	ObjectPtr<Net::IAsyncSocket> ptrSocket;
	if (m_flags & Net::IAsyncSocket::IncludeSocket)
		ptrSocket = this;

	guard.release();

	ObjectPtr<ISystemException> ptrSE;
	if (err != 0)
		ptrSE.Attach(ISystemException::Create(err));

	if (buffer)
		ptrNotify->OnRecv(ptrSocket,buffer->length(),reinterpret_cast<const byte_t*>(buffer->rd_ptr()),ptrSE);
	else
		ptrNotify->OnRecv(ptrSocket,0,0,ptrSE);
}

void AsyncSocket::on_sent(OOBase::Buffer* buffer, int err)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrNotify)
		return;

	ObjectPtr<Net::IAsyncSocketNotify> ptrNotify = m_ptrNotify;

	ObjectPtr<Net::IAsyncSocket> ptrSocket;
	if (m_flags & Net::IAsyncSocket::IncludeSocket)
		ptrSocket = this;

	bool bIncludeData = (m_flags & Net::IAsyncSocket::IncludeSentData);

	guard.release();

	ObjectPtr<ISystemException> ptrSE;
	if (err != 0)
		ptrSE.Attach(ISystemException::Create(err));

	if (buffer && bIncludeData)
		ptrNotify->OnSent(ptrSocket,buffer->length(),reinterpret_cast<const byte_t*>(buffer->rd_ptr()),ptrSE);
	else
		ptrNotify->OnSent(ptrSocket,0,0,ptrSE);
}

void AsyncSocket::on_closed()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrNotify)
		return;

	ObjectPtr<Net::IAsyncSocketNotify> ptrNotify = m_ptrNotify;

	ObjectPtr<Net::IAsyncSocket> ptrSocket;
	if (m_flags & Net::IAsyncSocket::IncludeSocket)
		ptrSocket = this;

	guard.release();

	ptrNotify->OnClose(ptrSocket);
}
