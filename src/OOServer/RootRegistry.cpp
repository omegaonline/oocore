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

#include "./OOServer_Root.h"
#include "./RootManager.h"
#include "./SpawnedProcess.h"

int Root::Manager::registry_access_check(ACE_CDR::ULong channel_id)
{
	Root::Manager* pThis = ROOT_MANAGER::instance();

	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,pThis->m_lock,ACE_OS::last_error());

	// Find the process info
	std::map<ACE_CDR::ULong,UserProcess>::iterator i = pThis->m_mapUserProcesses.find(channel_id);
	if (i == pThis->m_mapUserProcesses.end())
		return EINVAL;

	// Check access
	bool bAllowed = false;
	if (!i->second.pSpawn->CheckAccess(pThis->m_strRegistry.c_str(),O_RDWR,bAllowed))
		return ACE_OS::last_error();
	else if (!bAllowed)
		return EACCES;
	else
		return 0;
}

int Root::Manager::registry_parse_subkey(const ACE_INT64& uKey, ACE_CDR::ULong& channel_id, const ACE_CString& strSubKey, bool& bCurrent, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex>& ptrHive)
{
	int err = 0;
	if (uKey == 0)
	{
		// Parse strKey
		if (strSubKey.substr(0,11) == "Local User\\")
		{
			bCurrent = true;
		}
		else if (strSubKey == "Local User")
		{
			bCurrent = true;
		}

		if (bCurrent)
		{
			ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
			if (guard.locked() == 0)
				err = ACE_OS::last_error();
			else
			{
				// Find the process info
				std::map<ACE_CDR::ULong,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
				if (i == m_mapUserProcesses.end())
					err = EINVAL;
				else
				{
					// Get the registry hive
					ptrHive = i->second.ptrRegistry;

					// Clear channel id -  not used for user content
					channel_id = 0;
				}
			}
		}
	}

	return err;
}

int Root::Manager::registry_open_hive(ACE_CDR::ULong& channel_id, ACE_InputCDR& request, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex>& ptrHive, ACE_INT64& uKey, bool& bCurrent)
{
	// Read uKey
	if (!request.read_longlong(uKey))
		return ACE_OS::last_error();

	if (uKey < 0)
	{
		bCurrent = true;
		uKey = -uKey;

		ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

		// Find the process info
		std::map<ACE_CDR::ULong,UserProcess>::iterator i = m_mapUserProcesses.find(channel_id);
		if (i == m_mapUserProcesses.end())
			return EINVAL;

		// Get the registry hive
		ptrHive = i->second.ptrRegistry;

		// Clear channel id -  not used for user content
		channel_id = 0;
	}
	else
	{
		// Return the system hive
		bCurrent = false;
		ptrHive = m_registry;
	}

	return 0;
}

int Root::Manager::registry_open_hive(ACE_CDR::ULong& channel_id, ACE_InputCDR& request, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex>& ptrHive, ACE_INT64& uKey)
{
	bool bCurrent;
	return registry_open_hive(channel_id,request,ptrHive,uKey,bCurrent);
}

void Root::Manager::registry_key_exists(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	bool bCurrent = false;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,bCurrent);
	if (err == 0)
	{
		ACE_CString strSubKey;
		if (!request.read_string(strSubKey))
			err = ACE_OS::last_error();
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,bCurrent,ptrHive);
			if (err == 0)
				err = ptrHive->open_key(uKey,strSubKey,channel_id);
		}
	}

	response << err;
}

void Root::Manager::registry_create_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey = 0;
	bool bCurrent = false;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,bCurrent);
	if (err == 0)
	{
		ACE_CString strSubKey;
		if (!request.read_string(strSubKey))
			err = ACE_OS::last_error();
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,bCurrent,ptrHive);
			if (err == 0)
			{
				ACE_CDR::Boolean bCreate = false;
				if (!request.read_boolean(bCreate))
					err = ACE_OS::last_error();
				else
				{
					if (bCurrent && strSubKey == "Local User")
					{
						// Always create the Local User key...
						bCreate = true;
					}

					ACE_CDR::Boolean bFailIfThere = false;
					if (!request.read_boolean(bFailIfThere))
						err = ACE_OS::last_error();
					else
					{
						if (bCreate)
							err = ptrHive->create_key(uKey,strSubKey,bFailIfThere,8,channel_id);
						else
							err = ptrHive->open_key(uKey,strSubKey,channel_id);

						if (err == 0 && bCurrent)
							uKey = -uKey;
					}
				}
			}
		}
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_longlong(uKey);
}

void Root::Manager::registry_delete_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	bool bCurrent = false;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey,bCurrent);
	if (err == 0)
	{
		ACE_CString strSubKey;
		if (!request.read_string(strSubKey))
			err = ACE_OS::last_error();
		else
		{
			err = registry_parse_subkey(uKey,channel_id,strSubKey,bCurrent,ptrHive);
			if (err == 0)
				err = ptrHive->delete_key(uKey,strSubKey,channel_id);
		}
	}

	response << err;
}

void Root::Manager::registry_enum_subkeys(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		ptrHive->enum_subkeys(uKey,channel_id,response);

	if (err != 0)
		response << err;
}

void Root::Manager::registry_value_type(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::Octet type = 0;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_value_type(uKey,strValue,channel_id,type);
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_octet(type);
}

void Root::Manager::registry_get_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CString val;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_string_value(uKey,strValue,channel_id,val);
	}

	response << err;
	if (err == 0)
		response.write_string(val);
}

void Root::Manager::registry_get_int_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::LongLong val = 0;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_integer_value(uKey,strValue,channel_id,val);
	}

	response << err;
	if (err == 0 && response.good_bit())
		response.write_longlong(val);
}

void Root::Manager::registry_get_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::ULong cbLen = 0;
			if (!request.read_ulong(cbLen))
				err = ACE_OS::last_error();
			else
				ptrHive->get_binary_value(uKey,strValue,cbLen,channel_id,response);
		}
	}

	if (err != 0)
		response << err;
}

void Root::Manager::registry_set_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CString val;
			if (!request.read_string(val))
				err = ACE_OS::last_error();
			else
				err = ptrHive->set_string_value(uKey,strValue,channel_id,val.c_str());
		}
	}

	response << err;
}

void Root::Manager::registry_set_int_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CDR::LongLong val;
			if (!request.read_longlong(val))
				err = ACE_OS::last_error();
			else
				err = ptrHive->set_integer_value(uKey,strValue,channel_id,val);
		}
	}

	response << err;
}

void Root::Manager::registry_set_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->set_binary_value(uKey,strValue,channel_id,request);
	}

	response << err;
}

void Root::Manager::registry_set_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strDesc;
		if (!request.read_string(strDesc))
			err = ACE_OS::last_error();
		else
			err = ptrHive->set_description(uKey,channel_id,strDesc);
	}

	response << err;
}

void Root::Manager::registry_set_value_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
		{
			ACE_CString strDesc;
			if (!request.read_string(strDesc))
				err = ACE_OS::last_error();
			else
				err = ptrHive->set_value_description(uKey,strValue,channel_id,strDesc);
		}
	}

	response << err;
}

void Root::Manager::registry_get_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CString val;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		err = ptrHive->get_description(uKey,channel_id,val);

	response << err;
	if (err == 0)
		response.write_string(val);
}

void Root::Manager::registry_get_value_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CString val;

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->get_value_description(uKey,strValue,channel_id,val);
	}

	response << err;
	if (err == 0)
		response.write_string(val);
}

void Root::Manager::registry_enum_values(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
		ptrHive->enum_values(uKey,channel_id,response);

	if (err != 0)
		response << err;
}

void Root::Manager::registry_delete_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> ptrHive;
	ACE_INT64 uKey;
	int err = registry_open_hive(channel_id,request,ptrHive,uKey);
	if (err == 0)
	{
		ACE_CString strValue;
		if (!request.read_string(strValue))
			err = ACE_OS::last_error();
		else
			err = ptrHive->delete_value(uKey,strValue,channel_id);
	}

	response << err;
}
