///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOCore/libdb, the Omega Online Core db library.
//
// OOCore/libdb is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore/libdb is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore/libdb.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <OOBase/GlobalNew.h>

#include "RegistryHive.h"

#include <OOBase/Logger.h>
#include <OOBase/CDRStream.h>

#include <stdlib.h>

Db::Hive::Hive(Manager* pManager, const char* db_name) :
		m_pManager(pManager)
{
	m_strdb.assign(db_name);
}

bool Db::Hive::open(int flags)
{
	m_db = new (std::nothrow) Database();
	if (!m_db)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!m_db->open(m_strdb.c_str(),flags))
		return false;
	
	// Now build the prepared statements...
	return (
			prepare_statement(m_InsertKey_Stmt,"INSERT INTO RegistryKeys (Name,Parent) VALUES (?1,?2);") &&
			prepare_statement(m_InsertValue_Stmt,"INSERT INTO RegistryValues (Name,Parent,Value) VALUES (?1,?2,?3);") &&
		
			prepare_statement(m_UpdateValue_Stmt,"UPDATE RegistryValues SET Value = ?3 WHERE Name = ?1 AND Parent = ?2;") &&
			
			prepare_statement(m_CheckKey_Stmt,"SELECT Id, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Id = ?1;") &&
			prepare_statement(m_GetKeyInfo_Stmt,"SELECT Id, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Name = ?1 AND RegistryKeys.Parent = ?2;") && 
			prepare_statement(m_EnumKeyIds_Stmt,"SELECT Id FROM RegistryKeys WHERE Parent = ?1;") &&
			prepare_statement(m_EnumKeys_Stmt,"SELECT Id, RegistryKeys.Name, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Parent = ?1 ORDER BY RegistryKeys.Name;") &&
			prepare_statement(m_EnumValues_Stmt,"SELECT Name FROM RegistryValues WHERE Parent = ?1 ORDER BY Name;") &&
			prepare_statement(m_GetValue_Stmt,"SELECT Value FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;") &&
				
			prepare_statement(m_DeleteKeys_Stmt,"DELETE FROM RegistryKeys WHERE Parent = ?1;") &&
			prepare_statement(m_DeleteKey_Stmt,"DELETE FROM RegistryKeys WHERE Id = ?1;") &&
			prepare_statement(m_DeleteValue_Stmt,"DELETE FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;") );
}

bool Db::Hive::prepare_statement(Statement& stmt, const char* pszSql)
{
	return (stmt.prepare(*m_db,pszSql) == SQLITE_OK);
}

int Db::Hive::check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask)
{
	// Lock must be held first...

	if (uKey == 0)
	{
		access_mask = static_cast<access_rights_t>(Hive::protect_key | Hive::write_check);
		return SQLITE_ROW;
	}

	Resetter resetter(m_CheckKey_Stmt);

	// Bind the key value
	int err = m_CheckKey_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;

	// And run the statement
	err = m_CheckKey_Stmt.step();
	if (err == SQLITE_ROW)
		access_mask = static_cast<access_rights_t>(m_CheckKey_Stmt.column_int(1));
	
	return err;
}

int Db::Hive::get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, access_rights_t& access_mask, OOBase::LocalString& strLink)
{
	// Lock must be held first...
	
	Resetter resetter(m_GetKeyInfo_Stmt);

	int err = 0;

	if (strSubKey != ".links" && strSubKey != ".access")
	{
		// Check for links in .links

		// Bind the search values
		err = m_GetKeyInfo_Stmt.bind_string(1,".links",6);
		if (err != SQLITE_OK)
			return EIO;

		err = m_GetKeyInfo_Stmt.bind_int64(2,uParent);
		if (err != SQLITE_OK)
			return EIO;

		// And run the statement
		err = m_GetKeyInfo_Stmt.step();
		if (err == SQLITE_ROW)
		{
			// We have a .links subkey
			Omega::int64_t uLinksKey = m_GetKeyInfo_Stmt.column_int64(0);

			// Check access
			int acc = 0;
			access_mask = static_cast<access_rights_t>(m_GetKeyInfo_Stmt.column_int(1));
			if (access_mask & Hive::read_check)
			{
				// Read must be checked
				acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
			}

			err = get_value_i(uLinksKey,strSubKey.c_str(),strLink);
			if (err == SQLITE_ROW)
			{
				if (acc != 0)
					return acc;
			}
		}

		if (err != SQLITE_DONE)
			return err;

		// Reset and re-run
		resetter.reset();
	}
	
	// Bind the search values
	err = m_GetKeyInfo_Stmt.bind_string(1,strSubKey.c_str(),strSubKey.length());
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
			
		if (access_mask & Hive::read_check)
		{
			// Read must be checked
			int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
			if (acc != 0)
				return acc;
		}
	}
	
	return err;
}

int Db::Hive::find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id, OOBase::LocalString& strLink)
{
	// Lock must be held first...

	uKey = uParent;

	// Check if the parent still exists
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (uKey == 0 && strSubKey.empty())
		return 0;

	// Drill down looking for the key...
	for (;;)
	{
		Omega::int64_t uSubKey = uKey;

		size_t pos = strSubKey.find('/');
		if (pos == OOBase::LocalString::npos)
			err = get_key_info(uSubKey,uKey,strSubKey,channel_id,access_mask,strLink);
		else
		{
			OOBase::LocalString strFirst;
			err = strFirst.assign(strSubKey.c_str(),pos);
			if (err == 0)
				err = get_key_info(uSubKey,uKey,strFirst,channel_id,access_mask,strLink);
		}
	
		if (err == SQLITE_DONE)
			return ENOENT;
		else if (err != SQLITE_ROW)
			return err;

		if (pos == OOBase::LocalString::npos)
		{
			strSubKey.clear();
			return (strLink.empty() ? 0 : ENOEXEC);
		}
		
		if ((err = strSubKey.assign(strSubKey.c_str() + pos + 1)) != 0)
			return err;

		// Return ENOEXEC if we are a link...
		if (!strLink.empty())
			return ENOEXEC;
	}
}

int Db::Hive::insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask)
{
	Resetter resetter(m_InsertKey_Stmt);
	
	int err = m_InsertKey_Stmt.bind_string(1,pszSubKey,strlen(pszSubKey));
	if (err != SQLITE_OK)
		return EIO;
	
	err = m_InsertKey_Stmt.bind_int64(2,uParent);
	if (err != SQLITE_OK)
		return EIO;

	err = m_InsertKey_Stmt.step();
	if (err == SQLITE_DONE)
		uKey = m_db->last_insert_rowid();

	if (access_mask != 0)
	{
		OOBase::LocalString strAc;
		if ((err = strAc.printf("%u",access_mask)) == 0)
			err = set_value_i(uKey,".access",strAc.c_str());
		
		if (err == 0)
			err = SQLITE_DONE;
	}

	return err;
}

int Db::Hive::create_key(Omega::int64_t uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, Omega::uint16_t flags, Omega::uint32_t channel_id, OOBase::LocalString& strLink)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask = 0;
	int err = find_key(uParent,uKey,strSubKey,access_mask,channel_id,strLink);

	if (flags == 0 /*OpenExisting*/)
		return err;

	if (flags == 1 /*OpenCreate*/ && err != ENOENT)
		return err;

	if (flags == 2 /*CreateNew*/ && err != ENOENT)
		return (err == 0 ? EEXIST : err);

	if (access_mask & Hive::write_check)
	{
		// Write must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// protect_key and protect_values flags are not inherited
	access_mask &= ~(Hive::protect_key | Hive::protect_values);
	
	// Start a transaction..
	Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return EIO;

	// Drill down creating keys...
	Omega::int64_t uSubKey = uKey;
	for (size_t start = 0;;)
	{
		size_t pos = strSubKey.find('/',start);
		if (pos == OOBase::LocalString::npos)
			err = insert_key(uSubKey,uKey,strSubKey.c_str() + start,access_mask);
		else
		{
			OOBase::LocalString strFirst;
			err = strFirst.assign(strSubKey.c_str() + start,pos - start);
			if (err == 0)
				err = insert_key(uSubKey,uKey,strFirst.c_str(),access_mask);
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

int Db::Hive::delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id)
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
		// Write must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (access_mask & Hive::protect_key)
	{
		// Not allowed to delete!
		return EACCES;
	}

	// Recurse down
	OOBase::Stack<Omega::int64_t,OOBase::LocalAllocator> ids;
	{
		Resetter resetter(m_EnumKeyIds_Stmt);
		
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
		Resetter resetter(m_DeleteKeys_Stmt);
		
		err = m_DeleteKeys_Stmt.bind_int64(1,uKey);
		if (err != SQLITE_OK)
			return EIO;

		if (m_DeleteKeys_Stmt.step() != SQLITE_DONE)
			return EIO;
	}

	return 0;
}

int Db::Hive::delete_key(const Omega::int64_t& uParent, OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, OOBase::LocalString& strLink)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Get the start key
	Omega::int64_t uKey = 0;
	
	access_rights_t access_mask;
	int err = find_key(uParent,uKey,strSubKey,access_mask,channel_id,strLink);
	if (err != 0)
		return err;

	Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return EIO;

	err = delete_key_i(uKey,channel_id);
	if (err == 0)
	{
		// Do the delete of this key
		{
			Resetter resetter(m_DeleteKey_Stmt);
			
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

int Db::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys)
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
	int acc = -1;

	if (access_mask & Hive::read_check)
	{
		// Read must be checked
		acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	Resetter resetter(m_EnumKeys_Stmt);
	
	err = m_EnumKeys_Stmt.bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return EIO;
			
	do
	{
		err = m_EnumKeys_Stmt.step();
		if (err == SQLITE_ROW)
		{
			OOBase::String strSubKey;
			const char* v = m_EnumKeys_Stmt.column_text(1);
			if (v)
			{
				int err2 = strSubKey.assign(v);
				if (err2 != 0)
					return err2;
			}

			access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int64(2));
			if (access_mask & Hive::read_check)
			{
				// Read must be checked
				if (acc == -1)
					acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);

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

void Db::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
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
	Omega::int32_t acc = -1;

	if (access_mask & Hive::read_check)
	{
		// Read must be checked
		acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
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

	Resetter resetter(m_EnumKeys_Stmt);
	
	err = m_EnumKeys_Stmt.bind_int64(1,uKey);
	if (err == SQLITE_OK)
	{
		do
		{
			err = m_EnumKeys_Stmt.step();
			if (err == SQLITE_ROW)
			{
				const char* v = m_EnumKeys_Stmt.column_text(1);
							
				access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int64(2));

				if (access_mask & Hive::read_check)
				{
					// Read must be checked
					if (acc == -1)
						acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);

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

int Db::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues)
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
		// Read must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	Resetter resetter(m_EnumValues_Stmt);
	
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

void Db::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
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
		// Read must be checked
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

	Resetter resetter(m_EnumValues_Stmt);
	
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

int Db::Hive::delete_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::protect_values)
	{
		// Not allowed to delete!
		return EACCES;
	}
	else if (access_mask & Hive::write_check)
	{
		// Write must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}
	
	Resetter resetter(m_DeleteValue_Stmt);
	
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

int Db::Hive::value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id)
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
		// Read must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	return value_exists_i(uKey,pszValue);
}

int Db::Hive::value_exists_i(const Omega::int64_t& uKey, const char* pszValue)
{
	// Lock must be held first...

	Resetter resetter(m_GetValue_Stmt);

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

int Db::Hive::get_value_i(const Omega::int64_t& uKey, const char* pszValue, OOBase::LocalString& val)
{
	// Lock must be held first...

	Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	int err = m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue));
	if (err != SQLITE_OK)
		return err;

	err = m_GetValue_Stmt.bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return err;

	err = m_GetValue_Stmt.step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_GetValue_Stmt.column_text(0);
		if (v)
		{
			err = val.assign(v);
			if (err == 0)
				err = SQLITE_ROW;
		}
	}

	return err;
}

int Db::Hive::get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_ROW)
	{
		if (access_mask & Hive::read_check)
		{
			// Read must be checked
			int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
			if (acc != 0)
				return acc;
		}

		err = get_value_i(uKey,pszValue,val);
	}

	if (err == SQLITE_ROW)
		return 0;
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Db::Hive::set_value_i(const Omega::int64_t& uKey, const char* pszName, const char* pszValue)
{
	// Lock must be held first...

	// See if we have a value already
	Statement* pStmt = NULL;
	int err = value_exists_i(uKey,pszName);
	if (err == 0)
		pStmt = &m_UpdateValue_Stmt;
	else if (err == ENOENT)
		pStmt = &m_InsertValue_Stmt;
		
	if (pStmt)
	{
		Resetter resetter(*pStmt);
			
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

int Db::Hive::set_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id, const char* pszValue)
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
		// Write must be checked
		int acc = m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask);
		if (acc != 0)
			return acc;
	}
	
	return set_value_i(uKey,pszName,pszValue);
}
