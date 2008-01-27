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

namespace Root
{

class RegistryHive : private ACE_Configuration_Heap
{
public:
	RegistryHive();

	int open(const ACE_WString& strHive);

	int create_key(const ACE_WString& strKey, bool bFailIfThere);
	int delete_key(const ACE_WString& strKey, const ACE_WString& strSubKey);
	int key_exists(const ACE_WString& strKey);
	int enum_subkeys(const ACE_WString& strKey, std::list<ACE_WString>& listSubKeys);
	void enum_subkeys(const ACE_WString& strKey, ACE_OutputCDR& response);
	int enum_values(const ACE_WString& strKey, std::list<ACE_WString>& listValues);
	void enum_values(const ACE_WString& strKey, ACE_OutputCDR& response);
	int delete_value(const ACE_WString& strKey, const ACE_WString& strValue);

	int get_value_type(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::Octet& type);
	int get_string_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_WString& val);
	int get_integer_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong& val);
	void get_binary_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong cbLen, ACE_OutputCDR& response);

	int set_string_value(const ACE_WString& strKey, const ACE_WString& strValue, const wchar_t* val);
	int set_integer_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong val);
	int set_binary_value(const ACE_WString& strKey, const ACE_WString& strValue, const ACE_InputCDR& request);

private:
	ACE_Thread_Mutex m_lock;

	RegistryHive(const RegistryHive&) {}
	RegistryHive& operator = (const RegistryHive&) { return *this; }
};

}

#endif // OOSERVER_REGISTRY_HIVE_H_INCLUDED_