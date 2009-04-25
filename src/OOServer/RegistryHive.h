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

#ifndef OOSERVER_REGISTRY_HIVE_H_INCLUDED_
#define OOSERVER_REGISTRY_HIVE_H_INCLUDED_

#include "OOServer_Root.h"
#include "Database.h"

namespace Root
{
	class Manager;

	class RegistryHive
	{
	public:
		RegistryHive(Manager* pManager, OOBase::SmartPtr<Db::Database>& db);

		bool open();

		int open_key(Omega::int64_t& uKey, std::string strSubKey, Omega::uint32_t channel_id);
		int create_key(Omega::int64_t& uKey, std::string strSubKey, bool bFailIfThere, int access, Omega::uint32_t channel_id);
		int delete_key(Omega::int64_t uKey, std::string strSubKey, Omega::uint32_t channel_id);
		int enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::list<std::string>& listSubKeys);
		void enum_subkeys(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::list<std::string>& listValues);
		void enum_values(const Omega::int64_t& uKey, Omega::uint32_t channel_id, OOBase::CDRStream& response);
		int delete_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id);

		int get_value_type(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::byte_t& type);
		int get_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val);
		int get_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, Omega::int64_t& val);
		void get_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t cbLen, Omega::uint32_t channel_id, OOBase::CDRStream& response);

		int set_string_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const char* val);
		int set_integer_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const Omega::int64_t& val);
		int set_binary_value(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const OOBase::CDRStream& request);

		int get_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, std::string& val);
		int get_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, std::string& val);
		int set_description(const Omega::int64_t& uKey, Omega::uint32_t channel_id, const std::string& val);
		int set_value_description(const Omega::int64_t& uKey, const std::string& strValue, Omega::uint32_t channel_id, const std::string& val);

	private:
		Manager*                       m_pManager;
		OOBase::Mutex                  m_lock;
		OOBase::SmartPtr<Db::Database> m_db;

		RegistryHive(const RegistryHive&) {}
		RegistryHive& operator = (const RegistryHive&) { return *this; }

		int find_key(Omega::int64_t uParent, const std::string& strSubKey, Omega::int64_t& uKey, int& access_mask);
		int find_key(Omega::int64_t& uKey, std::string& strSubKey, int& access_mask, Omega::uint32_t channel_id);
		int insert_key(Omega::int64_t& uKey, std::string strSubKey, int access_mask);
		int check_key_exists(const Omega::int64_t& uKey, int& access_mask);
		int delete_key_i(const Omega::int64_t& uKey, Omega::uint32_t channel_id);
	};
}

#endif // OOSERVER_REGISTRY_HIVE_H_INCLUDED_
