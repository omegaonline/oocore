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

#ifndef OOSERVER_USER_REGISTRY_H_INCLUDED_
#define OOSERVER_USER_REGISTRY_H_INCLUDED_

#include "Protocol.h"

namespace User
{
	namespace Registry
	{
		class RootKey :
				public OTL::ObjectBase,
				public OTL::IProvideObjectInfoImpl<RootKey>,
				public Omega::Registry::IKey
		{
		public:
			void init(const Omega::string_t& strKey, const Omega::int64_t& key, Omega::byte_t type);

			BEGIN_INTERFACE_MAP(RootKey)
				INTERFACE_ENTRY(Omega::Registry::IKey)
				INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
			END_INTERFACE_MAP()

		private:
			Omega::string_t m_strKey;
			Omega::int64_t  m_key;
			Omega::byte_t   m_type;

			OOServer::RootErrCode_t open_key(const Omega::string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags, Omega::int64_t& key, Omega::byte_t& type, Omega::string_t& strFullKey);

		// IKey members
		public:
			Omega::string_t GetName();
			Omega::bool_t IsKey(const Omega::string_t& strSubKey);
			Omega::Registry::IKey::string_set_t EnumSubKeys();
			Omega::Registry::IKey* OpenKey(const Omega::string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags = OpenExisting);
			void DeleteSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::Registry::IKey::string_set_t EnumValues();
			Omega::any_t GetValue(const Omega::string_t& strName);
			void SetValue(const Omega::string_t& strName, const Omega::any_t& value);
			void DeleteValue(const Omega::string_t& strName);
		};

		class OverlayKeyFactory :
				public OTL::ObjectBase,
				public OTL::AutoObjectFactory<OverlayKeyFactory,&Omega::Registry::OID_OverlayKeyFactory,Omega::Activation::ProcessScope>,
				public Omega::Registry::IOverlayKeyFactory
		{
		public:
			BEGIN_INTERFACE_MAP(OverlayKeyFactory)
				INTERFACE_ENTRY(Omega::Registry::IOverlayKeyFactory)
			END_INTERFACE_MAP()

		// IOverlayKeyFactory members
		public:
			Omega::Registry::IKey* Overlay(const Omega::string_t& strOver, const Omega::string_t& strUnder);
		};
	}
}

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_
