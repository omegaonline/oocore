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

		void ThrowOverlay();

	// IKey members
	public:
		string_t GetName();
		bool_t IsKey(const string_t& strSubKey);
		IKey::string_set_t EnumSubKeys();
		IKey* OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		void DeleteSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		IKey::string_set_t EnumValues();
		any_t GetValue(const string_t& strName);
		void SetValue(const string_t& strName, const any_t& value);
		void DeleteValue(const string_t& strName);
	};

	void ThrowValueNotFound(const string_t& strKey, const string_t& strValue)
	{
		throw INotFoundException::Create(string_t::constant("The registry key '{0}' does not have a value named '{1}'") % strKey % strValue);
	}

	void ThrowCorrectException(OOServer::RootErrCode_t err, const string_t& strKey)
	{
		switch (err)
		{
		case OOServer::Ok:
			break;

		case OOServer::NotFound:
			throw INotFoundException::Create(string_t::constant("The registry key '{0}' does not exist") % strKey);

		case OOServer::AlreadyExists:
			throw IAlreadyExistsException::Create(string_t::constant("The registry key '{0}' already exists") % strKey);

		case OOServer::ReadOnlyHive:
			throw IAccessDeniedException::Create(string_t::constant("The registry database is read-only ({0})") % strKey);

		case OOServer::NoRead:
			throw IAccessDeniedException::Create(string_t::constant("You do not have read permissions for the registry key '{0}'") % strKey);

		case OOServer::NoWrite:
			throw IAccessDeniedException::Create(string_t::constant("You do not have write permissions for the registry key '{0}'") % strKey);

		case OOServer::ProtectedKey:
			throw IAccessDeniedException::Create(string_t::constant("The registry key '{0}' has the 'protect' bit set") % strKey);

		case OOServer::Errored:
		default:
			throw IInternalException::Create(string_t::constant("Internal registry failure.  Check server log for details"),strKey.c_str(),0,NULL,NULL);
		}
	}
}

void RootKey::init(const string_t& strKey, const int64_t& key, byte_t type)
{
	m_strKey = strKey;
	m_key = key;
	m_type = type;
}

string_t RootKey::GetName()
{
	return m_strKey;
}

OOServer::RootErrCode_t RootKey::open_key(const string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags, int64_t& key, byte_t& type, string_t& strFullKey)
{
	key = m_key;
	type = m_type;

	if (strSubKey[0] == '/')
	{
		key = 0;
		type = 0;
	}

	if (key != 0 && type != 0)
		strFullKey = m_strKey;

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_OpenKey));
	request.write(key);
	request.write(type);
	request.write(strSubKey.c_str());
	request.write(flags);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err != OOServer::Errored)
	{
		OOBase::LocalString strFullKeyName;
		if (!response.read(strFullKeyName))
			OMEGA_THROW(response.last_error());

		if (!strFullKeyName.empty())
			strFullKey += "/";
		strFullKey += strFullKeyName.c_str();

		if (err == OOServer::Linked)
		{
			OOBase::LocalString strLink,strLinkSubKey;
			if (!response.read(strLink) || !response.read(strLinkSubKey))
				OMEGA_THROW(response.last_error());

			OMEGA_THROW("No registry mount-point support!");
		}
		else if (err == OOServer::Ok)
		{
			if (!response.read(key) || !response.read(type))
				OMEGA_THROW(response.last_error());
		}
	}

	return err;
}

bool_t RootKey::IsKey(const string_t& strSubKey)
{
	int64_t key;
	byte_t type;
	string_t strFullKey;

	OOServer::RootErrCode_t err = open_key(strSubKey,IKey::OpenExisting,key,type,strFullKey);
	if (err == OOServer::NotFound)
		return false;

	ThrowCorrectException(err,strFullKey);
	return true;
}

bool_t RootKey::IsValue(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_ValueExists));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());
	
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == OOServer::NotFound)
		return false;

	ThrowCorrectException(err,m_strKey);
	return true;
}

any_t RootKey::GetValue(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_GetValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == OOServer::NotFound)
		ThrowValueNotFound(m_strKey,strName);
	else
		ThrowCorrectException(err,m_strKey);

	OOBase::LocalString strValue;
	if (!response.read(strValue))
		OMEGA_THROW(response.last_error());

	return strValue.c_str();
}

void RootKey::SetValue(const string_t& strName, const any_t& value)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_SetValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());
	request.write(value.cast<string_t>().c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == OOServer::NotFound)
		ThrowValueNotFound(m_strKey,strName);
	else if (err == OOServer::BadName)
		throw IAccessDeniedException::Create(string_t::constant("Invalid value name '{0}'") % strName);
	else
		ThrowCorrectException(err,m_strKey);
}

IKey* RootKey::OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	int64_t key;
	byte_t type;
	string_t strFullKey;

	OOServer::RootErrCode_t err = open_key(strSubKey,flags,key,type,strFullKey);
	ThrowCorrectException(err,strFullKey);

	if (key == m_key && type == m_type)
	{
		Internal_AddRef();
		return this;
	}

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<RootKey> > ptrNew = ObjectImpl<RootKey>::CreateInstance();
	ptrNew->init(strFullKey,key,type);
	return ptrNew.Detach();
}

IKey::string_set_t RootKey::EnumSubKeys()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_EnumSubKeys));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err,m_strKey);

	IKey::string_set_t sub_keys;
	for (;;)
	{
		OOBase::LocalString strName;
		if (!response.read(strName))
			OMEGA_THROW(response.last_error());

		if (strName.empty())
			break;

		sub_keys.insert(strName.c_str());
	}
	return sub_keys;
}

IKey::string_set_t RootKey::EnumValues()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_EnumValues));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	ThrowCorrectException(err,m_strKey);

	IKey::string_set_t values;
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

void RootKey::DeleteSubKey(const string_t& strSubKey)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_DeleteSubKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	string_t strFullKey = m_strKey;
	if (err != OOServer::Errored)
	{
		OOBase::LocalString strFullKeyName;
		if (!response.read(strFullKeyName))
			OMEGA_THROW(response.last_error());

		if (!strFullKeyName.empty())
			strFullKey += "/";
		strFullKey += strFullKeyName.c_str();

		if (err == OOServer::Linked)
		{
			OOBase::LocalString strLink,strLinkSubKey;
			if (!response.read(strLink) || !response.read(strLinkSubKey))
				OMEGA_THROW(response.last_error());

			OMEGA_THROW("No registry mount-point support!");
		}
	}

	ThrowCorrectException(err,strFullKey);
}

void RootKey::DeleteValue(const string_t& strName)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Registry_DeleteValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.c_str());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::CDRStream response;
	Manager::instance()->sendrecv_root(request,&response,TypeInfo::Synchronous);
	
	OOServer::RootErrCode_t err;
	if (!response.read(err))
		OMEGA_THROW(response.last_error());

	if (err == OOServer::NotFound)
		ThrowValueNotFound(m_strKey,strName);
	else
		ThrowCorrectException(err,m_strKey);
}

void OverlayKey::init(IKey* pOver, IKey* pUnder)
{
	m_ptrOver = pOver;
	m_ptrOver.AddRef();

	m_ptrUnder = pUnder;
	m_ptrUnder.AddRef();
}

void OverlayKey::ThrowOverlay()
{
	throw IAccessDeniedException::Create(string_t::constant("The registry key '{0}' is an overlay") % GetName());
}

string_t OverlayKey::GetName()
{
	return m_ptrOver->GetName();
}

bool_t OverlayKey::IsKey(const string_t& strSubKey)
{
	return (m_ptrOver->IsKey(strSubKey) || m_ptrUnder->IsKey(strSubKey));
}

bool_t OverlayKey::IsValue(const string_t& strName)
{
	return (m_ptrOver->IsValue(strName) || m_ptrUnder->IsValue(strName));
}

any_t OverlayKey::GetValue(const string_t& strName)
{
	if (!m_ptrUnder->IsValue(strName))
		return m_ptrOver->GetValue(strName);

	if (!m_ptrOver->IsValue(strName))
		return m_ptrUnder->IsValue(strName);

	return m_ptrOver->GetValue(strName);
}

void OverlayKey::SetValue(const string_t& /*strName*/, const any_t& /*value*/)
{
	ThrowOverlay();
}

IKey* OverlayKey::OpenKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	if (flags != IKey::OpenExisting)
		ThrowOverlay();

	if (!m_ptrUnder->IsKey(strSubKey))
		return m_ptrOver->OpenKey(strSubKey,IKey::OpenExisting);

	if (!m_ptrOver->IsKey(strSubKey))
		return m_ptrUnder->OpenKey(strSubKey,IKey::OpenExisting);

	ObjectPtr<IKey> ptrSubOver = m_ptrOver->OpenKey(strSubKey,IKey::OpenExisting);
	ObjectPtr<IKey> ptrSubUnder = m_ptrUnder->OpenKey(strSubKey,IKey::OpenExisting);

	ObjectPtr<ObjectImpl<OverlayKey> > ptrKey = ObjectImpl<OverlayKey>::CreateInstance();
	ptrKey->init(ptrSubOver,ptrSubUnder);
	return ptrKey.Detach();
}

IKey::string_set_t OverlayKey::EnumSubKeys()
{
	IKey::string_set_t over_set = m_ptrOver->EnumSubKeys();
	IKey::string_set_t under_set = m_ptrUnder->EnumSubKeys();

	over_set.insert(under_set.begin(),under_set.end());
	return over_set;
}

IKey::string_set_t OverlayKey::EnumValues()
{
	IKey::string_set_t over_set = m_ptrOver->EnumValues();
	IKey::string_set_t under_set = m_ptrUnder->EnumValues();

	over_set.insert(under_set.begin(),under_set.end());
	return over_set;
}

void OverlayKey::DeleteSubKey(const string_t& /*strSubKey*/)
{
	ThrowOverlay();
}

void OverlayKey::DeleteValue(const string_t& /*strName*/)
{
	ThrowOverlay();
}

IKey* OverlayKeyFactory::Overlay(const string_t& strOver, const string_t& strUnder)
{
	ObjectPtr<IKey> ptrRoot("/");

	if (!ptrRoot->IsKey(strUnder))
		return ptrRoot->OpenKey(strOver,IKey::OpenExisting);

	if (!ptrRoot->IsKey(strOver))
		return ptrRoot->OpenKey(strUnder,IKey::OpenExisting);

	ObjectPtr<IKey> ptrSubOver = ptrRoot->OpenKey(strOver,IKey::OpenExisting);
	ObjectPtr<IKey> ptrSubUnder = ptrRoot->OpenKey(strUnder,IKey::OpenExisting);

	ObjectPtr<ObjectImpl<OverlayKey> > ptrKey = ObjectImpl<OverlayKey>::CreateInstance();
	ptrKey->init(ptrSubOver,ptrSubUnder);
	return ptrKey.Detach();
}
