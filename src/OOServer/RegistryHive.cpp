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

Root::RegistryHive::RegistryHive() : ACE_Configuration_Heap()
{
}

int Root::RegistryHive::open(const ACE_WString& strHive)
{
	static char* count = (char*)0x40000000;
	count += 0x04000000;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	return ACE_Configuration_Heap::open(strHive.c_str(),count);
}

int Root::RegistryHive::create_key(const ACE_WString& strKey, bool bFailIfThere)
{
	if (strKey.empty())
		return bFailIfThere ? EALREADY : 0;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	if (bFailIfThere)
	{
		// Check to see if the key already exists by opening it...
		ACE_Configuration_Section_Key sub_key;
		if (open_section(root_section(),strKey.c_str(),0,sub_key) == 0)
			return EALREADY;
	}

	ACE_Configuration_Section_Key sub_key;
	if (open_section(root_section(),strKey.c_str(),1,sub_key) != 0)
		return ACE_OS::last_error();
	
	return 0;
}

int Root::RegistryHive::delete_key(const ACE_WString& strKey, const ACE_WString& strSubKey)
{
	if (strSubKey.empty())
		return EACCES;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (remove_section(key,strSubKey.c_str(),1) != 0)
		return ACE_OS::last_error();
	
	return 0;
}

int Root::RegistryHive::key_exists(const ACE_WString& strKey)
{
	if (strKey.empty())
		return 0;

	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key sub_key;
	if (open_section(root_section(),strKey.c_str(),0,sub_key) != 0)
		return ACE_OS::last_error();
	
	return 0;
}

int Root::RegistryHive::enum_subkeys(const ACE_WString& strKey, std::list<ACE_WString>& listSubKeys)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	try
	{
		for (int index=0;;++index)
		{
			ACE_WString strSubKey;
			int err = enumerate_sections(key,index,strSubKey);
			if (err == 0)
				listSubKeys.push_back(strSubKey.c_str());
			else if (err == 1)
				return 0;
			else
				return ACE_OS::last_error();
		}
	}
	catch (std::exception&)
	{
		return ENOMEM;
	}
}

void Root::RegistryHive::enum_subkeys(const ACE_WString& strKey, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	// Write out success first...
	int err = 0;
	response << err;
	if (!response.good_bit())
		return;
		
	for (int index=0;!err;++index)
	{
		ACE_WString strSubKey;
		int ret = enumerate_sections(key,index,strSubKey);
		if (ret == 0)
		{
			if (!response.write_wstring(strSubKey.c_str()))
				err = ACE_OS::last_error();
		}
		else if (ret == 1)
			break;
		else		
			err = ACE_OS::last_error();
	}

	if (err == 0)
	{
		if (!response.write_wstring(0))
			err = ACE_OS::last_error();
	}
	
	// Update err code
	if (err != 0)
	{
		response.reset();
		response << err;
	}
}

int Root::RegistryHive::enum_values(const ACE_WString& strKey, std::list<ACE_WString>& listValues)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	try
	{
		for (int index=0;;++index)
		{
			ACE_WString strValue;
			ACE_Configuration_Heap::VALUETYPE type;
			int err = enumerate_values(key,index,strValue,type);
			if (err == 0)
				listValues.push_back(strValue.c_str());
			else if (err == 1)
				return 0;
			else
				return ACE_OS::last_error();
		}
	}
	catch (std::exception&)
	{
		return ENOMEM;
	}
}

void Root::RegistryHive::enum_values(const ACE_WString& strKey, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	// Write out success first...
	int err = 0;
	response << err;
	if (!response.good_bit())
		return;
		
	for (int index=0;!err;++index)
	{
		ACE_WString strValue;
		ACE_Configuration_Heap::VALUETYPE type;
		int ret = enumerate_values(key,index,strValue,type);
		if (ret == 0)
		{
			if (!response.write_wstring(strValue.c_str()))
				err = ACE_OS::last_error();
		}
		else if (ret == 1)
			break;
		else		
			err = ACE_OS::last_error();
	}

	if (err == 0)
	{
		if (!response.write_wstring(0))
			err = ACE_OS::last_error();
	}
	
	// Update err code
	if (err != 0)
	{
		response.reset();
		response << err;
	}
}

int Root::RegistryHive::delete_value(const ACE_WString& strKey, const ACE_WString& strValue)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (remove_value(key,strValue.c_str()) != 0)
		return ACE_OS::last_error();
	
	return 0;
}

int Root::RegistryHive::get_value_type(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::Octet& type)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	ACE_Configuration_Heap::VALUETYPE vtype;
	if (find_value(key,strValue.c_str(),vtype) != 0)
		return ACE_OS::last_error();

	type = static_cast<ACE_CDR::Octet>(vtype);
	
	return 0;
}

int Root::RegistryHive::get_string_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_WString& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (ACE_Configuration_Heap::get_string_value(key,strValue.c_str(),val) != 0)
		return ACE_OS::last_error();
		
	return 0;
}

int Root::RegistryHive::get_integer_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong& val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (ACE_Configuration_Heap::get_integer_value(key,strValue.c_str(),val) != 0)
		return ACE_OS::last_error();
		
	return 0;
}

void Root::RegistryHive::get_binary_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong cbLen, ACE_OutputCDR& response)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
    if (guard.locked() == 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	void* data = 0;
	size_t len = 0;
	if (ACE_Configuration_Heap::get_binary_value(key,strValue.c_str(),data,len) != 0)
	{
		response << ACE_OS::last_error();
		return;
	}

	bool bData = true;
	if (cbLen)
		cbLen = std::min(static_cast<ACE_CDR::ULong>(len),cbLen);
	else
	{
		bData = false;
		cbLen = static_cast<ACE_CDR::ULong>(len);
	}

	// Write out success first...
	response << (int)0;
	if (response.good_bit())
	{
		if (!response.write_ulong(cbLen) || 
			(bData && !response.write_octet_array(static_cast<ACE_CDR::Octet*>(data),cbLen)))
		{
			response.reset();
			response << ACE_OS::last_error();
		}
	}
	
	delete [] data;
}

int Root::RegistryHive::set_string_value(const ACE_WString& strKey, const ACE_WString& strValue, const wchar_t* val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (ACE_Configuration_Heap::set_string_value(key,strValue.c_str(),val) != 0)
		return ACE_OS::last_error();
		
	return 0;
}

int Root::RegistryHive::set_integer_value(const ACE_WString& strKey, const ACE_WString& strValue, ACE_CDR::ULong val)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (ACE_Configuration_Heap::set_integer_value(key,strValue.c_str(),val) != 0)
		return ACE_OS::last_error();
		
	return 0;
}

int Root::RegistryHive::set_binary_value(const ACE_WString& strKey, const ACE_WString& strValue, const ACE_InputCDR& request)
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex,guard,m_lock,ACE_OS::last_error());

	ACE_Configuration_Section_Key key = root_section();
	if (!strKey.empty() && open_section(root_section(),strKey.c_str(),0,key) != 0)
		return ACE_OS::last_error();

	if (ACE_Configuration_Heap::set_binary_value(key,strValue.c_str(),request.start()->rd_ptr(),request.length()) != 0)
		return ACE_OS::last_error();
		
	return 0;
}
