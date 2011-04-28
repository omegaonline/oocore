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

#ifndef OOSERVER_REGISTRY_HIVE_H_INCLUDED_
#define OOSERVER_REGISTRY_HIVE_H_INCLUDED_

#include "OOServer_Root.h"

namespace Registry
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

		typedef OOBase::Stack<OOBase::String,OOBase::LocalAllocator<OOBase::NoFailure> > registry_set_t;

		int open_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id);
		int create_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint16_t flags, access_rights_t access, Omega::uint32_t channel_id);
		int delete_key(const Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id);
		int enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setSubKeys);
		void enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int value_exists(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		
		int get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::LocalString& val);
		int set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const char* val);
		
		int enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, registry_set_t& setValues);
		void enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int delete_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id);
		int get_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val);
		int set_value(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, const char* val);
		
		int get_value_description(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, OOBase::LocalString& val);
		int set_value_description(const Omega::int64_t& uKey, const char* pszValue, Omega::uint32_t channel_id, const char* val);

	private:
		Manager*               m_pManager;
		OOBase::SpinLock       m_lock;
		OOBase::String         m_strdb;
		
		OOBase::SmartPtr<OOSvrBase::Db::Database> m_db;

		// Stashed prepared statements...
		OOBase::SmartPtr<OOSvrBase::Db::Statement> m_ptrCheckKey_Stmt;
		OOBase::SmartPtr<OOSvrBase::Db::Statement> m_ptrGetKeyInfo_Stmt;
		OOBase::SmartPtr<OOSvrBase::Db::Statement> m_ptrGetValue_Stmt;

		Hive(const Hive&);
		Hive& operator = (const Hive&);

		int get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t& access_mask);
		int find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszKey, OOBase::LocalString& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id);
		int insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const char* pszSubKey, access_rights_t access_mask);
		int check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask);
		int delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id);
		int value_exists_i(const Omega::int64_t& uKey, const char* pszValue);
	};

	class Manager
	{
	public:
		virtual int registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Hive::access_rights_t access_mask) = 0;

	protected:
		virtual ~Manager() {}
	};
}

#endif // OOSERVER_REGISTRY_HIVE_H_INCLUDED_
