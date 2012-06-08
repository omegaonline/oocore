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

namespace
{
	bool get_service_dependencies(OOBase::SmartPtr<Db::Hive> ptrRegistry, const Omega::int64_t key, const OOBase::String& strName, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames, OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator>& queueKeys)
	{
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign(strName.c_str(),strName.length());
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t subkey = key;
		Db::hive_errors err = ptrRegistry->create_key(key,subkey,strSubKey,0,0,strLink,strFullKeyName);
		if (err)
			LOG_ERROR_RETURN(("Failed to open the '/System/Services/%s' key in the registry",strName.c_str()),false);

		// Add the name to the name queue
		// This prevents circular dependencies
		err2 = queueNames.push(strName);
		if (!err2)
			err2 = queueKeys.push(key);
		if (err2)
			LOG_ERROR_RETURN(("Failed to push to queue: %s",OOBase::system_error_text(err2)),false);

		// Check dependencies
		err2 = strSubKey.assign("Dependencies");
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t depskey = subkey;
		err = ptrRegistry->create_key(subkey,depskey,strSubKey,0,0,strLink,strFullKeyName);
		if (err && err != Db::HIVE_NOTFOUND)
			LOG_ERROR_RETURN(("Failed to open the '/System/Services/%s/Dependencies' key in the registry",strName.c_str()),false);
		else if (!err)
		{
			Db::Hive::registry_set_t names;
			err = ptrRegistry->enum_values(depskey,0,names);
			if (err)
				LOG_ERROR_RETURN(("Failed to enumerate the '/System/Services/%s/Dependencies' values in the user registry",strName.c_str()),false);

			OOBase::String strDep;
			while (names.pop(&strDep))
			{
				if (!queueNames.find(strDep) && !get_service_dependencies(ptrRegistry,key,strDep,queueNames,queueKeys))
					return false;
			}
		}

		return true;
	}

	bool enum_services(OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames, OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator>& queueKeys)
	{
		Omega::int64_t key = 0;
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign("/System/Services");
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Db::hive_errors err = ptrRegistry->create_key(0,key,strSubKey,0,0,strLink,strFullKeyName);
		if (err)
		{
			if (err != Db::HIVE_NOTFOUND)
				LOG_ERROR_RETURN(("Failed to open the '/System/Services' key in the registry"),false);

			return true;
		}

		// Enum each service, building a queue of dependencies...
		Db::Hive::registry_set_t keys;
		err = ptrRegistry->enum_subkeys(key,0,keys);
		if (err)
			LOG_ERROR_RETURN(("Failed to enumerate the '/System/Services' values in the registry"),false);

		OOBase::String strName;
		while (keys.pop(&strName))
		{
			if (!queueNames.find(strName) && !get_service_dependencies(ptrRegistry,key,strName,queueNames,queueKeys))
				return false;
		}

		return true;
	}
}

bool Root::Manager::start_services()
{
	// Get the list of services, ordered by dependency
	OOBase::Queue<OOBase::String,OOBase::LocalAllocator> queueNames;
	OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator> queueKeys;
	if (!enum_services(m_registry,queueNames,queueKeys))
		return false;

	// Remove the associated spawned process
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	UserProcess sandbox;
	if (!m_sandbox_channel || !m_mapUserProcesses.find(m_sandbox_channel,sandbox))
		LOG_ERROR_RETURN(("Failed to find sandbox process"),false);

	guard.release();

	OOBase::String strName;
	Omega::int64_t key;
	while (queueNames.pop(&strName) && queueKeys.pop(&key))
	{
		OOBase::Logger::log(OOBase::Logger::Information,"Starting service: %s",strName.c_str());

		// Get the timeout value
		unsigned long wait_secs = 0;
		OOBase::LocalString strTimeout;
		if (m_registry->get_value(key,"Timeout",0,strTimeout) == Db::HIVE_OK)
			wait_secs = strtoul(strTimeout.c_str(),NULL,10);
		
		if (wait_secs == 0)
			wait_secs = 15;

		// Create a unique local socket name
		OOBase::RefPtr<OOBase::Socket> ptrSocket = sandbox.m_ptrProcess->LaunchService(this,strName,key,wait_secs);
		if (ptrSocket)
		{

		}
	}

	return true;
}
