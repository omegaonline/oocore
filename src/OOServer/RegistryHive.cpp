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
#include "RegistryHive.h"
#include "RootManager.h"

Root::RegistryHive::RegistryHive(Manager* pManager, OOBase::SmartPtr<Db::Database>& db) :
	m_pManager(pManager),
	m_db(db)
{
}

bool Root::RegistryHive::open()
{
	// Check that the tables we require exist, and create if they don't
	const char szSQL[] = 
		"BEGIN TRANSACTION; "
		"CREATE TABLE IF NOT EXISTS RegistryKeys "
		"("
			"Id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"Name TEXT NOT NULL, "
			"Description TEXT, "
			"Parent INTEGER NOT NULL, "
			"Access INTEGER DEFAULT 3, "
			"UNIQUE(Name,Parent) "
		");"
		"CREATE INDEX IF NOT EXISTS idx_RegistryKeys ON RegistryKeys(Parent);"
		"ANALYZE RegistryKeys;"
		"CREATE TABLE IF NOT EXISTS RegistryValues "
		"("
			"Name TEXT NOT NULL, "
			"Description TEXT, "
			"Parent INTEGER NOT NULL, "
			"Type INTEGER NOT NULL, "
			"Value,"
			"UNIQUE(Name,Parent) "
		");"
		"CREATE INDEX IF NOT EXISTS idx_RegistryValues ON RegistryValues(Parent);"
		"CREATE INDEX IF NOT EXISTS idx_RegistryValues2 ON RegistryValues(Name,Parent,Type);"
		"CREATE TRIGGER IF NOT EXISTS trg_RegistryKeys AFTER DELETE ON RegistryKeys "
			"BEGIN "
				" DELETE FROM RegistryValues WHERE Parent = OLD.Id; "
			"END;"
		"ANALYZE RegistryValues;"
		"COMMIT;";
			
	return (m_db->exec(szSQL) == SQLITE_OK);
}

int Root::RegistryHive::check_key_exists(const Omega::int64_t& uKey, int& access_mask)
{
	// Lock must be help first...
	if (uKey == 0)
	{
		access_mask = 5;
		return SQLITE_ROW;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Access FROM RegistryKeys WHERE Id = %lld;",uKey);
	
	int err = ptrStmt->step();
	if (err == SQLITE_ROW)
		access_mask = ptrStmt->column_int(0);
	
	return err;
}

int Root::RegistryHive::find_key(Omega::int64_t uParent, const std::string& strSubKey, Omega::int64_t& uKey, int& access_mask)
{
	// Lock must be help first...

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Id, Access FROM RegistryKeys WHERE Name = %Q AND Parent = %lld;",strSubKey.c_str(),uParent);
	
	int err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		uKey = ptrStmt->column_int64(0);
		access_mask = ptrStmt->column_int(1);
	}
		
	return err;
}

int Root::RegistryHive::find_key(Omega::int64_t& uKey, std::string& strSubKey, int& access_mask, Omega::uint32_t channel_id)
{
	// Check if the key still exists
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Drill down looking for the key...
	for (;;)
	{
		size_t pos = strSubKey.find('\\');
		if (pos == std::string::npos)
			break;

		err = find_key(uKey,strSubKey.substr(0,pos),uKey,access_mask);
		if (err == SQLITE_DONE)
			return ENOENT;
		else if (err != SQLITE_ROW)
			return EIO;

		if (!(access_mask & 1) && channel_id != 0)
		{
			// Read not allowed - check access!
			int acc = m_pManager->registry_access_check(channel_id);
			if (acc != 0)
				return acc;
		}

		strSubKey = strSubKey.substr(pos+1);
	}

	err = find_key(uKey,strSubKey,uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	strSubKey.empty();
	return 0;
}

int Root::RegistryHive::insert_key(Omega::int64_t& uKey, std::string strSubKey, int access_mask)
{
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("INSERT INTO RegistryKeys (Name,Parent,Access) VALUES (%Q,%lld,%d);",strSubKey.c_str(),uKey,access_mask);
	
	int err = ptrStmt->step();
	if (err == SQLITE_DONE)
		uKey = sqlite3_last_insert_rowid(m_db->database());
	
	return err;
}

int Root::RegistryHive::open_key(Omega::int64_t& uKey, std::string strSubKey, Omega::uint32_t channel_id)
{
	if (uKey==0 && strSubKey.empty())
		return 0;

	OOBase::Guard<OOBase::Mutex> guard(m_lock);
	
	// Find the key
	int access_mask = 0;
	return find_key(uKey,strSubKey,access_mask,channel_id);
}

int Root::RegistryHive::create_key(Omega::int64_t& uKey, std::string strSubKey, bool bFailIfThere, int access, Omega::uint32_t channel_id)
{
	if (uKey==0 && strSubKey.empty())
		return bFailIfThere ? EEXIST : 0;

	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 3;
	int err = find_key(uKey,strSubKey,access_mask,channel_id);
	if (err != 0 && err != ENOENT)
		return err;

	OOBase::SmartPtr<Db::Transaction> ptrTrans;
	if (err == ENOENT)
	{
		// Need to add more...				
		if (!(access_mask & 2) && channel_id != 0)
		{
			// Write not allowed - check access!
			int acc = m_pManager->registry_access_check(channel_id);
			if (acc != 0)
				return acc;
		}

		// Start a transaction..
		ptrTrans = m_db->begin_transaction();
		if (!ptrTrans)
			return EIO;

		// Mask the access mask
		if (access & 8)
			access = (access_mask & ~4);

		// Drill down creating keys...
		for (;;)
		{
			size_t pos = strSubKey.find('\\');
			if (pos == std::string::npos)
				err = insert_key(uKey,strSubKey,access);
			else
				err = insert_key(uKey,strSubKey.substr(0,pos),access);

			if (err != SQLITE_DONE)
				return EIO;

			if (pos == std::string::npos)
				break;
			
			strSubKey = strSubKey.substr(pos+1);
		}
	}
	else if (bFailIfThere)
	{
		return EEXIST;
	}
	
	if (ptrTrans)
	{
		err = ptrTrans->commit();
		if (err != SQLITE_OK)
			return EIO;
	}
	
	return 0;
}

int Root::RegistryHive::delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id)
{
	// This one is recursive, within a transaction and a lock...
	
	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return 0;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	if (access_mask & 4)
	{
		// Never allowed to delete!
		return EACCES;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Id FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
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

	} while (err == SQLITE_ROW);

	if (err != SQLITE_DONE)
		return EIO;

	if (bFound)
	{
		// Do the delete
		ptrStmt = m_db->prepare_statement("DELETE FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
		err = ptrStmt->step();
		if (err != SQLITE_DONE)
			return EIO;
	}

	return 0;
}

int Root::RegistryHive::delete_key(Omega::int64_t uKey, std::string strSubKey, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = find_key(uKey,strSubKey,access_mask,channel_id);
	if (err != 0)
		return err;

	OOBase::SmartPtr<Db::Transaction> ptrTrans = m_db->begin_transaction();
	if (!ptrTrans)
		return EIO;
	
	err = delete_key_i(uKey,channel_id);
	if (err == 0)
	{
		// Do the delete of this key
		OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("DELETE FROM RegistryKeys WHERE Id = %lld;",uKey);
	
		err = ptrStmt->step();
		if (err != SQLITE_DONE)
			return EIO;

		err = ptrTrans->commit();
		if (err != SQLITE_OK)
			return EIO;
	}

	return err;
}

int Root::RegistryHive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::list<std::string>& listSubKeys)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	// Do the access check up front...
	int acc = m_pManager->registry_access_check(channel_id);

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Name, Access FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			std::string strSubKey;
			const char* v = ptrStmt->column_text(0);
			if (v)
				strSubKey = v;
			
			access_mask = ptrStmt->column_int(1);
			if (!(access_mask & 1) && channel_id != 0)
			{
				// Read not allowed - check access!
				if (acc != 0)
					strSubKey.clear();
			}
			
			if (!strSubKey.empty())
				listSubKeys.push_back(strSubKey);
		}

	} while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Root::RegistryHive::enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response.write((int)ENOENT);
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response.write((int)EIO);
		return;
	}

	// Do the access check up front...
	int acc = m_pManager->registry_access_check(channel_id);
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	// Write out success first
	response.write((int)0);
	if (response.last_error() != 0)
		return;

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Name, Access FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			std::string strSubKey;
			const char* v = ptrStmt->column_text(0);
			if (v)
				strSubKey = v;

			access_mask = ptrStmt->column_int(1);
			if (!(access_mask & 1) && channel_id != 0)
			{
				// Read not allowed - check access!
				if (acc != 0)
					strSubKey.clear();
			}
			
			if (!strSubKey.empty() && !response.write(strSubKey))
			{
				response.reset();
				response.write((int)EIO);
				return;
			}
		}
	} while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write(std::string());
	}
	else
	{
		response.reset();
		response.write((int)EIO);
	}
}

int Root::RegistryHive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::list<std::string>& listValues)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Name FROM RegistryValues WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			const char* v = ptrStmt->column_text(0);
			if (v)
				listValues.push_back(v);
		}

	} while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Root::RegistryHive::enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response.write((int)ENOENT);
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response.write((int)EIO);
		return;
	}
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	// Write out success first
	response.write((int)0);
	if (response.last_error() != 0)
		return;

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Name FROM RegistryValues WHERE Parent = %lld;",uKey);

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
				response.write((int)EIO);
				return;
			}
		}
	} while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write(std::string());
	}
	else
	{
		response.reset();
		response.write((int)EIO);
	}
}

int Root::RegistryHive::delete_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("DELETE FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::get_value_type(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::byte_t& type)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		type = static_cast<Omega::byte_t>(ptrStmt->column_int(0));
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Root::RegistryHive::get_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type,Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 0)
			return EINVAL;

		const char* v = ptrStmt->column_text(1);
		if (v)
			val = v;
		
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Root::RegistryHive::get_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::int64_t& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type,Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 1)
			return EINVAL;

		val = ptrStmt->column_int64(1);
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

void Root::RegistryHive::get_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t cbLen, Omega::uint32_t channel_id, OOBase::CDRStream& response)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response.write((int)ENOENT);
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response.write((int)EIO);
		return;
	}
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
		{
			response.write(acc);
			return;
		}
	}

	// Write out success first
	response.write((int)0);
	if (response.last_error() != 0)
		return;

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type,Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 2)
		{
			response.reset();
			response.write((int)EINVAL);
		}
		else
		{
			const void* pData = ptrStmt->column_blob(1);
			int len = ptrStmt->column_bytes(1);

			bool bData = false;
			if (cbLen)
			{
				bData = true;
				if (static_cast<Omega::uint32_t>(len) < cbLen)
					cbLen = static_cast<Omega::uint32_t>(len);
			}
			else
				cbLen = static_cast<Omega::uint32_t>(len);

			if (!response.write(cbLen))
			{
				response.reset();
				response.write((int)EIO);
			}
			else if (bData)
			{
				if (!response.write_bytes(static_cast<const Omega::byte_t*>(pData),cbLen))
				{
					response.reset();
					response.write((int)EIO);
				}
			}
		}

		return;
	}
	
	if (err == SQLITE_DONE)
		err = ENOENT;
	else
		err = EIO;

	response.reset();
	response.write(err);
}

int Root::RegistryHive::set_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const char* val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// See if we have a value already
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 0)
			return EINVAL;

		// We have an entry already
		ptrStmt = m_db->prepare_statement("UPDATE RegistryValues SET Value = %Q WHERE Name = %Q AND Parent = %lld;",val,strValue.c_str(),uKey);
	
		err = ptrStmt->step();
	}
	else if (err == SQLITE_DONE)
	{
		// Insert the new value
		OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("INSERT INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,0,%Q);",strValue.c_str(),uKey,val);
		
		err = ptrStmt->step();
	}

	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::set_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const Omega::int64_t& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// See if we have a value already
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 1)
			return EINVAL;

		// We have an entry already
		ptrStmt = m_db->prepare_statement("UPDATE RegistryValues SET Value = %lld WHERE Name = %Q AND Parent = %lld;",val,strValue.c_str(),uKey);
	
		err = ptrStmt->step();
	}
	else if (err == SQLITE_DONE)
	{
		// Insert the new value
		OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("INSERT INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,1,%lld);",strValue.c_str(),uKey,val);
		
		err = ptrStmt->step();
	}

	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::set_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const OOBase::CDRStream& request)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// See if we have a value already
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		int type = ptrStmt->column_int(0);
		if (type != 2)
			return EINVAL;

		// We have an entry already
		ptrStmt = m_db->prepare_statement("UPDATE RegistryValues SET Value = ? WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
		err = sqlite3_bind_blob(ptrStmt->statement(),1,request.buffer()->rd_ptr(),static_cast<int>(request.buffer()->length()),SQLITE_STATIC);
		if (err != SQLITE_OK)
			LOG_ERROR(("sqlite3_bind_blob failed: %s",sqlite3_errmsg(m_db->database())));
		else
			err = ptrStmt->step();
	}
	else if (err == SQLITE_DONE)
	{
		// Insert the new value
		OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("INSERT INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,2,?);",strValue.c_str(),uKey);
			
		err = sqlite3_bind_blob(ptrStmt->statement(),1,request.buffer()->rd_ptr(),static_cast<int>(request.buffer()->length()),SQLITE_STATIC);
		if (err != SQLITE_OK)
			LOG_ERROR(("sqlite3_bind_blob failed: %s",sqlite3_errmsg(m_db->database())));
		else
			err = ptrStmt->step();
	}
	
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Description FROM RegistryKeys WHERE Id = %lld;",uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		const char* v = ptrStmt->column_text(0);
		if (v)
			val = v;

		return 0;
	}
	else if (err == SQLITE_DONE)
		return 0;
	else
		return EIO;
}

int Root::RegistryHive::get_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("SELECT Description FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		const char* v = ptrStmt->column_text(0);
		if (v)
			val = v;

		return 0;
	}
	else if (err == SQLITE_DONE)
	{
		// See if the value exists at all...
		ptrStmt = m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
			return 0;
		else if (err == SQLITE_DONE)
			return ENOENT;
		else
			return EIO;
	}
	else
		return EIO;
}

int Root::RegistryHive::set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("UPDATE RegistryKeys SET Description = %Q WHERE Id = %lld;",val.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::set_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const std::string& val)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	if (!(access_mask & 2) && channel_id != 0)
	{
		// Write not allowed - check access!
		int acc = m_pManager->registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	OOBase::SmartPtr<Db::Statement> ptrStmt = m_db->prepare_statement("UPDATE RegistryValues SET Description = %Q WHERE Name = %Q AND Parent = %lld;",val.c_str(),strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}
