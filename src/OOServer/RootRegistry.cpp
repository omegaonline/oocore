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

bool Root::Manager::registry_access_check(const char* pszDb, OOBase::uint32_t channel_id, Db::access_rights_t access_mask, int& err)
{
	// Zero channel always has access...
	if (channel_id == 0)
	{
		err = 0;
		return true;
	}

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	// Find the process info
	/*const UserProcess* pU = m_mapUserProcesses.find(channel_id);
	if (!pU)
	{
		err = EINVAL;
		return false;
	}

	bool bRead = (access_mask & Db::read_check) == Db::read_check;
	bool bWrite = (access_mask & Db::write_check) == Db::write_check;

	// Check access
	bool bAllowed = false;
	err = pU->m_ptrProcess->CheckAccess(pszDb,bRead,bWrite,bAllowed);
	return bAllowed;*/
	return false;
}

OOServer::RootErrCode_t Root::Manager::registry_open_hive(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::SharedPtr<Db::Hive>& ptrHive, OOBase::int64_t& uKey, OOBase::uint8_t& nType)
{
	// Read uKey && nType
	if (!request.read(uKey) || !request.read(nType))
		LOG_ERROR_RETURN(("Failed to read registry request parameters: %s",OOBase::system_error_text(request.last_error())),OOServer::RootErrCode_t(OOServer::Errored));

	switch (nType)
	{
	case 0:
		// Return the system hive
		ptrHive = m_registry;
		break;

	case 1:
		{
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			// Find the process info
	//		UserProcess* pU = m_mapUserProcesses.find(channel_id);
	//		if (!pU)
				return OOServer::NoRead;

			// Get the registry hive
	//		ptrHive = pU->m_ptrRegistry;
		}
		break;

	default:
		// What?!?
		LOG_ERROR_RETURN(("Invalid hive type %u received",nType),OOServer::RootErrCode_t(OOServer::Errored));
	}

	return OOServer::Ok;
}

Db::hive_errors Root::Manager::registry_open_key(OOBase::int64_t& uKey, const OOBase::LocalString& strSubKey, OOBase::uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::LocalString strSubKey2(strSubKey),strLink(strSubKey.get_allocator()),strFullKeyName(strSubKey.get_allocator());
	return m_registry->create_key(0,uKey,strSubKey2,0,channel_id,strLink,strFullKeyName);
}

OOServer::RootErrCode_t Root::Manager::registry_open_link(OOBase::uint32_t channel_id, const OOBase::LocalString& strLink, OOBase::LocalString& strSubKey, OOBase::uint8_t& nType, OOBase::SharedPtr<Db::Hive>& ptrHive)
{
	if (nType == 0 && strncmp(strLink.c_str(),"system:user/",12) == 0)
	{
		/*if (channel_id == m_sandbox_channel)
		{
			// We link to /System/Sandbox/

			OOBase::LocalString strNew(strLink.get_allocator());
			int err = strNew.concat("/System/Sandbox",strSubKey.c_str());
			if (!err)
				err = strSubKey.assign(strNew.c_str());

			if (err)
				LOG_ERROR_RETURN(("Failed to concatenate strings: %s",OOBase::system_error_text(err)),OOServer::RootErrCode_t(OOServer::Errored));
		}
		else*/
		{
			OOBase::LocalString strNew(strLink.get_allocator());
			int err = strNew.concat(strLink.c_str()+12,strSubKey.c_str());
			if (!err)
				err = strSubKey.assign(strNew.c_str());
			if (err)
				LOG_ERROR_RETURN(("Failed to concatenate strings: %s",OOBase::system_error_text(err)),OOServer::RootErrCode_t(OOServer::Errored));

			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			// Find the process info
			/*UserProcess* pU = m_mapUserProcesses.find(channel_id);
			if (!pU)*/
				return OOServer::NoRead;

			nType = 1;
			//ptrHive = pU->m_ptrRegistry;
		}

		return OOServer::Ok;
	}

	// If it's not known, it's not found
	return (strncmp(strLink.c_str(),"system:",7) == 0 ? OOServer::RootErrCode_t(OOServer::NotFound) : OOServer::RootErrCode_t(OOServer::Linked));
}

void Root::Manager::registry_open_key(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey = 0;
	OOBase::uint8_t nType;
	OOBase::int64_t uSubKey;
	OOBase::StackAllocator<512> allocator;
	OOBase::LocalString strLink(allocator);
	OOBase::LocalString strSubKey(allocator);
	OOBase::LocalString strFullKeyName(allocator);

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (!err)
	{
		if (!request.read_string(strSubKey))
		{
			LOG_ERROR(("Failed to read key name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			OOBase::uint16_t flags = 0;
			if (!request.read(flags))
			{
				LOG_ERROR(("Failed to read open flags from request: %s",OOBase::system_error_text(request.last_error())));
				err = OOServer::Errored;
			}
			else
			{
				err = OOServer::RootErrCode_t(ptrHive->create_key(uKey,uSubKey,strSubKey,flags,channel_id,strLink,strFullKeyName));
				if (err == Db::HIVE_LINK)
				{
					err = registry_open_link(channel_id,strLink,strSubKey,nType,ptrHive);
					if (!err)
					{
						strLink.clear();
						err = OOServer::RootErrCode_t(ptrHive->create_key(0,uSubKey,strSubKey,flags,channel_id,strLink,strFullKeyName));
					}
				}
			}
		}
	}

	response.write(err);
	if (err != Db::HIVE_ERRORED)
	{
		response.write_string(strFullKeyName);

		if (err == Db::HIVE_LINK)
		{
			response.write_string(strLink);
			response.write_string(strSubKey);
		}
		else if (err == Db::HIVE_OK)
		{
			response.write(uSubKey);
			response.write(nType);
		}
	}

	if (response.last_error() != 0)
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void Root::Manager::registry_delete_key(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;
	OOBase::StackAllocator<512> allocator;
	OOBase::LocalString strLink(allocator);
	OOBase::LocalString strSubKey(allocator);
	OOBase::LocalString strFullKeyName(allocator);

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (!err)
	{
		if (!request.read_string(strSubKey))
		{
			LOG_ERROR(("Failed to read key name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			err = OOServer::RootErrCode_t(ptrHive->delete_key(uKey,strSubKey,channel_id,strLink,strFullKeyName));
			if (err == Db::HIVE_LINK)
			{
				err = registry_open_link(channel_id,strLink,strSubKey,nType,ptrHive);
				if (!err)
				{
					strLink.clear();
					err = OOServer::RootErrCode_t(ptrHive->delete_key(uKey,strSubKey,channel_id,strLink,strFullKeyName));
				}
			}
		}
	}

	response.write(err);
	if (err != Db::HIVE_ERRORED)
	{
		response.write_string(strFullKeyName);

		if (err == Db::HIVE_LINK)
		{
			response.write_string(strLink);
			response.write_string(strSubKey);
		}
	}

	if (response.last_error() != 0)
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void Root::Manager::registry_enum_subkeys(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err)
	{
		if (!response.write(err))
			LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
	}
	else
		ptrHive->enum_subkeys(uKey,channel_id,response);
}

void Root::Manager::registry_value_exists(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err == 0)
	{
		OOBase::StackAllocator<512> allocator;
		OOBase::LocalString strValue(allocator);
		if (!request.read_string(strValue))
		{
			LOG_ERROR(("Failed to read value name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
			err = OOServer::RootErrCode_t(ptrHive->value_exists(uKey,strValue.c_str(),channel_id));
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void Root::Manager::registry_get_value(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::StackAllocator<512> allocator;
	OOBase::LocalString val(allocator);
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (!err)
	{
		OOBase::LocalString strValue(allocator);
		if (!request.read_string(strValue))
		{
			LOG_ERROR(("Failed to read value name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
			err = OOServer::RootErrCode_t(ptrHive->get_value(uKey,strValue.c_str(),channel_id,val));
	}

	response.write(err);
	if (!err)
		response.write_string(val);

	if (response.last_error())
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void Root::Manager::registry_set_value(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (!err)
	{
		OOBase::StackAllocator<512> allocator;
		OOBase::LocalString strValue(allocator);
		if (!request.read_string(strValue))
		{
			LOG_ERROR(("Failed to read value name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
		{
			OOBase::LocalString val(allocator);
			if (!request.read_string(val))
			{
				LOG_ERROR(("Failed to read value data from request: %s",OOBase::system_error_text(request.last_error())));
				err = OOServer::Errored;
			}
			else if (strValue.empty())
				err = OOServer::BadName;
			else
				err = OOServer::RootErrCode_t(ptrHive->set_value(uKey,strValue.c_str(),channel_id,val.c_str()));
		}
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

void Root::Manager::registry_enum_values(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (err)
	{
		if (!response.write(err))
			LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
	}
	else
		ptrHive->enum_values(uKey,channel_id,response);
}

void Root::Manager::registry_delete_value(OOBase::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::SharedPtr<Db::Hive> ptrHive;
	OOBase::int64_t uKey;
	OOBase::uint8_t nType;

	OOServer::RootErrCode_t err = registry_open_hive(channel_id,request,ptrHive,uKey,nType);
	if (!err)
	{
		OOBase::StackAllocator<512> allocator;
		OOBase::LocalString strValue(allocator);
		if (!request.read_string(strValue))
		{
			LOG_ERROR(("Failed to read value name from request: %s",OOBase::system_error_text(request.last_error())));
			err = OOServer::Errored;
		}
		else
			err = OOServer::RootErrCode_t(ptrHive->delete_value(uKey,strValue.c_str(),channel_id));
	}

	if (!response.write(err))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}
