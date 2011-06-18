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
	bool write_utf8(OOBase::CDRStream& stream, const string_t& str)
	{
		size_t len = 0;
		const char* sz = str.c_ustr(&len);
		return stream.write(sz,len);
	}
}

void Key::Init(Manager* pManager, const Omega::string_t& strKey, const Omega::int64_t& key, Omega::byte_t type)
{
	m_pManager = pManager;
	m_strKey = strKey;
	m_key = key;
	m_type = type;
}

string_t Key::GetName()
{
	return m_strKey;
}

bool_t Key::IsSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey);

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey;
		ptrKey.Attach(ParseSubKey(strSub));
		if (ptrKey)
		{
			if (strSub.IsEmpty())
				return true;
			else
				return ptrKey->IsSubKey(strSub);
		}
	}

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::KeyExists));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strSubKey);
	
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t Key::IsValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::ValueExists));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strName);
	
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

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

any_t Key::GetValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetValue));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strName);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	OOBase::LocalString strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

void Key::SetValue(const string_t& strName, const any_t& value)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SetValue));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strName);
	write_utf8(request,value.cast<string_t>());

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t Key::GetDescription()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetDescription));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err==ENOENT)
		NotFoundException::Throw(GetName());
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	OOBase::LocalString strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

string_t Key::GetValueDescription(const Omega::string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::GetValueDescription));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strName);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	OOBase::LocalString strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

void Key::SetDescription(const Omega::string_t& strDesc)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SetDescription));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strDesc);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(GetName());
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::SetValueDescription));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strValue);
	write_utf8(request,strDesc);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strValue);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* Key::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	BadNameException::ValidateSubKey(strSubKey);

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey;
		ptrKey.Attach(ParseSubKey(strSub));
		if (ptrKey)
		{
			if (strSub.IsEmpty())
				return ptrKey.AddRef();
			else
				return ptrKey->OpenSubKey(strSub,flags);
		}
	}

	ObjectPtr<ObjectImpl<Key> > ptrRet = OpenSubKey_i(strSubKey,flags);
	return ptrRet.AddRef();
}

IKey* Key::ParseSubKey(string_t& strSubKey)
{
	// See if we need a mirror key
	if (m_key == 0 && m_type == 0 && (strSubKey == L"Local User" || strSubKey.Left(11) == L"Local User/"))
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

		OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
		if (!response)
			OMEGA_THROW("No response from root");

		Omega::int32_t err = 0;
		if (!response->read(err))
			OMEGA_THROW(response->last_error());

		if (err != 0)
			OMEGA_THROW(err);

		Omega::byte_t local_type = 255;
		Omega::int64_t mirror_key = 0;
		OOBase::LocalString strName;

		if (!response->read(local_type) ||
				!response->read(mirror_key) ||
				!response->read(strName))
		{
			OMEGA_THROW(response->last_error());
		}

		ObjectPtr<ObjectImpl<Key> > ptrLocal = ObjectImpl<Key>::CreateInstancePtr();
		ptrLocal->Init(m_pManager,L"Local User",0,local_type);

		ObjectPtr<ObjectImpl<Key> > ptrMirror = ObjectImpl<Key>::CreateInstancePtr();
		ptrMirror->Init(m_pManager,string_t(strName.c_str(),true),mirror_key,0);

		ObjectPtr<ObjectImpl<MirrorKey> > ptrNew = ObjectImpl<MirrorKey>::CreateInstancePtr();
		ptrNew->Init(L"Local User",ptrLocal,ptrMirror);
		return ptrNew.AddRef();
	}

	return 0;
}

ObjectPtr<ObjectImpl<Key> > Key::OpenSubKey_i(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::CreateKey));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strSubKey);

	request.write(flags);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += L"/";
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

	Omega::int64_t key = 0;
	Omega::byte_t type = 255;
	if (!response->read(key) || !response->read(type))
		OMEGA_THROW(response->last_error());

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<Key> > ptrNew = ObjectImpl<Key>::CreateInstancePtr();
	ptrNew->Init(m_pManager,strFullKey,key,type);
	return ptrNew;
}

std::set<Omega::string_t> Key::EnumSubKeys()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::EnumSubKeys));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

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
		std::set<Omega::string_t> sub_keys;
		for (;;)
		{
			OOBase::LocalString strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			sub_keys.insert(string_t(strName.c_str(),true));

			if (m_key == 0 && m_type == 0)
			{
				// Add the local user key, although it doesn't really exist...
				sub_keys.insert(L"Local User");
			}
		}

		return sub_keys;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

std::set<Omega::string_t> Key::EnumValues()
{
	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::EnumValues));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

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
		std::set<Omega::string_t> values;
		for (;;)
		{
			OOBase::LocalString strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			values.insert(string_t(strName.c_str(),true));
		}

		return values;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

void Key::DeleteKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey);

	if (m_key == 0 && m_type == 0)
	{
		string_t strSub = strSubKey;
		ObjectPtr<IKey> ptrKey;
		ptrKey.Attach(ParseSubKey(strSub));
		if (ptrKey)
		{
			if (strSub.IsEmpty())
			{
				string_t strFullKey = GetName();
				if (!strFullKey.IsEmpty())
					strFullKey += L"/";
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
	write_utf8(request,strSubKey);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += L"/";
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

void Key::DeleteValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName);

	OOBase::CDRStream request;
	request.write(static_cast<OOServer::RootOpCode_t>(OOServer::DeleteValue));
	request.write(m_key);
	request.write(m_type);
	write_utf8(request,strName);

	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW("No response from root");

	Omega::int32_t err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

#include "MirrorKey.inl"
