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

#ifndef OOSERVER_REGISTRY_HIVE_H_INCLUDED_
#define OOSERVER_REGISTRY_HIVE_H_INCLUDED_

#include "OOServer_Root.h"
#include "Database.h"

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

		Hive(Manager* pManager, const std::string& strdb, access_rights_t default_permissions);

		bool open(int flags);

		int open_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, std::string strSubKey, Omega::uint32_t channel_id);
		int create_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, std::string strSubKey, Omega::uint16_t flags, access_rights_t access, Omega::uint32_t channel_id);
		int delete_key(const Omega::int64_t& uKey, std::string strSubKey, Omega::uint32_t channel_id);
		int enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::set<std::string>& setSubKeys);
		void enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::set<std::string>& setValues);
		void enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int delete_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id);

		int get_value_type(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::byte_t& type);
		int get_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val);
		int get_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::int64_t& val);
		void get_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::uint32_t cbLen, OOBase::CDRStream& response);
		int get_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::uint32_t& cbLen, Omega::byte_t* buf);

		int set_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const char* val);
		int set_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const Omega::int64_t& val);
		int set_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const OOBase::CDRStream& request);
		int set_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::uint32_t cbLen, const Omega::byte_t* buf);

		int get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::string& val);
		int get_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val);
		int set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const std::string& val);
		int set_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const std::string& val);

	private:
		Manager*                       m_pManager;
		OOBase::Mutex                  m_lock;
		std::string                    m_strdb;
		OOBase::SmartPtr<Db::Database> m_db;
		access_rights_t                m_default_permissions;

		// Stashed prepared statements...
		OOBase::SmartPtr<Db::Statement> m_ptrCheckKey_Stmt;
		OOBase::SmartPtr<Db::Statement> m_ptrGetKeyInfo_Stmt;

		Hive(const Hive&);
		Hive& operator = (const Hive&);

		int get_key_info(const Omega::int64_t& uParent, Omega::int64_t& uKey, const std::string& strSubKey, access_rights_t& access_mask);
		int find_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, std::string& strSubKey, access_rights_t& access_mask, Omega::uint32_t channel_id);
		int insert_key(const Omega::int64_t& uParent, Omega::int64_t& uKey, const std::string& strSubKey, access_rights_t access_mask);
		int check_key_exists(const Omega::int64_t& uKey, access_rights_t& access_mask);
		int delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id);
	};

	class Manager
	{
	public:
		virtual int registry_access_check(const std::string& strdb, Omega::uint32_t channel_id, Hive::access_rights_t access_mask) = 0;
	};
}

#endif // OOSERVER_REGISTRY_HIVE_H_INCLUDED_
