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
		bool_t IsValue(const string_t& strName);
		any_t GetValue(const string_t& strName);
		void SetValue(const string_t& strName, const any_t& value);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		std::set<string_t> EnumSubKeys();
		std::set<string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};
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

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey = ParseSubKey(strSub);
		if (ptrKey)
		{
			if (strSub.IsEmpty())
				return true;
			else
				return ptrKey->IsSubKey(strSub);
		}
	}

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
		NotFoundException::Throw(strName);
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
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* RootKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	BadNameException::ValidateSubKey(strSubKey);

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey = ParseSubKey(strSub);
		if (ptrKey)
		{
			if (strSub.IsEmpty())
				return ptrKey.Detach();
			else
				return ptrKey->OpenSubKey(strSub,flags);
		}
	}

	return OpenSubKey_i(strSubKey,flags);
}

IKey* RootKey::ParseSubKey(string_t& strSubKey)
{
	// See if we need a mirror key
	if (m_key == 0 && m_type == 0 && (strSubKey == "Local User" || strSubKey.Left(11) == "Local User/"))
	{
		// Local user, strip the start...
		if (strSubKey.Length() > 10)
			strSubKey = strSubKey.Mid(11);
		else
			strSubKey.Clear();

		OOBase::CDRStream request;
		request.write(static_cast<OOServer::RootOpCode_t>(OOServer::OpenMirrorKey));
		if (request.last_error() != 0)
			OMEGA_THROW(request.last_error());

		OOBase::CDRStream response;
		m_pManager->sendrecv_root(request,&response,TypeInfo::Synchronous);
		
		int32_t err = 0;
		if (!response.read(err))
			OMEGA_THROW(response.last_error());

		if (err != 0)
			OMEGA_THROW(err);

		byte_t local_type = 255;
		int64_t mirror_key = 0;
		OOBase::LocalString strName;

		if (!response.read(local_type) ||
				!response.read(mirror_key) ||
				!response.read(strName))
		{
			OMEGA_THROW(response.last_error());
		}

		ObjectPtr<ObjectImpl<RootKey> > ptrLocal = ObjectImpl<RootKey>::CreateInstance();
		ptrLocal->init(m_pManager,string_t::constant("Local User"),0,local_type);

		ObjectPtr<ObjectImpl<RootKey> > ptrMirror = ObjectImpl<RootKey>::CreateInstance();
		ptrMirror->init(m_pManager,strName.c_str(),mirror_key,0);

		ObjectPtr<ObjectImpl<MirrorKey> > ptrNew = ObjectImpl<MirrorKey>::CreateInstance();
		ptrNew->init(string_t::constant("Local User"),ptrLocal,ptrMirror);
		return ptrNew.Detach();
	}

	return NULL;
}

IKey* RootKey::OpenSubKey_i(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
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
		AlreadyExistsException::Throw(strFullKey);
	else if (err==ENOENT)
		NotFoundException::Throw(strFullKey);
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
		NotFoundException::Throw(GetName());
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
		NotFoundException::Throw(GetName());
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

void RootKey::DeleteKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey);

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey = ParseSubKey(strSub);
		if (ptrKey)
		{
			if (strSub.IsEmpty())
			{
				string_t strFullKey = GetName();
				if (!strFullKey.IsEmpty())
					strFullKey += "/";
				strFullKey += strSubKey;
				
				AccessDeniedException::Throw(strFullKey);
			}

			return ptrKey->DeleteKey(strSub);
		}
	}

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::DeleteKey));
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
			NotFoundException::Throw(strFullKey);
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
		NotFoundException::Throw(strName);
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
	try
	{
		return m_ptrOver->GetValue(strName);
	}
	catch (IException* pE)
	{
		pE->Release();
	}
	return m_ptrUnder->GetValue(strName);
}

void OverlayKey::SetValue(const string_t& strName, const any_t& value)
{
	AccessDeniedException::Throw("overlay:" + GetName());
}

IKey* OverlayKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	if (flags == IKey::CreateNew)
		AccessDeniedException::Throw("overlay:" + GetName());

	ObjectPtr<IKey> ptrSubOver = m_ptrOver->OpenSubKey(strSubKey,IKey::OpenExisting);
	ObjectPtr<IKey> ptrSubUnder = m_ptrUnder->OpenSubKey(strSubKey,IKey::OpenExisting);

	if (!ptrSubOver && !ptrSubUnder)
		return NULL;
	else if (!ptrSubOver)
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

void OverlayKey::DeleteKey(const string_t& strSubKey)
{
	AccessDeniedException::Throw("overlay:" + GetName());
}

void OverlayKey::DeleteValue(const string_t& strName)
{
	AccessDeniedException::Throw("overlay:" + GetName());
}

IKey* OverlayKeyFactory::Overlay(IKey* pOver, IKey* pUnder)
{
	// Check we have 2 keys to overlay
	if (!pOver && !pUnder)
		return NULL;
	else if (!pOver)
	{
		pUnder->AddRef();
		return pUnder;
	}
	else if (!pUnder)
	{
		pOver->AddRef();
		return pOver;
	}

	ObjectPtr<ObjectImpl<OverlayKey> > ptrKey = ObjectImpl<OverlayKey>::CreateInstance();
	ptrKey->init(pOver,pUnder);
	return ptrKey.Detach();
}

#include "MirrorKey.inl"
