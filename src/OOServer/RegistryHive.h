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

#ifndef OOSERVER_REGISTRY_HIVE_H_INCLUDED_
#define OOSERVER_REGISTRY_HIVE_H_INCLUDED_

#include "./OOServer_Root.h"
#include "./Database.h"

namespace Root
{

class RegistryHive
{
public:
	RegistryHive(ACE_Refcounted_Auto_Ptr<Db::Database,ACE_Null_Mutex>& db);

	int open();

	int open_key(ACE_INT64& uKey, ACE_CString strSubKey, ACE_CDR::ULong channel_id);
	int create_key(ACE_INT64& uKey, ACE_CString strSubKey, bool bFailIfThere, int access, ACE_CDR::ULong channel_id);
	int delete_key(ACE_INT64 uKey, ACE_CString strSubKey, ACE_CDR::ULong channel_id);
	int enum_subkeys(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, std::list<ACE_CString>& listSubKeys);
	void enum_subkeys(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, ACE_OutputCDR& response);
	int enum_values(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, std::list<ACE_CString>& listValues);
	void enum_values(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, ACE_OutputCDR& response);
	int delete_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id);

	int get_value_type(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CDR::Octet& type);
	int get_string_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CString& val);
	int get_integer_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CDR::LongLong& val);
	void get_binary_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong cbLen, ACE_CDR::ULong channel_id, ACE_OutputCDR& response);

	int set_string_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const char* val);
	int set_integer_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const ACE_CDR::LongLong& val);
	int set_binary_value(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const ACE_InputCDR& request);

	int get_description(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, ACE_CString& val);
	int get_value_description(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, ACE_CString& val);
	int set_description(const ACE_INT64& uKey, ACE_CDR::ULong channel_id, const ACE_CString& val);
	int set_value_description(const ACE_INT64& uKey, const ACE_CString& strValue, ACE_CDR::ULong channel_id, const ACE_CString& val);

#ifdef ACE_WIN32
	int get_string_value(const ACE_INT64& uKey, const ACE_WString& strValue, ACE_WString& val);
	int set_string_value(const ACE_INT64& uKey, const ACE_WString& strValue, const ACE_WString& val);
#endif

private:
	ACE_Thread_Mutex m_lock;
	ACE_Refcounted_Auto_Ptr<Db::Database,ACE_Null_Mutex> m_db;

	RegistryHive(const RegistryHive&) {}
	RegistryHive& operator = (const RegistryHive&) { return *this; }

	int find_key(ACE_INT64 uParent, const ACE_CString& strSubKey, ACE_INT64& uKey, int& access_mask);
	int find_key(ACE_INT64& uKey, ACE_CString& strSubKey, int& access_mask, ACE_CDR::ULong channel_id);
	int insert_key(ACE_INT64& uKey, ACE_CString strSubKey, int access_mask);
	int check_key_exists(const ACE_INT64& uKey, int& access_mask);
	int delete_key_i(const ACE_INT64& uKey, ACE_CDR::ULong channel_id);
};

}

#endif // OOSERVER_REGISTRY_HIVE_H_INCLUDED_

