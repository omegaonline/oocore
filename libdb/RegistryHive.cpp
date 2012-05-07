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
		LOG_ERROR_RETURN(("Failed to allocate database: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	int err = m_db->open(m_strdb.c_str(),flags);
	if (err != SQLITE_OK)
		return false;

	try
	{
		// Now build the prepared statements...
		prepare_statement(m_InsertKey_Stmt,"INSERT INTO RegistryKeys (Name,Parent) VALUES (?1,?2);");
		prepare_statement(m_InsertValue_Stmt,"INSERT INTO RegistryValues (Name,Parent,Value) VALUES (?1,?2,?3);");
	
		prepare_statement(m_UpdateValue_Stmt,"UPDATE RegistryValues SET Value = ?3 WHERE Name = ?1 AND Parent = ?2;");
		
		prepare_statement(m_CheckKey_Stmt,"SELECT Id, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Id = ?1;");
		prepare_statement(m_GetKeyInfo_Stmt,"SELECT Id, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Name = ?1 AND RegistryKeys.Parent = ?2;");
		prepare_statement(m_EnumKeyIds_Stmt,"SELECT Id FROM RegistryKeys WHERE Parent = ?1;");
		prepare_statement(m_EnumKeys_Stmt,"SELECT Id, RegistryKeys.Name, Value FROM RegistryKeys LEFT OUTER JOIN RegistryValues ON (RegistryValues.Parent = RegistryKeys.Id AND RegistryValues.Name = '.access') WHERE RegistryKeys.Parent = ?1 ORDER BY RegistryKeys.Name;");
		prepare_statement(m_EnumValues_Stmt,"SELECT Name FROM RegistryValues WHERE Parent = ?1 ORDER BY Name;");
		prepare_statement(m_GetValue_Stmt,"SELECT Value FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;");
			
		prepare_statement(m_DeleteKeys_Stmt,"DELETE FROM RegistryKeys WHERE Parent = ?1;");
		prepare_statement(m_DeleteKey_Stmt,"DELETE FROM RegistryKeys WHERE Id = ?1;");
		prepare_statement(m_DeleteValue_Stmt,"DELETE FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;");
	}
	catch (int&)
	{
		return false;
	}

	return true;
}

void Db::Hive::prepare_statement(Statement& stmt, const char* pszSql)
{
	int err = stmt.prepare(*m_db,pszSql);
	if (err != SQLITE_OK)
		throw err;
}

Db::hive_errors Db::Hive::access_check(Omega::uint32_t channel_id, access_rights_t access_mask, access_rights_t check)
{
	if (access_mask & check)
	{
		int err = 0;
		if (!m_pManager->registry_access_check(m_strdb.c_str(),channel_id,access_mask & (Db::read_check | Db::write_check),err))
		{
			if (err)
				LOG_ERROR(("Access check call failed: %s",OOBase::system_error_text(err)));

			if (check & Db::write_check)
				return HIVE_NOWRITE;

			return HIVE_NOREAD;
		}
	}

	return HIVE_OK;
}

Db::hive_errors Db::Hive::check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask)
{
	// Lock must be held first...

	if (uKey == 0)
	{
		access_mask = static_cast<access_rights_t>(Db::protect_key | Db::write_check);
		return HIVE_OK;
	}

	Resetter resetter(m_CheckKey_Stmt);

	// Bind the key value
	if (m_CheckKey_Stmt.bind_int64(1,uKey) != SQLITE_OK)
		return HIVE_ERRORED;

	// And run the statement
	switch (m_CheckKey_Stmt.step())
	{
	case SQLITE_ROW:
		access_mask = static_cast<access_rights_t>(m_CheckKey_Stmt.column_int(1));
		return HIVE_OK;

	case SQLITE_DONE:
		return HIVE_NOTFOUND;

	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, access_rights_t& access_mask, OOBase::LocalString& strLink)
{
	// Lock must be held first...
	
	Resetter resetter(m_GetKeyInfo_Stmt);

	if (strSubKey != ".links" && strSubKey != ".access")
	{
		// Check for links in .links

		// Bind the search values
		if (m_GetKeyInfo_Stmt.bind_string(1,".links",6) != SQLITE_OK ||
				m_GetKeyInfo_Stmt.bind_int64(2,uParent) != SQLITE_OK)
		{
			return HIVE_ERRORED;
		}

		// And run the statement
		switch (m_GetKeyInfo_Stmt.step())
		{
		case SQLITE_ROW:
			{
				// We have a .links subkey
				Omega::int64_t uLinksKey = m_GetKeyInfo_Stmt.column_int64(0);

				// Check access
				access_mask = static_cast<access_rights_t>(m_GetKeyInfo_Stmt.column_int(1));
				hive_errors err = access_check(channel_id,access_mask,Db::read_check);
				if (err)
					return err;

				err = get_value_i(uLinksKey,strSubKey.c_str(),strLink);
				if (err == HIVE_OK)
					return HIVE_LINK;

				if (err != HIVE_NOTFOUND)
					return err;
			}
			break;

		case SQLITE_DONE:
			break;

		default:
			return HIVE_ERRORED;
		}

		// Reset and re-run
		resetter.reset();
	}
	
	// Bind the search values
	if (m_GetKeyInfo_Stmt.bind_string(1,strSubKey.c_str(),strSubKey.length()) != SQLITE_OK ||
			m_GetKeyInfo_Stmt.bind_int64(2,uParent) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}

	// And run the statement
	switch (m_GetKeyInfo_Stmt.step())
	{
	case SQLITE_ROW:
		uKey = m_GetKeyInfo_Stmt.column_int64(0);
		access_mask = static_cast<access_rights_t>(m_GetKeyInfo_Stmt.column_int(1));
		return access_check(channel_id,access_mask,Db::read_check);
	
	case SQLITE_DONE:
		return HIVE_NOTFOUND;

	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::find_key(Omega::int64_t uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName)
{
	// Lock must be held first...

	// Check if the parent still exists
	hive_errors err = check_key_exists(uParent,access_mask);
	if (err)
		return err;

	err = access_check(channel_id,access_mask,Db::read_check);
	if (err)
		return err;

	// Drill down looking for the key...
	for (size_t start = 0;;)
	{
		size_t pos = strSubKey.find('/',start);
		if (pos > start)
		{
			OOBase::LocalString strFirst;
			int err2 = 0;
			if (pos == OOBase::LocalString::npos)
			{
				err2 = strFirst.assign(strSubKey.c_str()+start);
				if (!err2 && strFirst.empty())
				{
					// Trailing / or empty subkey name
					uKey = uParent;
					strSubKey.clear();
					return HIVE_OK;
				}
			}
			else
				err2 = strFirst.assign(strSubKey.c_str()+start,pos-start);

			if (err2)
				LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);

			err = get_key_info(uParent,uKey,strFirst,channel_id,access_mask,strLink);
			if (!err || err == HIVE_LINK)
			{
				// Append valid part to strFullKeyName
				err2 = strFullKeyName.append("/",1);
				if (!err2)
					err2 = strFullKeyName.append(strFirst.c_str());
				if (err2)
					LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);

				if (err == HIVE_LINK)
				{
					if (pos == OOBase::LocalString::npos)
						strSubKey.clear();
					else
					{
						// Assign remains of strSubKey to strSubKey
						OOBase::LocalString strSecond;
						err2 = strSecond.assign(strSubKey.c_str()+pos);
						if (!err2)
							err2 = strSubKey.assign(strSecond.c_str());
						if (err2)
							LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);
					}
				}
			}
			else
			{
				// Assign remains of strSubKey to strSubKey
				OOBase::LocalString strSecond;
				err2 = strSecond.assign(strSubKey.c_str()+start);
				if (!err2)
					err2 = strSubKey.assign(strSecond.c_str());
				if (err2)
					LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);
			}

			if (err)
				return err;

			if (pos == OOBase::LocalString::npos)
			{
				strSubKey.clear();
				return HIVE_OK;
			}

			uParent = uKey;
		}

		start = pos + 1;
	}
}

Db::hive_errors Db::Hive::insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask)
{
	// Lock must be held first...

	Resetter resetter(m_InsertKey_Stmt);
	
	if (m_InsertKey_Stmt.bind_string(1,pszSubKey,strlen(pszSubKey)) != SQLITE_OK ||
			m_InsertKey_Stmt.bind_int64(2,uParent) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}

	int err = m_InsertKey_Stmt.step();
	if (err == SQLITE_READONLY)
		return HIVE_READONLY;
	else if (err != SQLITE_DONE)
		return HIVE_ERRORED;

	uKey = m_db->last_insert_rowid();

	if (access_mask != 0)
	{
		OOBase::LocalString strAc;
		int err2 = strAc.printf("%u",access_mask);
		if (err2)
			LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);

		return set_value_i(uKey,".access",strAc.c_str());
	}

	return HIVE_OK;
}

Db::hive_errors Db::Hive::create_key(Omega::int64_t uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, Omega::uint16_t flags, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	uKey = uParent;

	// Check if the key still exists
	access_rights_t access_mask = 0;
	hive_errors err = find_key(uParent,uKey,strSubKey,access_mask,channel_id,strLink,strFullKeyName);

	if (flags == 0 /*OpenExisting*/)
	{
		if (err == HIVE_NOTFOUND && !strSubKey.empty())
		{
			// Return the full missing name in strFullKeyName
			int err2 = strFullKeyName.append("/",1);
			if (!err2)
				err2 = strFullKeyName.append(strSubKey.c_str(),strSubKey.length());
		}
		return err;
	}

	if (flags == 1 /*OpenCreate*/ && err != HIVE_NOTFOUND)
		return err;

	if (flags == 2 /*CreateNew*/ && err != HIVE_NOTFOUND)
		return (err == HIVE_OK ? HIVE_ALREADYEXISTS : err);

	err = access_check(channel_id,access_mask,Db::write_check);
	if (err)
		return err;

	// protect_key flag is not inherited
	access_mask &= ~Db::protect_key;
	
	// parent is now key
	uParent = uKey;

	// Start a transaction..
	Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return HIVE_ERRORED;

	// Drill down creating keys...
	for (size_t start = 0;;)
	{
		size_t pos = strSubKey.find('/',start);
		if (pos > start)
		{
			OOBase::LocalString strFirst;
			int err2 = 0;
			if (pos == OOBase::LocalString::npos)
			{
				err2 = strFirst.assign(strSubKey.c_str()+start);
				if (!err2 && strFirst.empty())
				{
					// Trailing / or empty subkey name
					uKey = uParent;
					break;
				}
			}
			else
				err2 = strFirst.assign(strSubKey.c_str()+start,pos-start);

			if (!err2)
				err2 = strFullKeyName.append("/",1);
			if (!err2)
				err2 = strFullKeyName.append(strFirst.c_str());
			if (err2)
				LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);

			err = insert_key(uParent,uKey,strFirst.c_str(),access_mask);
			if (err)
				return err;

			if (pos == OOBase::LocalString::npos)
				break;

			uParent = uKey;
		}

		start = pos + 1;
	}
	
	if (trans.commit() != SQLITE_OK)
		return HIVE_ERRORED;

	return HIVE_OK;
}

Db::hive_errors Db::Hive::delete_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::LocalString& strFullKeyName)
{
	// This one is recursive, within a transaction and a lock...

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err != HIVE_NOTFOUND)
		return err;

	// Write must be checked
	err = access_check(channel_id,access_mask,Db::write_check);
	if (err)
		return err;

	if (access_mask & Db::protect_key)
	{
		// Not allowed to delete!
		return HIVE_PROTKEY;
	}

	// Get the set of subkeys
	OOBase::Stack<Omega::int64_t,OOBase::LocalAllocator> ids;
	{
		Resetter resetter(m_EnumKeyIds_Stmt);
		
		if (m_EnumKeyIds_Stmt.bind_int64(1,uKey) != SQLITE_OK)
			return HIVE_ERRORED;
				
		for (;;)
		{
			int err2 = m_EnumKeyIds_Stmt.step();
			if (err2 == SQLITE_ROW)
			{
				// Add to stack
				int res = ids.push(m_EnumKeyIds_Stmt.column_int64(0));
				if (res != 0)
					LOG_ERROR_RETURN(("Failed to stack push: %s",OOBase::system_error_text(res)),HIVE_ERRORED);
			}
			else if (err2 == SQLITE_DONE)
				break;
			else
				return HIVE_ERRORED;
		}
	}
	
	OOBase::LocalString strSubKey;
	if (!ids.empty())
	{
		for (Omega::int64_t id;ids.pop(&id);)
		{
			err = delete_subkeys(id,channel_id,strSubKey);
			if (err)
			{
				// Update strFullKeyName on err
				int err2 = strFullKeyName.append("/",1);
				if (!err2)
					err2 = strFullKeyName.append(strSubKey.c_str(),strSubKey.length());

				return err;
			}
		}
	}
	
	// Do the delete
	Resetter resetter(m_DeleteKeys_Stmt);

	if (m_DeleteKeys_Stmt.bind_int64(1,uKey) != SQLITE_OK)
		return HIVE_ERRORED;

	switch (m_DeleteKeys_Stmt.step())
	{
	case SQLITE_READONLY:
		return HIVE_READONLY;

	case SQLITE_DONE:
		return HIVE_OK;
		
	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::delete_key(const Omega::int64_t& uParent, OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Get the start key
	Omega::int64_t uKey;
	access_rights_t access_mask;
	hive_errors err = find_key(uParent,uKey,strSubKey,access_mask,channel_id,strLink,strFullKeyName);
	if (err)
	{
		if (err == HIVE_NOTFOUND && !strSubKey.empty())
		{
			// Return the full missing name in strFullKeyName
			int err2 = strFullKeyName.append("/",1);
			if (!err2)
				err2 = strFullKeyName.append(strSubKey.c_str(),strSubKey.length());
			if (err2)
				LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);
		}
		return err;
	}

	// Write must be checked
	err = access_check(channel_id,access_mask,Db::write_check);
	if (err)
		return err;

	if (access_mask & Db::protect_key)
	{
		// Not allowed to delete!
		return HIVE_PROTKEY;
	}

	Transaction trans(*m_db);
	if (trans.begin() != SQLITE_OK)
		return HIVE_ERRORED;

	err = delete_subkeys(uKey,channel_id,strFullKeyName);
	if (err)
		return err;

	// Do the delete
	Resetter resetter(m_DeleteKey_Stmt);

	if (m_DeleteKey_Stmt.bind_int64(1,uKey) != SQLITE_OK)
		return HIVE_ERRORED;

	switch (m_DeleteKey_Stmt.step())
	{
	case SQLITE_READONLY:
		return HIVE_READONLY;

	case SQLITE_DONE:
		break;
		
	default:
		return HIVE_ERRORED;
	}

	if (trans.commit() != SQLITE_OK)
		return HIVE_ERRORED;

	return HIVE_OK;
}

Db::hive_errors Db::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	// Do the access check up front...
	bool access_checked = false;
	bool allowed_access = false;
	if (access_mask & Db::read_check)
	{
		// Read must be checked
		err = access_check(channel_id,access_mask,Db::read_check);
		if (err)
			return err;

		access_checked = true;
		allowed_access = true;
	}

	Resetter resetter(m_EnumKeys_Stmt);
	
	if (m_EnumKeys_Stmt.bind_int64(1,uKey) != SQLITE_OK)
		return HIVE_ERRORED;

	for (;;)
	{
		switch (m_EnumKeys_Stmt.step())
		{
		case SQLITE_ROW:
			{
				const char* v = m_EnumKeys_Stmt.column_text(1);
				if (v)
				{
					OOBase::String strSubKey;
					int err3 = strSubKey.assign(v);
					if (err3)
						LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err3)),HIVE_ERRORED);

					bool bAllow = true;
					access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int64(2));
					if (access_mask & Db::read_check)
					{
						// Read must be checked
						if (!access_checked)
						{
							err = access_check(channel_id,access_mask,Db::read_check);

							allowed_access = (err == 0);
							access_checked = true;
						}

						bAllow = allowed_access;
					}

					if (bAllow)
					{
						err3 = setSubKeys.push(strSubKey);
						if (err3)
							LOG_ERROR_RETURN(("Failed to stack push: %s",OOBase::system_error_text(err3)),HIVE_ERRORED);
					}
				}
			}
			break;

		case SQLITE_DONE:
			return HIVE_OK;

		default:
			return HIVE_ERRORED;
		}
	}
}

void Db::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
	{
		response.write(err);
		return;
	}

	// Do the access check up front...
	bool access_checked = false;
	bool allowed_access = false;
	if (access_mask & Db::read_check)
	{
		// Read must be checked
		err = access_check(channel_id,access_mask,Db::read_check);
		if (err)
		{
			response.write(err);
			return;
		}

		access_checked = true;
		allowed_access = true;
	}

	// Write out success first
	if (!response.write(Omega::uint16_t(HIVE_OK)))
	{
		LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
		return;
	}

	Resetter resetter(m_EnumKeys_Stmt);
	
	if (m_EnumKeys_Stmt.bind_int64(1,uKey) != SQLITE_OK)
	{
		response.reset();
		response.write(Omega::uint16_t(HIVE_ERRORED));
		return;
	}

	for (;;)
	{
		switch (m_EnumKeys_Stmt.step())
		{
		case SQLITE_ROW:
			{
				const char* v = m_EnumKeys_Stmt.column_text(1);
				if (v)
				{
					bool bAllow = true;
					access_mask = static_cast<access_rights_t>(m_EnumKeys_Stmt.column_int64(2));
					if (access_mask & Db::read_check)
					{
						// Read must be checked
						if (!access_checked)
						{
							err = access_check(channel_id,access_mask,Db::read_check);

							allowed_access = (err == 0);
							access_checked = true;
						}

						bAllow = allowed_access;
					}

					if (bAllow && !response.write(v))
					{
						LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
						response.reset();
						response.write(Omega::uint16_t(HIVE_ERRORED));
						return;
					}
				}
			}
			break;

		case SQLITE_DONE:
			// Write terminating null
			if (!response.write(""))
			{
				LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
				response.reset();
				response.write(Omega::uint16_t(HIVE_ERRORED));
			}
			return;

		default:
			response.reset();
			response.write(Omega::uint16_t(HIVE_ERRORED));
			return;
		}
	}
}

Db::hive_errors Db::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	// Do the access check up front...
	if (access_mask & Db::read_check)
	{
		// Read must be checked
		err = access_check(channel_id,access_mask,Db::read_check);
		if (err)
			return err;
	}

	Resetter resetter(m_EnumValues_Stmt);
	
	if (m_EnumValues_Stmt.bind_int64(1,uKey) != SQLITE_OK)
		return HIVE_ERRORED;
	
	for (;;)
	{
		switch (m_EnumValues_Stmt.step())
		{
		case SQLITE_ROW:
			{
				const char* v = m_EnumValues_Stmt.column_text(0);
				if (v)
				{
					OOBase::String val;
					int err2 = val.assign(m_EnumValues_Stmt.column_text(0));
					if (err2)
						LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);

					err2 = setValues.push(val);
					if (err2)
						LOG_ERROR_RETURN(("Failed to stack push: %s",OOBase::system_error_text(err2)),HIVE_ERRORED);
				}
			}
			break;

		case SQLITE_DONE:
			return HIVE_OK;

		default:
			return HIVE_ERRORED;
		}
	}
}

void Db::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
	{
		response.write(err);
		return;
	}

	// Do the access check up front...
	if (access_mask & Db::read_check)
	{
		// Read must be checked
		err = access_check(channel_id,access_mask,Db::read_check);
		if (err)
		{
			response.write(err);
			return;
		}
	}

	// Write out success first
	if (!response.write(Omega::uint16_t(HIVE_OK)))
	{
		LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
		return;
	}

	Resetter resetter(m_EnumValues_Stmt);
	
	if (m_EnumValues_Stmt.bind_int64(1,uKey) != SQLITE_OK)
	{
		response.reset();
		response.write(Omega::uint16_t(HIVE_ERRORED));
		return;
	}

	for (;;)
	{
		switch (m_EnumValues_Stmt.step())
		{
		case SQLITE_ROW:
			{
				const char* v = m_EnumValues_Stmt.column_text(0);
				if (v && !response.write(v))
				{
					LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
					response.reset();
					response.write(Omega::uint16_t(HIVE_ERRORED));
					return;
				}
			}
			break;

		case SQLITE_DONE:
			// Write terminating null
			if (!response.write(""))
			{
				LOG_ERROR(("Failed to write to response: %s",OOBase::system_error_text(response.last_error())));
				response.reset();
				response.write(Omega::uint16_t(HIVE_ERRORED));
			}
			return;

		default:
			response.reset();
			response.write(Omega::uint16_t(HIVE_ERRORED));
			return;
		}
	}
}

Db::hive_errors Db::Hive::delete_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	// Write must be checked
	err = access_check(channel_id,access_mask,Db::write_check);
	if (err)
		return err;
	
	Resetter resetter(m_DeleteValue_Stmt);
	
	if (m_DeleteValue_Stmt.bind_string(1,pszName,strlen(pszName)) != SQLITE_OK ||
			m_DeleteValue_Stmt.bind_int64(2,uKey) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}

	switch (m_DeleteValue_Stmt.step())
	{
	case SQLITE_READONLY:
		return HIVE_READONLY;

	case SQLITE_DONE:
		return HIVE_OK;
	
	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::value_exists_i(const Omega::int64_t& uKey, const char* pszValue)
{
	// Lock must be held first...

	Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	if (m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue))  != SQLITE_OK ||
			m_GetValue_Stmt.bind_int64(2,uKey) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}

	switch (m_GetValue_Stmt.step())
	{
	case SQLITE_ROW:
		return HIVE_OK;

	case SQLITE_DONE:
		return HIVE_NOTFOUND;

	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	// Read must be checked
	err = access_check(channel_id,access_mask,Db::read_check);
	if (err)
		return err;

	return value_exists_i(uKey,pszValue);
}

Db::hive_errors Db::Hive::get_value_i(const Omega::int64_t& uKey, const char* pszValue, OOBase::LocalString& val)
{
	// Lock must be held first...

	Resetter resetter(m_GetValue_Stmt);

	// Bind the values
	if (m_GetValue_Stmt.bind_string(1,pszValue,strlen(pszValue)) != SQLITE_OK ||
			m_GetValue_Stmt.bind_int64(2,uKey) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}

	switch (m_GetValue_Stmt.step())
	{
	case SQLITE_ROW:
		{
			const char* v = m_GetValue_Stmt.column_text(0);
			if (v)
			{
				int err = val.assign(v);
				if (err)
					LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),HIVE_ERRORED);
			}
			return HIVE_OK;
		}

	case SQLITE_DONE:
		return HIVE_NOTFOUND;

	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	err = access_check(channel_id,access_mask,Db::read_check);
	if (err)
		return err;

	return get_value_i(uKey,pszValue,val);
}

Db::hive_errors Db::Hive::set_value_i(const Omega::int64_t& uKey, const char* pszName, const char* pszValue)
{
	// Lock must be held first...

	// See if we have a value already
	Statement* pStmt = NULL;
	hive_errors err = value_exists_i(uKey,pszName);
	if (err == HIVE_OK)
		pStmt = &m_UpdateValue_Stmt;
	else if (err == HIVE_NOTFOUND)
		pStmt = &m_InsertValue_Stmt;
	else
		return err;
		
	Resetter resetter(*pStmt);

	if (pStmt->bind_string(1,pszName,strlen(pszName)) != SQLITE_OK ||
			pStmt->bind_int64(2,uKey) != SQLITE_OK ||
			pStmt->bind_string(3,pszValue,strlen(pszValue)) != SQLITE_OK)
	{
		return HIVE_ERRORED;
	}
		
	switch (pStmt->step())
	{
	case SQLITE_READONLY:
		return HIVE_READONLY;

	case SQLITE_DONE:
		return HIVE_OK;

	default:
		return HIVE_ERRORED;
	}
}

Db::hive_errors Db::Hive::set_value(const Omega::int64_t& uKey, const char* pszName, Omega::uint32_t channel_id, const char* pszValue)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	hive_errors err = check_key_exists(uKey,access_mask);
	if (err)
		return err;

	err = access_check(channel_id,access_mask,Db::write_check);
	if (err)
		return err;
	
	return set_value_i(uKey,pszName,pszValue);
}
