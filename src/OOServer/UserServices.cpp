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

void User::Manager::start_service(OOBase::CDRStream& request)
{
	if (!m_bIsSandbox)
		LOG_ERROR(("Request to start service received in non-sandbox host"));
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
		}
		else
		{
			try
			{
				// Wrap up a registry key
				ObjectPtr<ObjectImpl<Registry::RootKey> > ptrKey = ObjectImpl<User::Registry::RootKey>::CreateInstance();
				ptrKey->init(string_t::constant("/System/Services/") + strName.c_str(),key,0);

				// Return a pointer to a IService interface and place in stack
				ObjectPtr<System::IService> ptrService = ObjectPtr<OOCore::IServiceManager>("Omega.ServiceHost")->Start(strPipe.c_str(),strName.c_str(),ptrKey,strSecret.c_str());
				if (ptrService)
				{
					OOBase::Guard<OOBase::RWMutex> guard(m_lock);

					int err = m_mapServices.push(ptrService);
					if (err)
						OMEGA_THROW(err);
				}
			}
			catch (IException* pE)
			{
				ObjectPtr<IException> ptrE = pE;
				OOBase::Logger::log(OOBase::Logger::Warning,"Failed to start service '%s': %s",strName.c_str(),recurse_log_exception(ptrE).c_str());
			}
		}
	}
}

int User::Manager::stop_services()
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ObjectPtr<System::IService> ptrService;
	while (m_mapServices.pop(&ptrService))
	{
		guard.release();

		ptrService->Stop();

		guard.acquire();
	}

	return 0;
}

namespace
{
	void ThrowCorrectException(OOServer::RootErrCode_t err, const string_t& strService)
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
			throw IInternalException::Create(string_t::constant("Internal service controller failure.  Check server log for details"),strService.c_str(),0,NULL,NULL);
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
