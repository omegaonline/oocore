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

// {D2A10F8C-ECD1-F698-7105-48247D50DB1B}
OMEGA_DEFINE_OID(System,OID_ServiceController,"{D2A10F8C-ECD1-F698-7105-48247D50DB1B}");

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
		OOBase::LocalString strPipe,strName,strSecret;
		int64_t key = 0;
		if (!request.read(strPipe) ||
				!request.read(strName) ||
				!request.read(key) ||
				!request.read(strSecret))
		{
			LOG_ERROR(("Failed to read service start args: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			ServiceEntry entry;
			int err2 = entry.strName.assign(strName.c_str());
			if (err2)
			{
				LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err2)));
				err = OOServer::Errored;
			}
			else
			{
				// Check if it's running already
				OOBase::Guard<OOBase::RWMutex> guard(m_lock);

				bool found = false;
				for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
				{
					if (m_mapServices.at(pos)->strName == strName.c_str())
					{
						found = true;
						break;
					}
				}

				if (found)
				{
					if (!response)
						OOBase::Logger::log(OOBase::Logger::Warning,"Service '%s' is already running",strName.c_str());

					err = OOServer::AlreadyExists;
				}
				else
				{
					// Insert an entry with a NULL pointer, we later update it
					err2 = m_mapServices.push(entry);
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
							ObjectPtr<ObjectImpl<Registry::RootKey> > ptrKey = ObjectImpl<User::Registry::RootKey>::CreateInstance();
							ptrKey->init(string_t::constant("/System/Services/") + strName.c_str(),key,0);

							// Return a pointer to a IService interface and place in stack
							ObjectPtr<Omega::System::IService> ptrService = ObjectPtr<OOCore::IServiceManager>("Omega.ServiceHost")->Start(strPipe.c_str(),strName.c_str(),ptrKey,strSecret.c_str());

							guard.acquire();

							for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
							{
								if (m_mapServices.at(pos)->strName == strName.c_str())
								{
									m_mapServices.at(pos)->ptrService = ptrService;
									found = true;
									break;
								}
							}

							if (!found)
							{
								LOG_ERROR(("Failed to find service start partial entry"));
								err = OOServer::Errored;
							}
						}
						catch (IException* pE)
						{
							ObjectPtr<IException> ptrE = pE;
							OOBase::Logger::log(OOBase::Logger::Warning,"Failed to start service '%s': %s",strName.c_str(),recurse_log_exception(ptrE).c_str());
							err = OOServer::Errored;
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
	while (m_mapServices.pop(&entry))
	{
		if (entry.ptrService)
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
		OOBase::LocalString strName;
		if (!request.read(strName))
		{
			LOG_ERROR(("Failed to read service stop request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			ServiceEntry entry;

			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
			{
				if (m_mapServices.at(pos)->strName == strName.c_str())
				{
					entry = *m_mapServices.at(pos);
					m_mapServices.remove_at(pos);
					break;
				}
			}

			guard.release();

			if (!entry.ptrService)
				err = OOServer::NotFound;
			else
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
		OOBase::LocalString strName;
		if (!request.read(strName))
		{
			LOG_ERROR(("Failed to read service status request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (size_t pos = 0;pos < m_mapServices.size(); ++pos)
			{
				if (m_mapServices.at(pos)->strName == strName.c_str())
				{
					if (m_mapServices.at(pos)->ptrService)
						found = true;

					break;
				}
			}
		}
	}

	if (!response.write(err) || (!err && !response.write(found)))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
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
			throw IInternalException::Create(string_t::constant("Failed to start or stop service '{0}'.  Check server log for details") % strService,strService.c_str(),0,NULL,NULL);
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

	System::IServiceController::service_set_t values;
	for (;;)
	{
		OOBase::LocalString strName;
		if (!response.read(strName))
			OMEGA_THROW(response.last_error());

		if (strName.empty())
			break;

		values.insert(strName.c_str());
	}
	return values;
}
