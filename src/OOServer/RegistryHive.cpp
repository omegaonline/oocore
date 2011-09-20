///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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
#include "RegistryHive.h"

Registry::Hive::Hive(Manager* pManager, const char* db_name) :
		m_pManager(pManager)
{
	m_strdb.assign(db_name);
}

bool Registry::Hive::open(int flags)
{
	m_db = new (std::nothrow) OOSvrBase::Db::Database();
	if (!m_db)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!m_db->open(m_strdb.c_str(),flags))
		return false;
	
	// Now build the prepared statements...
	return (
			prepare_statement(m_InsertKey_Stmt,"INSERT INTO RegistryKeys (Name,Parent,Access) VALUES (?1,?2,?3);") &&
			prepare_statement(m_InsertValue_Stmt,"INSERT INTO RegistryValues (Name,Parent,Value) VALUES (?1,?2,?3);") &&
		
			prepare_statement(m_UpdateValue_Stmt,"UPDATE RegistryValues SET Value = ?3 WHERE Name = ?1 AND Parent = ?2;") &&
			prepare_statement(m_UpdateDesc_Stmt,"UPDATE RegistryKeys SET Description = ?1 WHERE Id = ?2;") &&
			prepare_statement(m_UpdateValueDesc_Stmt,"UPDATE RegistryValues SET Description = ?3 WHERE Name = ?1 AND Parent = ?2;") &&
	
			prepare_statement(m_CheckKey_Stmt,"SELECT Access, Description FROM RegistryKeys WHERE Id = ?1;") &&
			prepare_statement(m_GetKeyInfo_Stmt,"SELECT Id, Access FROM RegistryKeys WHERE Name = ?1 AND Parent = ?2;") && 
			prepare_statement(m_EnumKeyIds_Stmt,"SELECT Id FROM RegistryKeys WHERE Parent = ?1;") &&
			prepare_statement(m_EnumKeys_Stmt,"SELECT Name, Access FROM RegistryKeys WHERE Parent = ?1 ORDER BY Name;") &&
			prepare_statement(m_EnumValues_Stmt,"SELECT Name FROM RegistryValues WHERE Parent = ?1 ORDER BY Name;") &&
			prepare_statement(m_GetValue_Stmt,"SELECT Value,Description FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;") &&
				
			prepare_statement(m_DeleteKeys_Stmt,"DELETE FROM RegistryKeys WHERE Parent = ?1;") &&
			prepare_statement(m_DeleteKey_Stmt,"DELETE FROM RegistryKeys WHERE Id = ?1;") &&
			prepare_statement(m_DeleteValue_Stmt,"DELETE FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;") );
}

bool Registry::Hive::prepare_statement(OOSvrBase::Db::Statement& stmt, const char* pszSql)
{
	return (stmt.prepare(*m_db,pszSql) == SQLITE_OK);
}

int Registry::Hive::check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask)
{
	// Lock must be held first...
	if (uKey == 0)
	{
		access_mask = static_cast<access_rights_t>(Hive::never_delete | Hive::write_check);
		return SQLITE_ROW;
	}

	OOSvrBase::Db::Resetter resetter(m_CheckKey_Stmt);

	// Bind the key value
	int err = m_CheckKey_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;

	// And run the statement
	err = m_CheckKey_Stmt.step();
	if (err == SQLITE_ROW)
		access_mask = static_cast<access_rights_t>(m_CheckKey_Stmt.column_int(0));

	return err;
}

int Registry::Hive::get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t& access_mask)
{
	// Lock must be held first...

	OOSvrBase::Db::Resetter resetter(m_GetKeyInfo_Stmt);

	// Bind the search values
	int err = m_GetKeyInfo_Stmt.bind_string(1,pszSubKey,strlen(pszSubKey));
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetKeyInfo_Stmt.bind_int64(2,uParent);
	if (err != SQLITE_OK)
		return EIO;

	// And run the statement
	err = m_GetKeyInfo_Stmt.step();
	if (err == SQLITE_ROW)
	{
		uKey = m_GetKeyInfo_Stmt.column_int64(0);
		access_mask = static_cast<access_rights_t>(m_GetKeyInfo_Stmt.column_int(1));
	}

	return err;
}

int Registry::Hive::find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id)
{
	// Lock must be held first...
	
	int err = strSubKey.assign(pszKey);
	if (err != 0)
		return err;

	// Check for root key
	if (uParent==0 && strSubKey.empty())
	{
		access_mask = static_cast<access_rights_t>(Hive::never_delete | Hive::write_check);
		uKey = 0;
		return 0;
	}

	// Check if the key still exists
	err = check_key_exists(uParent,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Drill down looking for the key...
	uKey = uParent;
	Omega::int64_t uSubKey = uParent;
	for (;;)
	{
		size_t pos = strSubKey.find('/');
		if (pos == OOBase::LocalString::npos)
			err = get_key_info(uSubKey,uKey,strSubKey.c_str(),access_mask);
		else
		{
			OOBase::LocalString strFirst;
			err = strFirst.assign(strSubKey.c_str(),pos);
			if (err == 0)
				err = get_key_info(uSubKey,uKey,strFirst.c_str(),access_mask);
		}
	
		if (err == SQLITE_DONE)
			return ENOENT;
		else if (err != SQLITE_ROW)
			return EIO;

		if (access_mask & Hive::read_check)
		{
			// Read not allowed - check access!
			int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
			if (acc != 0)
				return acc;
		}

		if (pos == OOBase::LocalString::npos)
		{
			strSubKey.clear();
			return 0;
		}

		uSubKey = uKey;

		if ((err = strSubKey.assign(strSubKey.c_str() + pos + 1)) != 0)
			return err;
	}
}

int Registry::Hive::insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask)
{
	OOSvrBase::Db::Resetter resetter(m_InsertKey_Stmt);
	
	int err = m_InsertKey_Stmt.bind_string(1,pszSubKey,strlen(pszSubKey));
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_InsertKey_Stmt.bind_int64(2,uParent);
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_InsertKey_Stmt.bind_int64(3,access_mask);
	if (err != SQLITE_OK)
		return EIO;

	err = m_InsertKey_Stmt.step();
	if (err == SQLITE_DONE)
		uKey = m_db->last_insert_rowid();

	return err;
}

int Registry::Hive::open_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Find the key
	access_rights_t access_mask = 0;
	OOBase::LocalString strSubKey;
	return find_key(uParent,uKey,pszSubKey,strSubKey,access_mask,channel_id);
}

int Registry::Hive::create_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint16_t flags, access_rights_t access, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask = 0;
	OOBase::LocalString strSubKey;
	int err = find_key(uParent,uKey,pszSubKey,strSubKey,access_mask,channel_id);

	if (flags == 0 /*OpenExisting*/)
		return err;

	if (flags == 1 /*OpenCreate*/ && err != ENOENT)
		return err;

	if (flags == 2 /*CreateNew*/ && err != ENOENT)
		return (err == 0 ? EEXIST : err);

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Mask the access mask
	if (access & Hive::inherit_checks)
	{
		// Remove the inherit flag and the never_delete flag
		access = (access_mask & ~(Hive::inherit_checks | Hive::never_delete));
	}

	// Start a transaction..
	OOSvrBase::Db::Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return EIO;

	// Drill down creating keys...
	Omega::int64_t uSubKey = uKey;
	for (size_t start = 0;;)
	{
		size_t pos = strSubKey.find('/',start);
		if (pos == OOBase::LocalString::npos)
			err = insert_key(uSubKey,uKey,strSubKey.c_str() + start,access);
		else
		{
			OOBase::LocalString strFirst;
			err = strFirst.assign(strSubKey.c_str() + start,pos - start);
			if (err == 0)
				err = insert_key(uSubKey,uKey,strFirst.c_str(),access);
		}

		if (err == SQLITE_READONLY)
			return EACCES;
		else if (err != SQLITE_DONE)
			return EIO;

		if (pos == OOBase::LocalString::npos)
			break;

		start = pos + 1;
		uSubKey = uKey;
	}
	
	if (trans.commit() != SQLITE_OK)
		return EIO;

	return 0;
}

int Registry::Hive::delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id)
{
	// This one is recursive, within a transaction and a lock...

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return 0;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (access_mask & Hive::never_delete)
	{
		// Never allowed to delete!
		return EACCES;
	}

	// Recurse down
	OOBase::Stack<Omega::int64_t,OOBase::LocalAllocator> ids;
	{
		OOSvrBase::Db::Resetter resetter(m_EnumKeyIds_Stmt);
		
		err = m_EnumKeyIds_Stmt.bind_int64(1,uKey);
		if (err != SQLITE_OK)
			return EIO;
				
		do
		{
			err = m_EnumKeyIds_Stmt.step();
			if (err == SQLITE_ROW)
			{
				// Add to stack
				int res = ids.push(m_EnumKeyIds_Stmt.column_int64(0));
				if (res != 0)
					LOG_ERROR_RETURN(("Failed to stack push: %s",OOBase::system_error_text(res)),res);
			}
		}
		while (err == SQLITE_ROW);
		
		if (err != SQLITE_DONE)
			return EIO;
	}
	
	if (!ids.empty())
	{
		for (Omega::int64_t id;ids.pop(&id);)
		{
			int res = delete_key_i(id,channel_id);
			if (res != 0)
				return res;
		}
	
		// Do the delete
		OOSvrBase::Db::Resetter resetter(m_DeleteKeys_Stmt);
		
		err = m_DeleteKeys_Stmt.bind_int64(1,uKey);
		if (err != SQLITE_OK)
			return EIO;

		if (m_DeleteKeys_Stmt.step() != SQLITE_DONE)
			return EIO;
	}

	return 0;
}

int Registry::Hive::delete_key(const Omega::int64_t& uParent, const char* pszSubKey, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Get the start key
	Omega::int64_t uKey = 0;
	access_rights_t access_mask;
	OOBase::LocalString strSubKey;
	int err = find_key(uParent,uKey,pszSubKey,strSubKey,access_mask,channel_id);
	if (err != 0)
		return err;

	OOSvrBase::Db::Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return EIO;

	err = delete_key_i(uKey,channel_id);
	if (err == 0)
	{
		// Do the delete of this key
		{
			OOSvrBase::Db::Resetter resetter(m_DeleteKey_Stmt);
			
			err = m_DeleteKey_Stmt.bind_int64(1,uKey);
			if (err != SQLITE_OK)
				return EIO;
					
			if (m_DeleteKey_Stmt.step() != SQLITE_DONE)
				return EIO;
		}
			
		if (trans.commit() != SQLITE_OK)
			return EIO;
	}

	return err;
}

int Registry::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	// Do the access check up front...
	int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		if (acc != 0)
			return acc;
	}

	OOSvrBase::Db::Resetter resetter(m_EnumKeys_Stmt);
	
	err = m_EnumKeys_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;
			
	do
	{
		err = m_EnumKeys_Stmt.step();
		if (err == SQLITE_ROW)
		{
			OOBase::String strSubKey;
			const char* v = m_EnumKeys_Stmt.column_text(0);
			if (v)
			{
				int err2 = strSubKey.assign(v);
				if (err2 != 0)
					return err2;
			}

			access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int(1));
			if (access_mask & Hive::read_check)
			{
				// Read not allowed - check access!
				if (acc != 0)
					strSubKey.clear();
			}

			if (!strSubKey.empty())
			{
				int err2 = setSubKeys.push(strSubKey);
				if (err2 != 0)
					return err2;
			}
		}
	}
	while (err == SQLITE_ROW);
	
	return (err == SQLITE_DONE ? 0 : EIO);
}

void Registry::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response.write(Omega::int32_t(ENOENT));
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response.write(Omega::int32_t(EIO));
		return;
	}

	// Do the access check up front...
	Omega::int32_t acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	// Write out success first
	response.write(Omega::int32_t(0));
	if (response.last_error() != 0)
		return;

	OOSvrBase::Db::Resetter resetter(m_EnumKeys_Stmt);
	
	err = m_EnumKeys_Stmt.bind_int64(1,uKey);
	if (err == SQLITE_OK)
	{
		do
		{
			err = m_EnumKeys_Stmt.step();
			if (err == SQLITE_ROW)
			{
				const char* v = m_EnumKeys_Stmt.column_text(0);
							
				access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int(1));
				if (access_mask & Hive::read_check)
				{
					// Read not allowed - check access!
					if (acc != 0)
						v = NULL;
				}

				if (v && !response.write(v))
				{
					response.reset();
					response.write(Omega::int32_t(EIO));
					return;
				}
			}
		}
		while (err == SQLITE_ROW);
	}

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write("");
	}
	else
	{
		response.reset();
		response.write(Omega::int32_t(EIO));
	}
}

int Registry::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOSvrBase::Db::Resetter resetter(m_EnumValues_Stmt);
	
	err = m_EnumValues_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;
	
	do
	{
		err = m_EnumValues_Stmt.step();
		if (err == SQLITE_ROW)
		{
			OOBase::String val;
			int err2 = val.assign(m_EnumValues_Stmt.column_text(0));
			if (err2 == 0 && !val.empty())
				err2 = setValues.push(val);

			if (err2 != 0)
				return err2;
		}
	}
	while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Registry::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response.write(Omega::int32_t(ENOENT));
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response.write(Omega::int32_t(EIO));
		return;
	}

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		Omega::int32_t acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	// Write out success first
	response.write(Omega::int32_t(0));
	if (response.last_error() != 0)
		return;

	OOSvrBase::Db::Resetter resetter(m_EnumValues_Stmt);
	
	err = m_EnumValues_Stmt.bind_int64(1,uKey);
	if (err == SQLITE_OK)
	{
		do
		{
			err = m_EnumValues_Stmt.step();
			if (err == SQLITE_ROW)
			{
				const char* v = m_EnumValues_Stmt.column_text(0);
				if (v && !response.write(v))
				{
					response.reset();
					response.write(Omega::int32_t(EIO));
					return;
				}
			}
		}
		while (err == SQLITE_ROW);
	}

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write("");
	}
	else
	{
		response.reset();
		response.write(Omega::int32_t(EIO));
	}
}

int Registry::Hive::delete_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}
	
	OOSvrBase::Db::Resetter resetter(m_DeleteValue_Stmt);
	
	err = m_DeleteValue_Stmt.bind_string(1,pszName,strlen(pszName));
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_DeleteValue_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = m_DeleteValue_Stmt.step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	return value_exists_i(uKey,pszValue);
}

int Registry::Hive::value_exists_i(const Omega::int64_t& uKey, const char* pszValue)
{
	OOSvrBase::Db::Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	int err = m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue));
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.step();
	if (err == SQLITE_ROW)
		return 0;
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Registry::Hive::get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOSvrBase::Db::Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	err = m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue));
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_GetValue_Stmt.column_text(0);
		if (v)
			err = val.assign(v);
			
		return (v ? err : 0);
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Registry::Hive::set_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id, const char* pszValue)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}
	
	// See if we have a value already
	OOSvrBase::Db::Statement* pStmt = NULL;
	err = value_exists_i(uKey,pszName);
	if (err == 0)
		pStmt = &m_UpdateValue_Stmt;
	else if (err == ENOENT)
		pStmt = &m_InsertValue_Stmt;
		
	if (pStmt)
	{
		OOSvrBase::Db::Resetter resetter(*pStmt);
			
		err = pStmt->bind_string(1,pszName,strlen(pszName));
		if (err != SQLITE_OK)
			return EIO;
		
		err = pStmt->bind_int64(2,uKey);
		if (err != SQLITE_OK)
			return EIO;
		
		err = pStmt->bind_string(3,pszValue,strlen(pszValue));
		if (err != SQLITE_OK)
			return EIO;
		
		err = pStmt->step();
	}
		
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::LocalString& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOSvrBase::Db::Resetter resetter(m_CheckKey_Stmt);

	// Bind the key value
	err = m_CheckKey_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = m_CheckKey_Stmt.step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_CheckKey_Stmt.column_text(1);
		if (v)
			err = val.assign(v);
			
		return (v ? err : 0);
	}
	else if (err == SQLITE_DONE)
		return 0;
	else
		return EIO;
}

int Registry::Hive::get_value_description(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOSvrBase::Db::Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	err = m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue));
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = m_GetValue_Stmt.step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_GetValue_Stmt.column_text(1);
		if (v)
			err = val.assign(v);
			
		return (v ? err : 0);
	}
	else if (err == SQLITE_DONE)
	{
		// See if the value exists at all...
		return value_exists_i(uKey,pszValue);
	}
	else
		return EIO;
}

int Registry::Hive::set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const char* val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}
	
	OOSvrBase::Db::Resetter resetter(m_UpdateDesc_Stmt);
	
	err = m_UpdateDesc_Stmt.bind_string(1,val,strlen(val));
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_UpdateDesc_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;

	// Insert the new value
	err = m_UpdateDesc_Stmt.step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::set_value_description(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id, const char* pszValue)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Check the value exists...
	if ((err = value_exists_i(uKey,pszName)) != 0)
		return err;
	
	OOSvrBase::Db::Resetter resetter(m_UpdateValueDesc_Stmt);
	
	err = m_UpdateValueDesc_Stmt.bind_string(1,pszName,strlen(pszName));
	if (err != SQLITE_OK)
		return EIO;

	err = m_UpdateValueDesc_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_UpdateValueDesc_Stmt.bind_string(3,pszValue,strlen(pszValue));
	if (err != SQLITE_OK)
		return EIO;

	// Insert the new value
	err = m_UpdateValueDesc_Stmt.step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}
