///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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
#include "UserServices.h"
#include "UserManager.h"
#include "UserRegistry.h"

using namespace Omega;
using namespace OTL;

void User::Manager::start_service(OOBase::CDRStream& request, OOBase::CDRStream* response)
{
	OOServer::RootErrCode_t err = OOServer::Ok;
	if (!m_bIsSandbox)
	{
		LOG_ERROR(("Request to start service received in non-sandbox host"));
		err = OOServer::Errored;
	}
	else
	{
		ServiceEntry entry;
		OOBase::StackAllocator<512> allocator;
		OOBase::LocalString strPipe(allocator),strSecret(allocator);
		int64_t key = 0;
		if (!request.read_string(strPipe) ||
				!request.read_string(entry.strName) ||
				!request.read(key) ||
				!request.read_string(strSecret))
		{
			LOG_ERROR(("Failed to read service start args: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			// Check if it's running already
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			bool found = false;
			for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
			{
				if (m_mapServices.at(pos)->strName == entry.strName)
				{
					found = true;
					break;
				}
			}

			if (found)
			{
				guard.release();

				if (!response)
					OOBase::Logger::log(OOBase::Logger::Warning,"Service '%s' is already running",entry.strName.c_str());

				err = OOServer::AlreadyExists;
			}
			else
			{
				// Insert an entry with a NULL pointer, we later update it
				int err2 = m_mapServices.push_back(entry);
				if (err2)
				{
					LOG_ERROR(("Failed to insert service entry: %s",OOBase::system_error_text(err2)));
					err = OOServer::Errored;
				}
				else
				{
					guard.release();

					try
					{
						// Wrap up a registry key
						ObjectPtr<ObjectImpl<Registry::RootKey> > ptrKey = ObjectImpl<User::Registry::RootKey>::CreateObject();
						ptrKey->init(string_t::constant("/System/Services/") + entry.strName.c_str(),key,0);

						// Return a pointer to a IService interface and place in stack
						ObjectPtr<OOCore::IServiceManager> ptrServiceManager("Omega.ServiceHost");
						entry.ptrService = ptrServiceManager->Create(ptrKey->GetValue(string_t::constant("OID")));

						guard.acquire();

						for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
						{
							if (m_mapServices.at(pos)->strName == entry.strName && !m_mapServices.at(pos)->ptrService)
							{
								ServiceEntry* se = const_cast<ServiceEntry*>(m_mapServices.at(pos));
								se->ptrService = entry.ptrService;
								found = true;
								break;
							}
						}

						guard.release();

						if (found)
							ptrServiceManager->Start(entry.ptrService,entry.strName.c_str(),strPipe.c_str(),ptrKey,strSecret.c_str());
					}
					catch (IException* pE)
					{
						ObjectPtr<IException> ptrE = pE;
						OOBase::Logger::log(OOBase::Logger::Warning,"Failed to start service '%s': %s",entry.strName.c_str(),recurse_log_exception(ptrE).c_str());
						err = OOServer::Errored;

						guard.acquire();

						// Remove any entries from the map
						for (OOBase::Vector<ServiceEntry>::iterator i=m_mapServices.begin(); i!=m_mapServices.end(); ++i)
						{
							if (i->strName == entry.strName)
							{
								m_mapServices.remove(i);
								break;
							}
						}
					}
				}
			}
		}
	}

	if (response && !response->write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response->last_error())));
}

OOServer::RootErrCode_t User::Manager::stop_all_services()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ServiceEntry entry;
	while (m_mapServices.pop_back(&entry))
	{
		if (entry.ptrService && Remoting::IsAlive(entry.ptrService))
		{
			guard.release();

			try
			{
				entry.ptrService->Stop();
			}
			catch (IException* pE)
			{
				ObjectPtr<IException> ptrE = pE;
				OOBase::Logger::log(OOBase::Logger::Warning,"Failed to stop service '%s': %s",entry.strName.c_str(),recurse_log_exception(ptrE).c_str());
			}

			guard.acquire();
		}
	}

	return OOServer::Ok;
}

void User::Manager::stop_all_services(OOBase::CDRStream& response)
{
	OOServer::RootErrCode_t err;
	if (!m_bIsSandbox)
	{
		LOG_ERROR(("Request to stop services received in non-sandbox host"));
		err = OOServer::Errored;
	}
	else
		err = stop_all_services();

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void User::Manager::stop_service(OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOServer::RootErrCode_t err = OOServer::Ok;
	if (!m_bIsSandbox)
	{
		LOG_ERROR(("Request to stop service received in non-sandbox host"));
		err = OOServer::Errored;
	}
	else
	{
		OOBase::String strName;
		if (!request.read_string(strName))
		{
			LOG_ERROR(("Failed to read service stop request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			ServiceEntry entry;

			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (OOBase::Vector<ServiceEntry>::iterator i=m_mapServices.begin(); i!=m_mapServices.end(); ++i)
			{
				if (i->strName == strName)
				{
					m_mapServices.remove(i,&entry);
					break;
				}
			}

			guard.release();

			if (entry.ptrService && Remoting::IsAlive(entry.ptrService))
			{
				try
				{
					entry.ptrService->Stop();
				}
				catch (IException* pE)
				{
					ObjectPtr<IException> ptrE = pE;
					OOBase::Logger::log(OOBase::Logger::Warning,"Failed to stop service '%s': %s",entry.strName.c_str(),recurse_log_exception(ptrE).c_str());
					err = OOServer::Errored;
				}
			}
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void User::Manager::service_is_running(OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOServer::RootErrCode_t err = OOServer::Ok;
	bool found = false;

	if (!m_bIsSandbox)
	{
		LOG_ERROR(("Request for service status received in non-sandbox host"));
		err = OOServer::Errored;
	}
	else
	{
		OOBase::String strName;
		if (!request.read_string(strName))
		{
			LOG_ERROR(("Failed to read service status request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (OOBase::Vector<ServiceEntry>::iterator i=m_mapServices.begin(); i!=m_mapServices.end(); ++i)
			{
				if (i->strName == strName)
				{
					if (!i->ptrService || Remoting::IsAlive(i->ptrService))
						found = true;
					else
						m_mapServices.remove(i);
					break;
				}
			}
		}
	}

	if (!response.write(err) || (!err && !response.write(found)))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void User::Manager::list_services(OOBase::CDRStream& response)
{
	OOServer::RootErrCode_t err = OOServer::Ok;
	if (!m_bIsSandbox)
	{
		LOG_ERROR(("Request for service status received in non-sandbox host"));
		err = OOServer::Errored;
	}
	else
	{
		response.write(err);

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		OOBase::Vector<ServiceEntry>::iterator i = m_mapServices.end();
		while ( --i != m_mapServices.begin())
		{
			if (!i->ptrService || Remoting::IsAlive(i->ptrService))
			{
				if (!response.write_string(i->strName))
					break;
			}
			else
				m_mapServices.remove(i);
		}

		guard.release();

		response.write("",0);

		if (response.last_error())
		{
			LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
			err = OOServer::Errored;
		}
	}

	if (err)
	{
		response.reset();
		if (!response.write(err))
			LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
	}
}

namespace
{
	void ThrowCorrectException(OOServer::RootErrCode_t err, const string_t& strService = string_t())
	{
		switch (err)
		{
		case OOServer::Ok:
			break;

		case OOServer::NotFound:
			throw INotFoundException::Create(string_t::constant("The service '{0}' does not exist or is not running") % strService);

		case OOServer::AlreadyExists:
			throw INotFoundException::Create(string_t::constant("The service '{0}' is already running") % strService);

		case OOServer::NoWrite:
			throw IAccessDeniedException::Create(string_t::constant("You do not have permissions to start or stop services"));

		case OOServer::Errored:
		default:
			throw IInternalException::Create(string_t::constant("The call to the service controller failed.  Check server log for details"),"IServiceController");
		}
	}
}

void User::ServiceController::StartService(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Service_Start));
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);

	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err,strName);
}

void User::ServiceController::StopService(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Service_Stop));
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);

	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err,strName);
}

bool_t User::ServiceController::IsServiceRunning(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Service_IsRunning));
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);

	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	bool retval = false;
	if (!err && !response.read(retval))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err,strName);

	return retval;
}

System::IServiceController::service_set_t User::ServiceController::GetRunningServices()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Service_ListRunning));

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);

	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err);

	OOBase::StackAllocator<512> allocator;
	System::IServiceController::service_set_t values;
	for (;;)
	{
		OOBase::LocalString strName(allocator);
		if (!response.read_string(strName))
			OMEGA_THROW(response.last_error());

		if (strName.empty())
			break;

		values.insert(strName.c_str());
	}
	return values;
}
