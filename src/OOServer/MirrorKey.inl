///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

void User::Registry::MirrorKey::Init(const string_t& strKey, IKey* pLocal, IKey* pSystem)
{
	m_strKey = strKey;
	m_ptrLocal = pLocal;
	m_ptrSystem = pSystem;
}

bool_t User::Registry::MirrorKey::IsSubKey(const string_t& strSubKey)
{
	return ((m_ptrLocal && m_ptrLocal->IsSubKey(strSubKey)) ||
			(m_ptrSystem && m_ptrSystem->IsSubKey(strSubKey)));
}

bool_t User::Registry::MirrorKey::IsValue(const string_t& strName)
{
	return ((m_ptrLocal && m_ptrLocal->IsValue(strName)) ||
			(m_ptrSystem && m_ptrSystem->IsValue(strName)));
}

string_t User::Registry::MirrorKey::GetStringValue(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetStringValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetStringValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(strName);
	return string_t();
}

int64_t User::Registry::MirrorKey::GetIntegerValue(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetIntegerValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetIntegerValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(strName);
	return 0;
}

void User::Registry::MirrorKey::GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetBinaryValue(strName,cbLen,pBuffer);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetBinaryValue(strName,cbLen,pBuffer);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(strName);
}

void User::Registry::MirrorKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	if (!m_ptrLocal)
		m_ptrLocal = IKey::OpenKey(m_strKey,IKey::OpenCreate);

	m_ptrLocal->SetStringValue(strName,strValue);
}

void User::Registry::MirrorKey::SetIntegerValue(const string_t& strName, const int64_t& uValue)
{
	if (!m_ptrLocal)
		m_ptrLocal = IKey::OpenKey(m_strKey,IKey::OpenCreate);

	m_ptrLocal->SetIntegerValue(strName,uValue);
}

void User::Registry::MirrorKey::SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val)
{
	if (!m_ptrLocal)
		m_ptrLocal = IKey::OpenKey(m_strKey,IKey::OpenCreate);

	m_ptrLocal->SetBinaryValue(strName,cbLen,val);
}

string_t User::Registry::MirrorKey::GetDescription()
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetDescription();
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetDescription();
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(m_strKey);
	return string_t();
}

string_t User::Registry::MirrorKey::GetValueDescription(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetValueDescription(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetValueDescription(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(strName);
	return string_t();
}

void User::Registry::MirrorKey::SetDescription(const string_t& strValue)
{
	if (!m_ptrLocal)
		User::Registry::AccessDeniedException::Throw(m_strKey);

	try
	{
		m_ptrLocal->SetDescription(strValue);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
		User::Registry::AccessDeniedException::Throw(m_strKey);
	}
}

void User::Registry::MirrorKey::SetValueDescription(const string_t& strName, const string_t& strValue)
{
	if (!m_ptrLocal)
		User::Registry::AccessDeniedException::Throw(m_strKey);

	try
	{
		m_ptrLocal->SetValueDescription(strName,strValue);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
		User::Registry::AccessDeniedException::Throw(m_strKey);
	}
}

ValueType_t User::Registry::MirrorKey::GetValueType(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetValueType(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetValueType(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	User::Registry::NotFoundException::Throw(strName);
	return 0;
}

IKey* User::Registry::MirrorKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	ObjectPtr<IKey> ptrNewLocal;
	ObjectPtr<IKey> ptrNewSystem;
	if (m_ptrLocal)
	{
		try
		{
			ptrNewLocal = m_ptrLocal.OpenSubKey(strSubKey,flags);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			ptrNewSystem = m_ptrSystem.OpenSubKey(strSubKey,IKey::OpenExisting);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (!ptrNewLocal && !ptrNewSystem)
		User::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey);

	ObjectPtr<ObjectImpl<MirrorKey> > ptrNew = ObjectImpl<MirrorKey>::CreateInstancePtr();
	ptrNew->Init(m_strKey + L"\\" + strSubKey,ptrNewLocal,ptrNewSystem);
	return ptrNew.AddRef();
}

std::set<string_t> User::Registry::MirrorKey::EnumSubKeys()
{
	std::set<string_t> ret;
	if (m_ptrLocal)
		ret = m_ptrLocal->EnumSubKeys();

	if (m_ptrSystem)
	{
		std::set<string_t> ret2 = m_ptrSystem->EnumSubKeys();
		ret.insert(ret2.begin(),ret2.end());
	}

	return ret;
}

std::set<string_t> User::Registry::MirrorKey::EnumValues()
{
	std::set<string_t> ret;
	if (m_ptrLocal)
		ret = m_ptrLocal->EnumValues();

	if (m_ptrSystem)
	{
		std::set<string_t> ret2 = m_ptrSystem->EnumValues();
		ret.insert(ret2.begin(),ret2.end());
	}

	return ret;
}

void User::Registry::MirrorKey::DeleteKey(const string_t& strSubKey)
{
	if (!m_ptrLocal)
		User::Registry::AccessDeniedException::Throw(m_strKey);

	try
	{
		m_ptrLocal->DeleteKey(strSubKey);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
		User::Registry::AccessDeniedException::Throw(m_strKey);
	}
}

void User::Registry::MirrorKey::DeleteValue(const string_t& strName)
{
	if (!m_ptrLocal)
		User::Registry::AccessDeniedException::Throw(m_strKey);

	try
	{
		m_ptrLocal->DeleteValue(strName);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
		User::Registry::AccessDeniedException::Throw(m_strKey);
	}
}
