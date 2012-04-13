///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#include "OOServer_User.h"

#include "UserRegistry.h"
#include "UserManager.h"

using namespace Omega;
using namespace Omega::Registry;
using namespace OTL;

#include "RegistryCmn.h"

#include <iterator>

using namespace User;
using namespace User::Registry;

namespace
{
	class OverlayKey :
			public ObjectBase,
			public IProvideObjectInfoImpl<OverlayKey>,
			public IKey
	{
	public:
		void init(IKey* pOver, IKey* pUnder);

		BEGIN_INTERFACE_MAP(OverlayKey)
			INTERFACE_ENTRY(IKey)
			INTERFACE_ENTRY(TypeInfo::IProvideObjectInfo)
		END_INTERFACE_MAP()

	private:
		ObjectPtr<IKey> m_ptrOver;
		ObjectPtr<IKey> m_ptrUnder;

	// IKey members
	public:
		string_t GetName();
		bool_t IsSubKey(const string_t& strSubKey);
		std::set<string_t> EnumSubKeys();
		IKey* OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		void DeleteSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		std::set<string_t> EnumValues();
		any_t GetValue(const string_t& strName);
		void SetValue(const string_t& strName, const any_t& value);
		void DeleteValue(const string_t& strName);
	};

	void ThrowKeyNotFound(const string_t& strKey)
	{
		throw INotFoundException::Create(string_t::constant("The registry key {0} does not exist") % strKey);
	}

	void ThrowValueNotFound(const string_t& strValue)
	{
		throw INotFoundException::Create(string_t::constant("The registry value {0} does not exist") % strValue);
	}

	void ThrowAlreadyExists(const string_t& strKey)
	{
		throw IAlreadyExistsException::Create(string_t::constant("The registry key {0} already exists") % strKey);
	}
}

void RootKey::init(Manager* pManager, const string_t& strKey, const int64_t& key, byte_t type)
{
	m_pManager = pManager;
	m_strKey = strKey;
	m_key = key;
	m_type = type;
}

string_t RootKey::GetName()
{
	return m_strKey;
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OpenKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.c_str());
	request.write(static_cast<IKey::OpenFlags_t>(IKey::OpenExisting));
	
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += "/";
	strFullKey += strSubKey;
	
	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		AccessDeniedException::Throw(strFullKey);
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t RootKey::IsValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::ValueExists));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());
	
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

any_t RootKey::GetValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == ENOENT)
		ThrowValueNotFound(GetName() + "/" + strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	OOBase::LocalString strValue;
	if (!response.read(strValue))
		OMEGA_THROW(response.last_error());

	return strValue.c_str();
}

void RootKey::SetValue(const string_t& strName, const any_t& value)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SetValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());
	request.write(value.cast<string_t>().c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == ENOENT)
		ThrowValueNotFound(GetName() + "/" + strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* RootKey::OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	BadNameException::ValidateSubKey(strSubKey);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OpenKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.c_str());
	request.write(flags);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += "/";
	strFullKey += strSubKey;
	
	if (err==EACCES)
		AccessDeniedException::Throw(strFullKey);
	else if (err==EEXIST)
		ThrowAlreadyExists(strFullKey);
	else if (err==ENOENT)
		ThrowKeyNotFound(strFullKey);
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	int64_t key = 0;
	byte_t type = 255;
	if (!response.read(key) || !response.read(type))
		OMEGA_THROW(response.last_error());

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<RootKey> > ptrNew = ObjectImpl<RootKey>::CreateInstance();
	ptrNew->init(m_pManager,strFullKey,key,type);
	return ptrNew.Detach();
}

std::set<string_t> RootKey::EnumSubKeys()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::EnumSubKeys));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		ThrowKeyNotFound(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	try
	{
		std::set<string_t> sub_keys;
		for (;;)
		{
			OOBase::LocalString strName;
			if (!response.read(strName))
				OMEGA_THROW(response.last_error());

			if (strName.empty())
				break;

			sub_keys.insert(strName.c_str());

			if (m_key == 0 && m_type == 0)
			{
				// Add the local user key, although it doesn't really exist...
				sub_keys.insert(string_t::constant("Local User"));
			}
		}
		return sub_keys;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

std::set<string_t> RootKey::EnumValues()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::EnumValues));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		ThrowKeyNotFound(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	try
	{
		std::set<string_t> values;
		for (;;)
		{
			OOBase::LocalString strName;
			if (!response.read(strName))
				OMEGA_THROW(response.last_error());

			if (strName.empty())
				break;

			values.insert(strName.c_str());
		}
		return values;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

void RootKey::DeleteSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::DeleteSubKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err != 0)
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += "/";
		strFullKey += strSubKey;

		if (err == ENOENT)
			ThrowKeyNotFound(strFullKey);
		else if (err==EACCES)
			AccessDeniedException::Throw(strFullKey);
		else if (err==EIO)
			OMEGA_THROW("Unexpected registry error");
		else
			OMEGA_THROW(err);
	}
}

void RootKey::DeleteValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::DeleteValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	int32_t err = 0;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == ENOENT)
		ThrowValueNotFound(GetName() + "/" + strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

void OverlayKey::init(IKey* pOver, IKey* pUnder)
{
	m_ptrOver = pOver;
	m_ptrOver.AddRef();

	m_ptrUnder = pUnder;
	m_ptrUnder.AddRef();
}

string_t OverlayKey::GetName()
{
	return m_ptrOver->GetName();
}

bool_t OverlayKey::IsSubKey(const string_t& strSubKey)
{
	return (m_ptrOver->IsSubKey(strSubKey) || m_ptrUnder->IsSubKey(strSubKey));
}

bool_t OverlayKey::IsValue(const string_t& strName)
{
	return (m_ptrOver->IsValue(strName) || m_ptrUnder->IsValue(strName));
}

any_t OverlayKey::GetValue(const string_t& strName)
{
	if (m_ptrOver->IsValue(strName))
		return m_ptrOver->GetValue(strName);

	return m_ptrUnder->GetValue(strName);
}

void OverlayKey::SetValue(const string_t& /*strName*/, const any_t& /*value*/)
{
	AccessDeniedException::Throw(GetName());
}

IKey* OverlayKey::OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += "/";
	strFullKey += strSubKey;

	if (flags == IKey::CreateNew)
		AccessDeniedException::Throw(strFullKey);

	ObjectPtr<IKey> ptrSubOver,ptrSubUnder;
	if (m_ptrOver->IsSubKey(strSubKey))
		ptrSubOver = m_ptrOver->OpenKey(strSubKey,IKey::OpenExisting);

	if (m_ptrUnder->IsSubKey(strSubKey))
		ptrSubUnder = m_ptrUnder->OpenKey(strSubKey,IKey::OpenExisting);

	if (!ptrSubOver && !ptrSubUnder)
		ThrowKeyNotFound(strFullKey);

	if (!ptrSubOver)
		return ptrSubUnder.Detach();
	else if (!ptrSubUnder)
		return ptrSubOver.Detach();

	ObjectPtr<ObjectImpl<OverlayKey> > ptrKey = ObjectImpl<OverlayKey>::CreateInstance();
	ptrKey->init(ptrSubOver,ptrSubUnder);
	return ptrKey.Detach();
}

std::set<string_t> OverlayKey::EnumSubKeys()
{
	std::set<string_t> over_set = m_ptrOver->EnumSubKeys();
	std::set<string_t> under_set = m_ptrUnder->EnumSubKeys();

	std::copy(under_set.begin(),under_set.end(),std::inserter(over_set,over_set.begin()));
	return over_set;
}

std::set<string_t> OverlayKey::EnumValues()
{
	std::set<string_t> over_set = m_ptrOver->EnumValues();
	std::set<string_t> under_set = m_ptrUnder->EnumValues();

	std::copy(under_set.begin(),under_set.end(),std::inserter(over_set,over_set.begin()));
	return over_set;
}

void OverlayKey::DeleteSubKey(const string_t& strSubKey)
{
	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += "/";
	strFullKey += strSubKey;

	AccessDeniedException::Throw(strFullKey);
}

void OverlayKey::DeleteValue(const string_t& /*strName*/)
{
	AccessDeniedException::Throw(GetName());
}

IKey* OverlayKeyFactory::Overlay(const string_t& strOver, const string_t& strUnder)
{
	ObjectPtr<IKey> ptrSubOver,ptrSubUnder;
	try
	{
		ptrSubOver = ObjectPtr<IKey>(strOver,IKey::OpenExisting);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
	}

	try
	{
		ptrSubUnder = ObjectPtr<IKey>(strUnder,IKey::OpenExisting);
	}
	catch (INotFoundException* pE)
	{
		pE->Release();
	}

	if (!ptrSubOver && !ptrSubUnder)
		ThrowKeyNotFound(strOver);

	if (!ptrSubOver)
		return ptrSubUnder.Detach();
	else if (!ptrSubUnder)
		return ptrSubOver.Detach();

	ObjectPtr<ObjectImpl<OverlayKey> > ptrKey = ObjectImpl<OverlayKey>::CreateInstance();
	ptrKey->init(ptrSubOver,ptrSubUnder);
	return ptrKey.Detach();
}
