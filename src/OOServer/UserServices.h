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

#ifndef OOSERVER_USER_SERVICES_H_INCLUDED_
#define OOSERVER_USER_SERVICES_H_INCLUDED_

namespace User
{
	class ServiceController :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactory<ServiceController,&Omega::System::OID_ServiceController,Omega::Activation::UserScope>,
			public Omega::System::IServiceController
	{
	public:
		BEGIN_INTERFACE_MAP(ServiceController)
			INTERFACE_ENTRY(Omega::System::IServiceController)
		END_INTERFACE_MAP()

	// IServiceController members
	public:
		void StartService(const Omega::string_t& strName);
		void StopService(const Omega::string_t& strName);
		Omega::bool_t IsServiceRunning(const Omega::string_t& strName);
		Omega::System::IServiceController::service_set_t GetRunningServices();
	};
}

#endif // OOSERVER_USER_SERVICES_H_INCLUDED_
