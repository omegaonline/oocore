///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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
#include "./RegistryHive.h"
#include "./RootManager.h"

Root::RegistryHive::RegistryHive(ACE_Refcounted_Auto_Ptr<Db::Database,ACE_Null_Mutex>& db) :
	m_db(db)
{
}

int Root::RegistryHive::open()
{
	// Check that the tables we require exist, and create if they don't
	const char szSQL[] = 
		"BEGIN TRANSACTION; "
		"CREATE TABLE IF NOT EXISTS RegistryKeys "
		"("
			"Id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"Name TEXT NOT NULL, Parent INTEGER NOT NULL, Access INTEGER DEFAULT 3, "
			"UNIQUE(Name,Parent) "
		");"
		"CREATE INDEX IF NOT EXISTS idx_RegistryKeys ON RegistryKeys(Parent);"
		"ANALYZE RegistryKeys;"
		"CREATE TABLE IF NOT EXISTS RegistryValues "
		"("
			"Name TEXT NOT NULL, Parent INTEGER NOT NULL, Type INTEGER NOT NULL, Value,"
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
			
	int err = m_db->exec(szSQL);
	if (err != SQLITE_OK)
		return -1;

	return 0;
}

int Root::RegistryHive::check_key_exists(const ACE_INT64& uKey, int& access_mask)
{
	// Lock must be help first...
	if (uKey == 0)
	{
		access_mask = 0;
		return SQLITE_ROW;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Access FROM RegistryKeys WHERE Id = %lld;",uKey);
	
	int err = ptrStmt->step();
	if (err == SQLITE_ROW)
		access_mask = ptrStmt->column_int(0);
	
	return err;
}

int Root::RegistryHive::find_key(ACE_INT64 uParent, const ACE_CString& strSubKey, ACE_INT64& uKey, int& access_mask)
{
	// Lock must be help first...

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Id, Access FROM RegistryKeys WHERE Name = %Q AND Parent = %lld;",strSubKey.c_str(),uParent);
	
	int err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		uKey = ptrStmt->column_int64(0);
		access_mask = ptrStmt->column_int(1);
	}
		
	return err;
}

int Root::RegistryHive::find_key(ACE_INT64& uKey, ACE_CString& strSubKey, int& access_mask, ACE_CDR::ULong channel_id)
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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Drill down looking for the key...
	for (;;)
	{
		size_t pos = strSubKey.find('\\');
		if (pos == ACE_CString::npos)
			break;

		err = find_key(uKey,strSubKey.substr(0,pos),uKey,access_mask);
		if (err == SQLITE_DONE)
			return ENOENT;
		else if (err != SQLITE_ROW)
			return EIO;

		if (!(access_mask & 1) && channel_id != 0)
		{
			// Read not allowed - check access!
			int acc = Root::Manager::registry_access_check(channel_id);
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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	strSubKey.empty();
	return 0;
}

int Root::RegistryHive::insert_key(ACE_INT64& uKey, ACE_CString strSubKey, int access_mask)
{
	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("INSERT INTO RegistryKeys (Name,Parent,Access) VALUES (%Q,%lld,%d);",strSubKey.c_str(),uKey,access_mask);
	
	int err = ptrStmt->step();
	if (err == SQLITE_DONE)
		uKey = sqlite3_last_insert_rowid(m_db->database());
	
	return err;
}

int Root::RegistryHive::open_key(ACE_INT64& uKey, ACE_CString strSubKey, ACE_CDR::ULong channel_id)
{
	if (uKey==0 && strSubKey.empty())
		return 0;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	// Find the key
	int access_mask = 0;
	return find_key(uKey,strSubKey,access_mask,channel_id);
}

int Root::RegistryHive::create_key(ACE_INT64& uKey, ACE_CString strSubKey, bool bFailIfThere, int access, ACE_CDR::ULong channel_id)
{
	if (uKey==0 && strSubKey.empty())
		return bFailIfThere ? EALREADY : 0;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	// Check if the key still exists
	int access_mask = 3;
	int err = find_key(uKey,strSubKey,access_mask,channel_id);
	if (err != 0 && err != ENOENT)
		return err;

	ACE_Refcounted_Auto_Ptr<Db::Transaction,ACE_Null_Mutex> ptrTrans;

	if (err == ENOENT)
	{
		// Need to add more...				
		if (!(access_mask & 2) && channel_id != 0)
		{
			// Write not allowed - check access!
			int acc = Root::Manager::registry_access_check(channel_id);
			if (acc != 0)
				return acc;
		}

		// Start a transaction..
		ptrTrans = m_db->begin_transaction();
		if (ptrTrans.null())
			return EIO;

		// Mask the access mask
		if (access & 8)
			access = (access_mask & ~4);

		// Drill down creating keys...
		for (;;)
		{
			size_t pos = strSubKey.find('\\');
			if (pos == ACE_CString::npos)
				err = insert_key(uKey,strSubKey,access);
			else
				err = insert_key(uKey,strSubKey.substr(0,pos),access);

			if (err != SQLITE_DONE)
				return EIO;

			if (pos == ACE_CString::npos)
				break;
			
			strSubKey = strSubKey.substr(pos+1);
		}
	}
	else if (bFailIfThere)
	{
		return EALREADY;
	}
	
	if (!ptrTrans.null())
	{
		err = ptrTrans->commit();
		if (err != SQLITE_OK)
			return EIO;
	}
	
	return 0;
}

int Root::RegistryHive::delete_key_i(const ACE_INT64& uKey, ACE_CDR::ULong channel_id)
{
	// This one is recursive, within a transaction and a lock...
	
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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	if (access_mask & 4)
	{
		// Never allowed to delete!
		return EACCES;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Id FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
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

int Root::RegistryHive::delete_key(ACE_INT64 uKey, ACE_CString strSubKey, ACE_CDR::ULong channel_id)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	// Check if the key still exists
	int access_mask = 0;
	int err = find_key(uKey,strSubKey,access_mask,channel_id);
	if (err != 0)
		return err;

	ACE_Refcounted_Auto_Ptr<Db::Transaction,ACE_Null_Mutex> ptrTrans = m_db->begin_transaction();
	if (ptrTrans.null())
		return EIO;
	
	err = delete_key_i(uKey,channel_id);
	if (err == 0)
	{
		// Do the delete of this key
		ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
			m_db->prepare_statement("DELETE FROM RegistryKeys WHERE Id = %lld;",uKey);
	
		err = ptrStmt->step();
		if (err != SQLITE_DONE)
			return EIO;

		err = ptrTrans->commit();
		if (err != SQLITE_OK)
			return EIO;
	}

	return err;
}

int Root::RegistryHive::enum_subkeys(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, std::list<ACE_CString>& listSubKeys)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Name FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
			listSubKeys.push_back(ptrStmt->column_text(0));

	} while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Root::RegistryHive::enum_subkeys(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response << ENOENT;
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response << EIO;
		return;
	}	
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
		{
			response << acc;
			return;
		}
	}

	// Write out success first
	response << (int)0;
	if (!response.good_bit())
		return;

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Name FROM RegistryKeys WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			if (!response.write_string(ptrStmt->column_text(0)))
			{
				response.reset();
				response << ACE_OS::last_error();
				return;
			}
		}
	} while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write_string(0);
	}
	else
	{
		response.reset();
		response << EIO;
	}
}

int Root::RegistryHive::enum_values(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, std::list<ACE_CString>& listValues)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Name FROM RegistryValues WHERE Parent = %lld;",uKey);
	
	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
			listValues.push_back(ptrStmt->column_text(0));

	} while (err == SQLITE_ROW);

	return (err == SQLITE_DONE ? 0 : EIO);
}

void Root::RegistryHive::enum_values(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response << ENOENT;
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response << EIO;
		return;
	}
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
		{
			response << acc;
			return;
		}
	}

	// Write out success first
	response << (int)0;
	if (!response.good_bit())
		return;

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Name FROM RegistryValues WHERE Parent = %lld;",uKey);

	do
	{
		err = ptrStmt->step();
		if (err == SQLITE_ROW)
		{
			if (!response.write_string(ptrStmt->column_text(0)))
			{
				response.reset();
				response << ACE_OS::last_error();
				return;
			}
		}
	} while (err == SQLITE_ROW);

	if (err == SQLITE_DONE)
	{
		// Write terminating null
		response.write_string(0);
	}
	else
	{
		response.reset();
		response << EIO;
	}
}

int Root::RegistryHive::delete_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("DELETE FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::get_value_type(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CDR::Octet& type)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Type FROM RegistryValues WHERE Name = %Q AND Parent = %lld;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		type = static_cast<ACE_CDR::Octet>(ptrStmt->column_int(0));
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Root::RegistryHive::get_string_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CString& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld AND Type=0;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		val = ptrStmt->column_text(0);
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

int Root::RegistryHive::get_integer_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CDR::LongLong& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld AND Type=1;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		val = ptrStmt->column_int64(0);
		return 0;
	}
	else if (err == SQLITE_DONE)
		return ENOENT;
	else
		return EIO;
}

void Root::RegistryHive::get_binary_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong cbLen, ACE_CDR::ULong channel_id, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
	{
		response << ENOENT;
		return;
	}
	else if (err != SQLITE_ROW)
	{
		response << EIO;
		return;
	}
		
	if (!(access_mask & 1) && channel_id != 0)
	{
		// Read not allowed - check access!
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
		{
			response << acc;
			return;
		}
	}

	// Write out success first
	response << (int)0;
	if (!response.good_bit())
		return;

	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("SELECT Value FROM RegistryValues WHERE Name = %Q AND Parent = %lld AND Type=2;",strValue.c_str(),uKey);

	err = ptrStmt->step();
	if (err == SQLITE_ROW)
	{
		const void* pData = ptrStmt->column_blob(0);
		int len = ptrStmt->column_bytes(0);

		bool bData = false;
		if (cbLen)
		{
			bData = true;
			cbLen = std::min(static_cast<ACE_CDR::ULong>(len),cbLen);
		}
		else
			cbLen = static_cast<ACE_CDR::ULong>(len);

		if (!response.write_ulong(cbLen) || 
			(bData && !response.write_octet_array(static_cast<const ACE_CDR::Octet*>(pData),cbLen)))
		{
			response.reset();
			response << ACE_OS::last_error();
		}

		return;
	}
	
	if (err == SQLITE_DONE)
		err = ENOENT;
	else
		err = EIO;

	response.reset();
	response << err;
}

int Root::RegistryHive::set_string_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const char* val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("INSERT OR REPLACE INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,0,%Q);",strValue.c_str(),uKey,val);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::set_integer_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const ACE_CDR::LongLong& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("INSERT OR REPLACE INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,1,%lld);",strValue.c_str(),uKey,val);
	
	err = ptrStmt->step();
	if (err != SQLITE_DONE)
		return EIO;
	
	return 0;
}

int Root::RegistryHive::set_binary_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const ACE_InputCDR& request)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

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
		int acc = Root::Manager::registry_access_check(channel_id);
		if (acc != 0)
			return acc;
	}

	// Insert the new value
	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db->prepare_statement("INSERT OR REPLACE INTO RegistryValues (Name,Parent,Type,Value) VALUES (%Q,%lld,2,?);",strValue.c_str(),uKey);
		
	err = sqlite3_bind_blob(ptrStmt->statement(),1,request.start()->rd_ptr(),static_cast<int>(request.length()),SQLITE_STATIC);
	if (err != SQLITE_OK)
		ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_blob failed: %C\n",sqlite3_errmsg(m_db->database())));
	else
		err = ptrStmt->step();
	
	return (err == SQLITE_DONE) ? 0 : EIO;
}

#ifdef ACE_WIN32
int Root::RegistryHive::get_string_value(const ACE_INT64& uKey, const ACE_WString& strValue, ACE_WString& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	sqlite3_stmt* pStmt;
	err = sqlite3_prepare16_v2(m_db->database(),L"SELECT Value FROM RegistryValues WHERE Name = ? AND Parent = ? AND Type=0;",-1,&pStmt,NULL);
	if (err != SQLITE_OK)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: sqlite3_prepare_v2 failed: %C\n",sqlite3_errmsg(m_db->database())),EIO);

	err = sqlite3_bind_text16(pStmt,1,strValue.c_str(),-1,SQLITE_STATIC);
	if (err != SQLITE_OK)
		ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_text failed: %C\n",sqlite3_errmsg(m_db->database())));
	else
	{
		err = sqlite3_bind_int64(pStmt,2,static_cast<sqlite3_int64>(uKey));
		if (err != SQLITE_OK)
			ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_int64 failed: %C\n",sqlite3_errmsg(m_db->database())));
		else
		{
			err = sqlite3_step(pStmt);
			if (err == SQLITE_ROW)
				val = static_cast<const wchar_t*>(sqlite3_column_text16(pStmt,0));
			else if (err != SQLITE_DONE)
				ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_step failed: %C\n",sqlite3_errmsg(m_db->database())));
		}
	}
	
	sqlite3_finalize(pStmt);

	if (err == SQLITE_DONE)
		return ENOENT;
	else
		return (err == SQLITE_ROW ? 0 : EIO);
}

int Root::RegistryHive::set_string_value(const ACE_INT64& uKey, const ACE_WString& strValue, const ACE_WString& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	// Check if the key still exists
	int access_mask = 0;
	int err = check_key_exists(uKey,access_mask);
	if (err == SQLITE_DONE)
		return ENOENT;
	else if (err != SQLITE_ROW)
		return EIO;

	// Insert the new key
	sqlite3_stmt* pStmt;
	err = sqlite3_prepare16_v2(m_db->database(),L"INSERT OR REPLACE INTO RegistryValues (Name,Parent,Type,Value) VALUES (?,?,0,?);",-1,&pStmt,NULL);
	if (err != SQLITE_OK)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: sqlite3_prepare_v2 failed: %C\n",sqlite3_errmsg(m_db->database())),EIO);
	
	err = sqlite3_bind_text16(pStmt,1,strValue.c_str(),-1,SQLITE_STATIC);
	if (err != SQLITE_OK)
		ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_text failed: %C\n",sqlite3_errmsg(m_db->database())));
	else
	{
		err = sqlite3_bind_int64(pStmt,2,static_cast<sqlite3_int64>(uKey));
		if (err != SQLITE_OK)
			ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_int64 failed: %C\n",sqlite3_errmsg(m_db->database())));
		else
		{
			err = sqlite3_bind_text16(pStmt,3,val.c_str(),-1,SQLITE_STATIC);
			if (err != SQLITE_OK)
				ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_bind_text failed: %C\n",sqlite3_errmsg(m_db->database())));
			else
			{
				err = sqlite3_step(pStmt);
				if (err != SQLITE_DONE)
					ACE_ERROR((LM_ERROR,L"%N:%l: sqlite3_step failed: %C\n",sqlite3_errmsg(m_db->database())));
			}
		}
	}
	
	sqlite3_finalize(pStmt);

	return (err == SQLITE_DONE) ? 0 : EIO;
}
#endif
