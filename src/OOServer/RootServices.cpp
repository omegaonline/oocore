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
	bool get_service_dependencies(OOBase::SmartPtr<Db::Hive> ptrRegistry, const Omega::int64_t key, const OOBase::String& strName, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames)
	{
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign(strName.c_str(),strName.length());
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t subkey = key;
		Db::hive_errors err = ptrRegistry->create_key(key,subkey,strSubKey,0,0,strLink,strFullKeyName);
		if (err)
			LOG_ERROR_RETURN(("Failed to open the '/System/Services/%s' key in the registry",strName.c_str()),false);

		// Check we have an OID
		OOBase::LocalString strVal;
		err = ptrRegistry->get_value(subkey,"OID",0,strVal);
		if (err)
		{
			OOBase::Logger::log(OOBase::Logger::Warning,"Failed to get the OID value for the '/System/Services/%s' key in the registry",strName.c_str());
			return true;
		}

		// Add the name to the name queue
		// This prevents circular dependencies
		err2 = queueNames.push(strName);
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
				if (!queueNames.find(strDep) && !get_service_dependencies(ptrRegistry,key,strDep,queueNames))
					return false;
			}
		}

		return true;
	}

	bool enum_services(OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames)
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
			if (!queueNames.find(strName) && !get_service_dependencies(ptrRegistry,key,strName,queueNames))
				return false;
		}

		return true;
	}
}

bool Root::Manager::start_services()
{
	// Get the list of services, ordered by dependency

	OOBase::Queue<OOBase::String,OOBase::LocalAllocator> queueNames;
	if (!enum_services(m_registry,queueNames))
		return false;

	OOBase::String info;
	while (queueNames.pop(&info))
	{
		/*
		 * Create a unique local socket name
		 *
		 * Send name to sandbox via a root message + strName
		 *
		 * Listen for a single connection on the new local socket
		 *
		 * Accept 1st connection
		 *
		 * Read strName back and check, or the child has exited
		 *
		 * Open as many sockets as the service wants, and send down local socket
		 *
		 * close()
		 *
		 */
	}

	return true;
}
