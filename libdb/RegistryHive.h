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

#ifndef OOCORE_LIBDB_REGISTRY_HIVE_H_INCLUDED_
#define OOCORE_LIBDB_REGISTRY_HIVE_H_INCLUDED_

#include "Database.h"

namespace Db
{
	class Manager;

	enum access_rights
	{
		read_check = 1,
		write_check = 2,

		// Not used for access checks
		protect_key = 4
	};
	typedef Omega::uint16_t access_rights_t;

	enum hive_errors
	{
		HIVE_OK = 0,
		HIVE_ERRORED,
		HIVE_NOTFOUND,
		HIVE_ALREADYEXISTS,
		HIVE_READONLY,
		HIVE_NOREAD,
		HIVE_NOWRITE,
		HIVE_PROTKEY,
		HIVE_BADNAME,
		HIVE_LINK
	};
	
	class Hive
	{
	public:
		Hive(Manager* pManager, const char* db_name);

		bool open(int flags);

		typedef OOBase::Stack<OOBase::String,OOBase::LocalAllocator> registry_set_t;

		hive_errors create_key(Omega::int64_t uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, Omega::uint16_t flags, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName);
		hive_errors delete_key(const Omega::int64_t& uKey, OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName);
		hive_errors enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys);
		void enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		hive_errors value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		hive_errors enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues);
		void enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		hive_errors delete_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		hive_errors get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val);
		hive_errors set_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, const char* val);
		
	private:
		Manager*               m_pManager;
		OOBase::SpinLock       m_lock;
		OOBase::String         m_strdb;
		
		OOBase::SmartPtr<Database> m_db;

		// Stashed prepared statements...
		Statement m_InsertKey_Stmt;
		Statement m_InsertValue_Stmt;
		Statement m_UpdateValue_Stmt;
		Statement m_CheckKey_Stmt;
		Statement m_GetKeyInfo_Stmt;
		Statement m_EnumKeyIds_Stmt;
		Statement m_EnumKeys_Stmt;
		Statement m_EnumValues_Stmt;
		Statement m_GetValue_Stmt;
		Statement m_DeleteKeys_Stmt;
		Statement m_DeleteKey_Stmt;
		Statement m_DeleteValue_Stmt;

		Hive(const Hive&);
		Hive& operator = (const Hive&);

		hive_errors get_value_i(const Omega::int64_t& uKey, const char* pszValue, OOBase::LocalString& val);
		hive_errors get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const OOBase::LocalString& strSubKey, Omega::uint32_t channel_id, access_rights_t& access_mask, OOBase::LocalString& strLink);
		hive_errors find_key(Omega::int64_t uParent, Omega::int64_t& uKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id, OOBase::LocalString& strLink, OOBase::LocalString& strFullKeyName);
		hive_errors insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask);
		hive_errors check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask);
		hive_errors delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id);
		hive_errors value_exists_i(const Omega::int64_t& uKey, const char* pszValue);
		hive_errors set_value_i(const Omega::int64_t& uKey, const char* pszName, const char* pszValue);
		void prepare_statement(Statement& stmt, const char* pszSql);
		hive_errors access_check(Omega::uint32_t channel_id, access_rights_t access_mask, access_rights_t check);
	};

	class Manager
	{
	public:
		virtual ~Manager() {}
		virtual bool registry_access_check(const char* pszDb, Omega::uint32_t channel_id, access_rights_t access_mask, int& err) = 0;
	};
}

#endif // OOCORE_LIBDB_REGISTRY_HIVE_H_INCLUDED_
