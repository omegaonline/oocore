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
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RegistryHive.h"

Registry::Hive::Hive(Manager* pManager, const std::string& strdb) :
		m_pManager(pManager),
		m_strdb(strdb)
{
}

bool Registry::Hive::open(int flags)
{
	OOBASE_NEW_T(OOSvrBase::Db::Database,m_db,OOSvrBase::Db::Database());
	if (!m_db)
		LOG_ERROR_RETURN(("Out of memory"),false);

	return m_db->open(m_strdb.c_str(),flags);
}

int Registry::Hive::check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask)
{
	// Lock must be held first...
	if (uKey == 0)
	{
		access_mask = static_cast<access_rights_t>(Hive::never_delete | Hive::write_check);
		return SQLITE_ROW;
	}

	// Create or reset the prepared statement
	if (!m_ptrCheckKey_Stmt)
	{
		int err = m_db->prepare_statement(m_ptrCheckKey_Stmt,"SELECT Access, Description FROM RegistryKeys WHERE Id = ?1;");
		if (err != SQLITE_OK)
			return err;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrCheckKey_Stmt);

	// Bind the key value
	int err = m_ptrCheckKey_Stmt->bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return err;

	// And run the statement
	err = m_ptrCheckKey_Stmt->step();
	if (err == SQLITE_ROW)
		access_mask = static_cast<access_rights_t>(m_ptrCheckKey_Stmt->column_int(0));

	return err;
}

int Registry::Hive::get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const std::string& strSubKey, access_rights_t& access_mask)
{
	// Lock must be held first...

	// Create or reset the prepared statement
	int err = SQLITE_OK;
	if (!m_ptrGetKeyInfo_Stmt)
	{
		err = m_db->prepare_statement(m_ptrGetKeyInfo_Stmt,"SELECT Id, Access FROM RegistryKeys WHERE Name = ?1 AND Parent = ?2;");
		if (err != SQLITE_OK)
			return err;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrGetKeyInfo_Stmt);

	// Bind the search values
	err = m_ptrGetKeyInfo_Stmt->bind_string(1,strSubKey);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetKeyInfo_Stmt->bind_int64(2,uParent);
	if (err != SQLITE_OK)
		return err;

	// And run the statement
	err = m_ptrGetKeyInfo_Stmt->step();
	if (err == SQLITE_ROW)
	{
		uKey = m_ptrGetKeyInfo_Stmt->column_int64(0);
		access_mask = static_cast<access_rights_t>(m_ptrGetKeyInfo_Stmt->column_int(1));
	}

	return err;
}

int Registry::Hive::find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, std::string& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id)
{
	// Lock must be held first...

	// Check for root key
	if (uParent==0 && strSubKey.empty())
	{
		access_mask = static_cast<access_rights_t>(Hive::never_delete | Hive::write_check);
		uKey = 0;
		return 0;
	}

	// Check if the key still exists
	int err = check_key_exists(uParent,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Drill down looking for the key...
	uKey = uParent;
	Omega::int64_t uSubKey = uParent;
	for (;;)
	{
		size_t pos = strSubKey.find('/');
		if (pos == std::string::npos)
			err = get_key_info(uSubKey,uKey,strSubKey,access_mask);
		else
			err = get_key_info(uSubKey,uKey,strSubKey.substr(0,pos),access_mask);

		if (err == SQLITE_DONE)
			return ENOENT;
		else if (err != SQLITE_ROW)
			return EIO;

		if (access_mask & Hive::read_check)
		{
			// Read not allowed - check access!
			int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
			if (acc != 0)
				return acc;
		}

		if (pos == std::string::npos)
			break;

		strSubKey.erase(0,pos+1);
		uSubKey = uKey;
	}

	strSubKey.empty();
	return 0;
}

int Registry::Hive::insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const std::string& strSubKey, access_rights_t access_mask)
{
	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	int err = m_db->prepare_statement(ptrStmt,"INSERT INTO RegistryKeys (Name,Parent,Access) VALUES (%Q,%lld,%u);",strSubKey.c_str(),uParent,access_mask);
	if (err != SQLITE_OK)
		return err;

	err = ptrStmt->step();
	if (err == SQLITE_DONE)
		uKey = sqlite3_last_insert_rowid(m_db->database());

	return err;
}

int Registry::Hive::open_key(Omega::int64_t uParent, Omega::int64_t& uKey, std::string strSubKey, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Find the key
	access_rights_t access_mask;
	return find_key(uParent,uKey,strSubKey,access_mask,channel_id);
}

int Registry::Hive::create_key(Omega::int64_t uParent, Omega::int64_t& uKey, std::string strSubKey, Omega::uint16_t flags, access_rights_t access, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = find_key(uParent,uKey,strSubKey,access_mask,channel_id);

	if (flags == 0 /*OpenExisting*/)
		return err;

	if (flags == 1 /*OpenCreate*/ && err != ENOENT)
		return err;

	if (flags == 2 /*CreateNew*/ && err != ENOENT)
		return (err == 0 ? EEXIST : err);

	// Start inserting
	OOBase::SmartPtr<OOSvrBase::Db::Transaction> ptrTrans;

	// Need to add more...
	if (access_mask & Hive::write_check)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
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
	if (m_db->begin_transaction(ptrTrans) != SQLITE_OK)
		return EIO;

	// Drill down creating keys...
	Omega::int64_t uSubKey = uKey;
	for (;;)
	{
		size_t pos = strSubKey.find('/');
		if (pos == std::string::npos)
			err = insert_key(uSubKey,uKey,strSubKey,access);
		else
			err = insert_key(uSubKey,uKey,strSubKey.substr(0,pos),access);

		if (err == SQLITE_READONLY)
			return EACCES;
		else if (err != SQLITE_DONE)
			return EIO;

		if (pos == std::string::npos)
			break;

		strSubKey.erase(0,pos+1);
		uSubKey = uKey;
	}

	if (ptrTrans->commit() != SQLITE_OK)
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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (access_mask & Hive::never_delete)
	{
		// Never allowed to delete!
		return EACCES;
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	if (m_db->prepare_statement(ptrStmt,"SELECT Id FROM RegistryKeys WHERE Parent = %lld;",uKey) != SQLITE_OK)
		return EIO;

	// Recurse down
	bool bFound = false;
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			// Worth deleting at this level...
			bFound = true;

			// Recurse down
			int res = delete_key_i(ptrStmt->column_int64(0),channel_id);
			if (res != 0)
				return res;
		}

	}
	while (err == SQLITE_ROW);

	if (err != SQLITE_DONE)
		return EIO;

	if (bFound)
	{
		// Do the delete
		if (m_db->prepare_statement(ptrStmt,"DELETE FROM RegistryKeys WHERE Parent = %lld;",uKey) != SQLITE_OK)
			return EIO;

		if (ptrStmt->step() != SQLITE_DONE)
			return EIO;
	}

	return 0;
}

int Registry::Hive::delete_key(const Omega::int64_t& uParent, std::string strSubKey, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Get the start key
	Omega::int64_t uKey = 0;
	access_rights_t access_mask;
	int err = find_key(uParent,uKey,strSubKey,access_mask,channel_id);
	if (err != 0)
		return err;

	OOBase::SmartPtr<OOSvrBase::Db::Transaction> ptrTrans;
	if (m_db->begin_transaction(ptrTrans) != SQLITE_OK)
		return EIO;

	err = delete_key_i(uKey,channel_id);
	if (err == 0)
	{
		// Do the delete of this key
		OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
		err = m_db->prepare_statement(ptrStmt,"DELETE FROM RegistryKeys WHERE Id = %lld;",uKey);
		if (err != SQLITE_OK)
			return EIO;

		if (ptrStmt->step() != SQLITE_DONE)
			return EIO;

		if (ptrTrans->commit() != SQLITE_OK)
			return EIO;
	}

	return err;
}

int Registry::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::set<std::string>& setSubKeys)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	access_rights_t access_mask;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	// Do the access check up front...
	int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"SELECT Name, Access FROM RegistryKeys WHERE Parent = %lld ORDER BY Name;",uKey);
	if (err != SQLITE_OK)
		return EIO;

	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			std::string strSubKey;
			const char* v = ptrStmt->column_text(0);
			if (v)
				strSubKey = v;

			access_mask = static_cast<access_rights_t>(ptrStmt->column_int(1));
			if (access_mask & Hive::read_check)
			{
				// Read not allowed - check access!
				if (acc != 0)
					strSubKey.clear();
			}

			if (!strSubKey.empty())
				setSubKeys.insert(strSubKey);
		}

	}
	while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Registry::Hive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
	Omega::int32_t acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);

	if (access_mask & Hive::read_check)
	{
		// Read not allowed - check access!
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"SELECT Name, Access FROM RegistryKeys WHERE Parent = %lld ORDER BY Name;",uKey);
	if (err != SQLITE_OK)
	{
		response.write(Omega::int32_t(EIO));
		return;
	}

	// Write out success first
	response.write(Omega::int32_t(0));
	if (response.last_error() != 0)
		return;

	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			std::string strSubKey;
			const char* v = ptrStmt->column_text(0);
			if (v)
				strSubKey = v;

			access_mask = static_cast<access_rights_t>(ptrStmt->column_int(1));
			if (access_mask & Hive::read_check)
			{
				// Read not allowed - check access!
				if (acc != 0)
					strSubKey.clear();
			}

			if (!strSubKey.empty() && !response.write(strSubKey))
			{
				response.reset();
				response.write(Omega::int32_t(EIO));
				return;
			}
		}
	}
	while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write(std::string());
	}
	else
	{
		response.reset();
		response.write(Omega::int32_t(EIO));
	}
}

int Registry::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::set<std::string>& setValues)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"SELECT Name FROM RegistryValues WHERE Parent = %lld ORDER BY Name;",uKey);
	if (err != SQLITE_OK)
		return EIO;

	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			const char* v = ptrStmt->column_text(0);
			if (v)
				setValues.insert(v);
		}

	}
	while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Registry::Hive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		Omega::int32_t acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"SELECT Name FROM RegistryValues WHERE Parent = %lld ORDER BY Name;",uKey);
	if (err != SQLITE_OK)
	{
		response.write(Omega::int32_t(EIO));
		return;
	}

	// Write out success first
	response.write(Omega::int32_t(0));
	if (response.last_error() != 0)
		return;

	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			std::string str;
			const char* v = ptrStmt->column_text(0);
			if (v)
				str = v;

			if (!response.write(str))
			{
				response.reset();
				response.write(Omega::int32_t(EIO));
				return;
			}
		}
	}
	while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write(std::string());
	}
	else
	{
		response.reset();
		response.write(Omega::int32_t(EIO));
	}
}

int Registry::Hive::delete_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"DELETE FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = ptrStmt->step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::value_exists(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	return value_exists_i(uKey,strValue);
}

int Registry::Hive::value_exists_i(const Omega::int64_t& uKey, const std::string& strValue)
{
	if (!m_ptrGetValue_Stmt)
	{
		int err = m_db->prepare_statement(m_ptrGetValue_Stmt,"SELECT Value,Description FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;");
		if (err != SQLITE_OK)
			return EIO;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrGetValue_Stmt);

	// Bind the values
	int err = m_ptrGetValue_Stmt->bind_string(1,strValue);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->step();
	if (err == SQLITE_ROW)
		return 0;
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Registry::Hive::get_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (!m_ptrGetValue_Stmt)
	{
		err = m_db->prepare_statement(m_ptrGetValue_Stmt,"SELECT Value,Description FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;");
		if (err != SQLITE_OK)
			return EIO;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrGetValue_Stmt);

	// Bind the values
	err = m_ptrGetValue_Stmt->bind_string(1,strValue);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_ptrGetValue_Stmt->column_text(0);
		if (v)
			val = v;

		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Registry::Hive::set_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const char* val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// See if we have a value already
	err = value_exists_i(uKey,strValue);
	if (err == 0)
	{
		// We have an entry already
		OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
		err = m_db->prepare_statement(ptrStmt,"UPDATE RegistryValues SET Value = %Q WHERE Name = %Q AND Parent = %lld;",val,strValue.c_str(),uKey);
		if (err != SQLITE_OK)
			return EIO;

		err = ptrStmt->step();
	}
	else if (err == ENOENT)
	{
		// Insert the new value
		OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
		err = m_db->prepare_statement(ptrStmt,"INSERT INTO RegistryValues (Name,Parent,Value) VALUES (%Q,%lld,%Q);",strValue.c_str(),uKey,val);
		if (err != SQLITE_OK)
			return EIO;

		err = ptrStmt->step();
	}
	else
		return err;

	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Create or reset the prepared statement
	if (!m_ptrCheckKey_Stmt)
	{
		err = m_db->prepare_statement(m_ptrCheckKey_Stmt,"SELECT Access, Description FROM RegistryKeys WHERE Id = ?1;");
		if (err != SQLITE_OK)
			return err;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrCheckKey_Stmt);

	// Bind the key value
	err = m_ptrCheckKey_Stmt->bind_int64(1,uKey);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrCheckKey_Stmt->step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_ptrCheckKey_Stmt->column_text(1);
		if (v)
			val = v;

		return 0;
	}
	else if (err == SQLITE_DONE)
		return 0;
	else
		return EIO;
}

int Registry::Hive::get_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	if (!m_ptrGetValue_Stmt)
	{
		err = m_db->prepare_statement(m_ptrGetValue_Stmt,"SELECT Value,Description FROM RegistryValues WHERE Name = ?1 AND Parent = ?2;");
		if (err != SQLITE_OK)
			return EIO;
	}

	OOSvrBase::Db::Resetter resetter(*m_ptrGetValue_Stmt);

	// Bind the values
	err = m_ptrGetValue_Stmt->bind_string(1,strValue);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->bind_int64(2,uKey);
	if (err != SQLITE_OK)
		return err;

	err = m_ptrGetValue_Stmt->step();
	if (err == SQLITE_ROW)
	{
		const char* v = m_ptrGetValue_Stmt->column_text(1);
		if (v)
			val = v;

		return 0;
	}
	else if (err == SQLITE_DONE)
	{
		// See if the value exists at all...
		return value_exists_i(uKey,strValue);
	}
	else
		return EIO;
}

int Registry::Hive::set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"UPDATE RegistryKeys SET Description = %Q WHERE Id = %lld;",val.c_str(),uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = ptrStmt->step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;

	return 0;
}

int Registry::Hive::set_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

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
		int acc = m_pManager->registry_access_check(m_strdb,channel_id,access_mask);
		if (acc != 0)
			return acc;
	}

	// Check the value exists...
	err = value_exists_i(uKey,strValue);
	if (err != 0)
		return err;

	// Insert the new value
	OOBase::SmartPtr<OOSvrBase::Db::Statement> ptrStmt;
	err = m_db->prepare_statement(ptrStmt,"UPDATE RegistryValues SET Description = %Q WHERE Name = %Q AND Parent = %lld;",val.c_str(),strValue.c_str(),uKey);
	if (err != SQLITE_OK)
		return EIO;

	err = ptrStmt->step();
	if (err == SQLITE_READONLY)
		return EACCES;
	else if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}
