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
#include "RootProcess.h"

int Root::Manager::registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Db::Hive::access_rights_t access_mask)
{
	// Zero channel always has access...
	if (channel_id == 0)
		return 0;

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Find the process info
	const UserProcess* pU = m_mapUserProcesses.find(channel_id);
	if (!pU)
		return EINVAL;

	bool bRead = (access_mask & Db::Hive::read_check) == Db::Hive::read_check;
	bool bWrite = (access_mask & Db::Hive::write_check) == Db::Hive::write_check;

	// Check access
	bool bAllowed = false;
	if (!pU->m_ptrProcess->CheckAccess(pszDb,bRead,bWrite,bAllowed))
		return EIO;
	else if (!bAllowed)
		return EACCES;
	else
		return 0;
}

int Root::Manager::registry_open_hive(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Db::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType)
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
		ptrHive = pU->m_ptrRegistry;
	}
	else
	{
		// What?!?
		return EINVAL;
	}

	return 0;
}

int Root::Manager::registry_open_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::LocalString strSubKey;
	int err = strSubKey.assign(pszSubKey);
	if (err != 0)
		return err;

	OOBase::LocalString strLink;
	return m_registry->create_key(uParent,uKey,strSubKey,0,channel_id,strLink);
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

		err = registry_open_key(0,uKey,pszName+1,channel_id);
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
	{
		response.write(local_type);
		response.write(uKey);
		response.write(pszName);
	}
}

int Root::Manager::registry_open_link(Omega::uint32_t channel_id, const OOBase::LocalString& strLink, OOBase::LocalString& strSubKey, Omega::byte_t& nType, OOBase::SmartPtr<Db::Hive>& ptrHive)
{
	return ENOEXEC;
}

void Root::Manager::registry_open_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey = 0;
	Omega::byte_t nType;
	Omega::int64_t uSubKey;
	OOBase::LocalString strLink;
	OOBase::LocalString strSubKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		if (!request.read(strSubKey))
			err = request.last_error();
		else
		{
			Omega::uint16_t flags = 0;
			if (!request.read(flags))
				err = request.last_error();
			else
			{
				err = ptrHive->create_key(uKey,uSubKey,strSubKey,flags,channel_id,strLink);
				if (err == ENOEXEC)
				{
					err = registry_open_link(channel_id,strLink,strSubKey,nType,ptrHive);
					if (err == 0)
						err = ptrHive->create_key(0,uSubKey,strSubKey,flags,channel_id,strLink);
				}									
			}
		}
	}

	response.write(err);
	if (response.last_error() == 0)
	{
		if (err == ENOEXEC)
		{
			response.write(strLink.c_str(),strLink.length());
			response.write(strSubKey.c_str(),strSubKey.length());
		}
		else
		{
			response.write(uSubKey);
			response.write(nType);
		}
	}
}

void Root::Manager::registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	OOBase::LocalString strLink;
	OOBase::LocalString strSubKey;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		if (!request.read(strSubKey))
			err = request.last_error();
		else
		{
			err = ptrHive->delete_key(uKey,strSubKey,channel_id,strLink);
			if (err == ENOEXEC)
			{
				if (!strLink.empty() && strSubKey.empty())
				{
					// Attempt to delete link
					err = EACCES;
				}
				else if (err == ENOEXEC)
				{
					err = registry_open_link(channel_id,strLink,strSubKey,nType,ptrHive);
					if (err == 0)
						err = ptrHive->delete_key(0,strSubKey,channel_id,strLink);
				}
			}
		}
	}

	response.write(err);
	if (err == ENOEXEC && response.last_error() == 0)
	{
		response.write(strLink.c_str(),strLink.length());
		response.write(strSubKey.c_str(),strSubKey.length());
	}
}

void Root::Manager::registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Db::Hive> ptrHive;
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
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
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

	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
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
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
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

void Root::Manager::registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
		ptrHive->enum_values(uKey,channel_id,response);

	if (err != 0)
		response.write(err);
}

void Root::Manager::registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Db::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	Omega::int32_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
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
