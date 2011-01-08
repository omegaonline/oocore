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
		LOG_ERROR_RETURN(("Failed to write service data: %s",OOBase::system_error_text(request.last_error()).c_str()),false);

	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	try
	{
		response = sendrecv_root(request,TypeInfo::Synchronous);
	}
	catch (IException* pE)
	{
		OOBase::stack_string s;
		pE->GetDescription().ToNative(s);
		LOG_ERROR(("Sending message to root failed: %s",s.c_str()));
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
		OOBase::string strKey;
		OOBase::string strOid;
		if (!response->read(strKey) ||
			!response->read(strOid))
		{
			LOG_ERROR_RETURN(("Failed to read root response: %d",response->last_error()),false);
		}

		start_service(strKey,strOid);
	}

	// Now start all the network services listening...
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		std::map<uint32_t,Service> mapServices = m_mapServices;

		guard.release();

		for (std::map<uint32_t,Service>::const_iterator i=mapServices.begin();i!=mapServices.end();++i)
		{
			try
			{
				ObjectPtr<System::INetworkService> ptrNS = i->second.ptrService;
				if (ptrNS)
				{
					// Call the root, asking to start the async stuff, passing the id of the service...
					listen_service_socket(i->second.strKey,i->first,ptrNS);
				}
			}
			catch (IException* pE)
			{
				OOBase::stack_string s;
				pE->GetDescription().ToNative(s);
				LOG_ERROR(("Failed to start network service %s: %s",i->second.strKey.c_str(),s.c_str()));
				pE->Release();
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("Failed to start services: %s",e.what()));
	}
	catch (IException* pE)
	{
		OOBase::stack_string s;
		pE->GetDescription().ToNative(s);
		LOG_ERROR(("Failed to start services: %s",s.c_str()));
		pE->Release();
	}


	return true;
}

void User::Manager::start_service(const OOBase::string& strKey, const OOBase::string& strOid)
{
	ObjectPtr<System::IService> ptrService;
	try
	{
		ObjectPtr<System::IService> ptrService(string_t(strOid.c_str(),true),Activation::OutOfProcess);

		// Get the service's source channel
		uint32_t src = Remoting::GetSource(ptrService);
		if (classify_channel(src) != 2)
			OMEGA_THROW("Service has activated in an unusual context");

		ObjectPtr<Omega::Registry::IKey> ptrKey = get_service_key(strKey);
		ptrService->Start(ptrKey);

		uint32_t nServiceId = 0;
		try
		{
			ObjectPtr<System::INetworkService> ptrNetService;

			System::INetworkService* pNS = ptrService.QueryInterface<System::INetworkService>();
			if (pNS)
				ptrNetService.Attach(pNS);

			OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

			do
			{
				nServiceId = (++m_nNextService);
			}
			while (!nServiceId && m_mapServices.find(m_nNextService) != m_mapServices.end());

			// If we add the derived interface then the proxy QI will be *much* quicker elsewhere
			Service svc;
			svc.strKey = strKey;

			if (pNS)
				svc.ptrService = pNS;
			else
				svc.ptrService = ptrService;

			m_mapServices.insert(std::map<uint32_t,Service>::value_type(nServiceId,svc));
		}
		catch (...)
		{
			// Try to be nice and stop the service
			try
			{
				ptrService->Stop();
			}
			catch (IException* pE)
			{
				// ignore stop errors
				pE->Release();
			}

			try
			{
				OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

				m_mapServices.erase(nServiceId);
			}
			catch (std::exception&)
			{}

			throw;
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("Failed to start service %s: %s",strKey.c_str(),e.what()));
	}
	catch (IException* pE)
	{
		OOBase::stack_string s;
		pE->GetDescription().ToNative(s);
		LOG_ERROR(("Failed to start service %s: %s",strKey.c_str(),s.c_str()));
		pE->Release();
	}
}

ObjectPtr<Registry::IKey> User::Manager::get_service_key(const OOBase::string& strKey)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetServiceKey));
	request.write(strKey);
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
	OOBase::string strKeyPath;
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
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_service_lock);

		std::map<uint32_t,Service> mapServices = m_mapServices;
		m_mapServices.clear();

		guard.release();

		for (std::map<uint32_t,Service>::iterator i=mapServices.begin();i!=mapServices.end();++i)
		{
			try
			{
				i->second.ptrService->Stop();
			}
			catch (IException* pE)
			{
				// ignore stop errors
				pE->Release();
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("std::exception thrown %s",e.what()));
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_str()));
		pE->Release();
	}
}

void User::Manager::listen_service_socket(const OOBase::string& strKey, uint32_t nServiceId, ObjectPtr<System::INetworkService> ptrNetService)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::ListenSocket));
	request.write(strKey);
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
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(err).c_str()));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		err = ENOENT;
		std::map<uint32_t,Service>::iterator i = m_mapServices.find(service_id);
		if (i != m_mapServices.end())
		{
			try
			{
				ObjectPtr<System::INetworkService> ptrService = i->second.ptrService;
				if (ptrService)
				{
					guard.release();

					// Create a socket
					ObjectPtr<ObjectImpl<AsyncSocket> > ptrSocket = ObjectImpl<AsyncSocket>::CreateInstancePtr();
					ptrSocket->Init(this,id);

					// Add to the map...
					OOBase::Guard<OOBase::RWMutex> guard2(m_service_lock);

					// Force an overwriting insert
					m_mapSockets[id] = ptrSocket;

					guard2.release();

					// Let the service know...
					ptrService->OnAccept(ptrSocket);
					err = 0;
				}
			}
			catch (IException* pE)
			{
				OOBase::stack_string s;
				pE->GetDescription().ToNative(s);
				LOG_ERROR(("Sending message to root failed: %s",s.c_str()));
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

	if (m_mapSockets.erase(id) == 1)
	{
		guard.release();

		// Send the close message to the root
		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SocketClose));
		request.write(id);
		if (request.last_error() != 0)
			LOG_ERROR(("Failed to write request data: %s",OOBase::system_error_text(request.last_error()).c_str()));
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
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error()).c_str()));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		try
		{
			std::map<Omega::uint32_t,OOSvrBase::IOHandler*>::iterator i = m_mapSockets.find(id);
			if (i != m_mapSockets.end())
				i->second->on_recv(request.buffer(),err);
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("on_recv failed: %s",e.what()));
		}
		catch (IException* pE)
		{
			OOBase::stack_string s;
			pE->GetDescription().ToNative(s);
			LOG_ERROR(("on_recv failed: %s",s.c_str()));
			pE->Release();
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
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error()).c_str()));
	}
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		try
		{
			std::map<Omega::uint32_t,OOSvrBase::IOHandler*>::iterator i = m_mapSockets.find(id);
			if (i != m_mapSockets.end())
				i->second->on_sent(request.buffer(),err);
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("on_sent failed: %s",e.what()));
		}
		catch (IException* pE)
		{
			OOBase::stack_string s;
			pE->GetDescription().ToNative(s);
			LOG_ERROR(("on_sent failed: %s",s.c_str()));
			pE->Release();
		}
	}
}

void User::Manager::on_socket_close(OOBase::CDRStream& request)
{
	uint32_t id = 0;

	if (!request.read(id))
		LOG_ERROR(("Failed to read request: %s",OOBase::system_error_text(request.last_error()).c_str()));
	else
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_service_lock);

		try
		{
			std::map<Omega::uint32_t,OOSvrBase::IOHandler*>::iterator i = m_mapSockets.find(id);
			if (i != m_mapSockets.end())
				i->second->on_closed();
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("on_close failed: %s",e.what()));
		}
		catch (IException* pE)
		{
			OOBase::stack_string s;
			pE->GetDescription().ToNative(s);
			LOG_ERROR(("on_close failed: %s",s.c_str()));
			pE->Release();
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
