///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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
#include "SpawnedProcess.h"

int Root::Manager::registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Registry::Hive::access_rights_t access_mask)
{
	// Zero channel always has access...
	if (channel_id == 0)
		return 0;

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Find the process info
	const UserProcess* pU = m_mapUserProcesses.find(channel_id);
	if (!pU)
		return EINVAL;

	bool bRead = (access_mask & Registry::Hive::read_check) == Registry::Hive::read_check;
	bool bWrite = (access_mask & Registry::Hive::write_check) == Registry::Hive::write_check;

	// Check access
	bool bAllowed = false;
	if (!pU->ptrSpawn->CheckAccess(pszDb,bRead,bWrite,bAllowed))
		return EIO;
	else if (!bAllowed)
		return EACCES;
	else
		return 0;
}

int Root::Manager::registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Registry::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType)
{
	// Read uKey && nType
	if (!request.read(uKey) || !request.read(nType))
		return request.last_error();

	if (nType == 0)
	{
		// Return the system hive
		ptrHive = m_registry;
	}
	else if (nType == 1)
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Find the process info
		UserProcess* pU = m_mapUserProcesses.find(channel_id);
		if (!pU)
			return EINVAL;

		// Get the registry hive
		ptrHive = pU->ptrRegistry;

		// Clear channel id -  not used for user content
		channel_id = 0;
	}
	else
	{
		// What?!?
		return EIO;
	}

	return 0;
}

void Root::Manager::registry_open_mirror_key(Omega::uint32_t channel_id, OOBase::CDRStream& /*request*/, OOBase::CDRStream& response)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Find the process info
	Omega::byte_t local_type = 0;
	Omega::int32_t err = 0;
	const char* pszName = NULL;
	Omega::int64_t uKey;

	if (!m_mapUserProcesses.exists(channel_id))
		err = EINVAL;
	else
	{
		if (channel_id == m_sandbox_channel)
		{
			// Sandbox hive
			pszName = "/System/Sandbox";
		}
		else
		{
			// Get the registry hive
			pszName = "/All Users";
		}

		local_type = 1;

		err = m_registry->open_key(0,uKey,pszName+1,channel_id);
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
	{
		response.write(local_type);
		response.write(uKey);
		response.write(pszName);
	}
}

int Root::Manager::registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Registry::Hive>& ptrHive, Omega::int64_t& uKey)
{
	Omega::byte_t nType;
	return registry_open_hive(channel_id,request,ptrHive,uKey,nType);
}

void Root::Manager::registry_key_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		OOBase::LocalString strSubKey;
		if (!request.read(strSubKey))
			err = request.last_error();
		else
			err = ptrHive->open_key(uKey,uKey,strSubKey.c_str(),channel_id);
	}

	response.write(err);
}

void Root::Manager::registry_create_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey = 0;
	Omega::byte_t nType;
	Omega::int64_t uSubKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		OOBase::LocalString strSubKey;
		if (!request.read(strSubKey))
			err = request.last_error();
		else
		{
			Omega::uint16_t flags = 0;
			if (!request.read(flags))
				err = request.last_error();
			else
				err = ptrHive->create_key(uKey,uSubKey,strSubKey.c_str(),flags,Registry::Hive::inherit_checks,channel_id);
		}
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
	{
		response.write(uSubKey);
		response.write(nType);
	}
}

void Root::Manager::registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		OOBase::LocalString strSubKey;
		if (!request.read(strSubKey))
			err = request.last_error();
		else
			err = ptrHive->delete_key(uKey,strSubKey.c_str(),channel_id);
	}

	response.write(err);
}

void Root::Manager::registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err != 0)
	{
		response.write(err);
		return;
	}

	ptrHive->enum_subkeys(uKey,channel_id,response);
}

void Root::Manager::registry_value_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
			err = ptrHive->value_exists(uKey,strValue.c_str(),channel_id);
	}

	response.write(err);
}

void Root::Manager::registry_get_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::LocalString val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
			err = ptrHive->get_value(uKey,strValue.c_str(),channel_id,val);
	}

	response.write(err);
	if (err == 0)
		response.write(val.c_str());
}

void Root::Manager::registry_set_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
		{
			OOBase::LocalString val;
			if (!request.read(val))
				err = request.last_error();
			else
				err = ptrHive->set_value(uKey,strValue.c_str(),channel_id,val.c_str());
		}
	}

	response.write(err);
}

void Root::Manager::registry_set_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strDesc;
		if (!request.read(strDesc))
			err = request.last_error();
		else
			err = ptrHive->set_description(uKey,channel_id,strDesc.c_str());
	}

	response.write(err);
}

void Root::Manager::registry_set_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
		{
			OOBase::LocalString strDesc;
			if (!request.read(strDesc))
				err = request.last_error();
			else
				err = ptrHive->set_value_description(uKey,strValue.c_str(),channel_id,strDesc.c_str());
		}
	}

	response.write(err);
}

void Root::Manager::registry_get_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::LocalString val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		err = ptrHive->get_description(uKey,channel_id,val);

	response.write(err);
	if (err == 0)
		response.write(val.c_str());
}

void Root::Manager::registry_get_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::LocalString val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
			err = ptrHive->get_value_description(uKey,strValue.c_str(),channel_id,val);
	}

	response.write(err);
	if (err == 0)
		response.write(val.c_str());
}

void Root::Manager::registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		ptrHive->enum_values(uKey,channel_id,response);

	if (err != 0)
		response.write(err);
}

void Root::Manager::registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		OOBase::LocalString strValue;
		if (!request.read(strValue))
			err = request.last_error();
		else
			err = ptrHive->delete_value(uKey,strValue.c_str(),channel_id);
	}

	response.write(err);
}
