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

	class Hive
	{
	public:
		enum access_rights
		{
			read_check = 1,
			write_check = 2,
			never_delete = 4,
			inherit_checks = 8
		};
		typedef Omega::uint16_t access_rights_t;

		Hive(Manager* pManager, const char* db_name);

		bool open(int flags);

		typedef OOBase::Stack<OOBase::String,OOBase::LocalAllocator> registry_set_t;

		int open_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id);
		int create_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint16_t flags, access_rights_t access, Omega::uint32_t channel_id);
		int delete_key(const Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id);
		int enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys);
		void enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		int enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues);
		void enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int delete_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		int get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val);
		int set_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, const char* val);
		
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

		void get_access_mask(const Omega::int64_t& uKey, access_rights_t& access_mask);
		int get_value_i(const Omega::int64_t& uKey, const char* pszValue, OOBase::LocalString& val);
		int get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t& access_mask);
		int find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id);
		int insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask);
		int check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask);
		int delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id);
		int value_exists_i(const Omega::int64_t& uKey, const char* pszValue);
		bool prepare_statement(Statement& stmt, const char* pszSql);
		int set_value_i(const Omega::int64_t& uKey, const char* pszName, const char* pszValue);
	};

	class Manager
	{
	public:
		virtual int registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Hive::access_rights_t access_mask) = 0;
	};
}

#endif // OOCORE_LIBDB_REGISTRY_HIVE_H_INCLUDED_
