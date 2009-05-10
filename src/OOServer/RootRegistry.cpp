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
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "SpawnedProcess.h"

int Root::Manager::registry_access_check(const std::string& strdb, Omega::uint32_t channel_id, Registry::Hive::access_rights_t access_mask)
{
	// Zero channel always has access...
	if (channel_id == 0)
		return 0;

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Find the process info
	std::map<Omega::uint32_t,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
	if (i == m_mapUserProcesses.end())
		return EINVAL;

	bool bRead = (access_mask & Registry::Hive::read_check) == Registry::Hive::read_check;
	bool bWrite = (access_mask & Registry::Hive::write_check) == Registry::Hive::write_check;

	// Check access
	bool bAllowed = false;
	if (!i->second.ptrSpawn->CheckAccess(strdb.c_str(),bRead,bWrite,bAllowed))
		return EIO;
	else if (!bAllowed)
		return EACCES;
	else
		return 0;
}

int Root::Manager::registry_parse_subkey(const Omega::int64_t& uKey, Omega::uint32_t& channel_id, std::string& strSubKey, Omega::byte_t& nType, OOBase::SmartPtr<Registry::Hive>& ptrHive)
{
	int err = 0;
	if (uKey == 0 && nType == 0)
	{
		// Parse strKey
		if (strSubKey == "Local User" || strSubKey.substr(0,11) == "Local User\\")
		{
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			// Find the process info
			std::map<Omega::uint32_t,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
			if (i == m_mapUserProcesses.end())
				err = EINVAL;
			else
			{
				// Get the registry hive
				ptrHive = i->second.ptrRegistry;

				// Clear channel id -  not used for user content
				channel_id = 0;

				// Set the type and strip the start...
				if (strSubKey.length() > 11)
					strSubKey = strSubKey.substr(11);
				else
					strSubKey.clear();
				nType = 2;
			}
		}
		else if (strSubKey == "All Users" || strSubKey.substr(0,10) == "All Users\\")
		{
			ptrHive = m_registry_all_users;

			// Set the type and strip the start...
			if (strSubKey.length() > 10)
				strSubKey = strSubKey.substr(10);
			else
				strSubKey.clear();
			nType = 1;
		}
	}

	return err;
}

int Root::Manager::registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Registry::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType)
{
	// Read uKey && nType
	if (!request.read(uKey) || !request.read(nType))
		return EIO;

	if (nType == 0)
	{
		// Return the system hive
		ptrHive = m_registry;
	}
	else if (nType == 1)
	{
		ptrHive = m_registry_all_users;
	}
	else if (nType == 2)
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		// Find the process info
		std::map<Omega::uint32_t,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
		if (i == m_mapUserProcesses.end())
			return EINVAL;

		// Get the registry hive
		ptrHive = i->second.ptrRegistry;

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
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		std::string strSubKey;
		if (!request.read(strSubKey))
			err = EIO;
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,nType,ptrHive);
			if (err == 0)
				err = ptrHive->open_key(uKey,strSubKey,channel_id);
		}
	}

	response.write(err);
}

void Root::Manager::registry_create_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey = 0;
	Omega::byte_t nType;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		std::string strSubKey;
		if (!request.read(strSubKey))
			err = EIO;
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,nType,ptrHive);
			if (err == 0)
			{
				Omega::bool_t bCreate = false;
				if (!request.read(bCreate))
					err = EIO;
				else
				{
					Omega::bool_t bFailIfThere = false;
					if (!request.read(bFailIfThere))
						err = EIO;
					else
					{
						if (bCreate)
							err = ptrHive->create_key(uKey,strSubKey,bFailIfThere,Registry::Hive::inherit_checks,channel_id);
						else
							err = ptrHive->open_key(uKey,strSubKey,channel_id);
					}
				}
			}
		}
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
	{
		response.write(uKey);
		response.write(nType);
	}
}

void Root::Manager::registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	Omega::byte_t nType;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		std::string strSubKey;
		if (!request.read(strSubKey))
			err = EIO;
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,nType,ptrHive);
			if (err == 0)
				err = ptrHive->delete_key(uKey,strSubKey,channel_id);
		}
	}

	response.write(err);
}

void Root::Manager::registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		ptrHive->enum_subkeys(uKey,channel_id,response);

	if (err != 0)
		response.write(err);
}

void Root::Manager::registry_value_type(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::byte_t type = 0;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->get_value_type(uKey,strValue,channel_id,type);
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
		response.write(type);
}

void Root::Manager::registry_get_string_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	std::string val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->get_string_value(uKey,strValue,channel_id,val);
	}

	response.write(err);
	if (err == 0)
		response.write(val);
}

void Root::Manager::registry_get_int_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	Omega::int64_t val = 0;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->get_integer_value(uKey,strValue,channel_id,val);
	}

	response.write(err);
	if (err == 0 && response.last_error() == 0)
		response.write(val);
}

void Root::Manager::registry_get_binary_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
		{
			Omega::uint32_t cbLen = 0;
			if (!request.read(cbLen))
				err = EIO;
			else
				ptrHive->get_binary_value(uKey,strValue,cbLen,channel_id,response);
		}
	}

	if (err != 0)
		response.write(err);
}

void Root::Manager::registry_set_string_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
		{
			std::string val;
			if (!request.read(val))
				err = EIO;
			else
				err = ptrHive->set_string_value(uKey,strValue,channel_id,val.c_str());
		}
	}

	response.write(err);
}

void Root::Manager::registry_set_int_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
		{
			Omega::int64_t val;
			if (!request.read(val))
				err = EIO;
			else
				err = ptrHive->set_integer_value(uKey,strValue,channel_id,val);
		}
	}

	response.write(err);
}

void Root::Manager::registry_set_binary_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->set_binary_value(uKey,strValue,channel_id,request);
	}

	response.write(err);
}

void Root::Manager::registry_set_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strDesc;
		if (!request.read(strDesc))
			err = EIO;
		else
			err = ptrHive->set_description(uKey,channel_id,strDesc);
	}

	response.write(err);
}

void Root::Manager::registry_set_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
		{
			std::string strDesc;
			if (!request.read(strDesc))
				err = EIO;
			else
				err = ptrHive->set_value_description(uKey,strValue,channel_id,strDesc);
		}
	}

	response.write(err);
}

void Root::Manager::registry_get_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	std::string val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		err = ptrHive->get_description(uKey,channel_id,val);

	response.write(err);
	if (err == 0)
		response.write(val);
}

void Root::Manager::registry_get_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	std::string val;

	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->get_value_description(uKey,strValue,channel_id,val);
	}

	response.write(err);
	if (err == 0)
		response.write(val);
}

void Root::Manager::registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		ptrHive->enum_values(uKey,channel_id,response);

	if (err != 0)
		response.write(err);
}

void Root::Manager::registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SmartPtr<Registry::Hive> ptrHive;
	Omega::int64_t uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		std::string strValue;
		if (!request.read(strValue))
			err = EIO;
		else
			err = ptrHive->delete_value(uKey,strValue,channel_id);
	}

	response.write(err);
}
