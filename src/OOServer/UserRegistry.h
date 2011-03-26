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

namespace User
{
	class Manager;

	namespace Registry
	{
		class Key :
				public OTL::ObjectBase,
				public Omega::Registry::IKey
		{
		public:
			void Init(Manager* pManager, const Omega::string_t& strKey, const Omega::int64_t& key, Omega::byte_t type);

			BEGIN_INTERFACE_MAP(Key)
				INTERFACE_ENTRY(Omega::Registry::IKey)
			END_INTERFACE_MAP()

		private:
			Manager*        m_pManager;
			Omega::string_t m_strKey;
			Omega::int64_t  m_key;
			Omega::byte_t   m_type;

			Omega::Registry::IKey* ParseSubKey(Omega::string_t& strSubKey);
			OTL::ObjectPtr<OTL::ObjectImpl<Key> > OpenSubKey_i(const Omega::string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags);

		// IKey members
		public:
			Omega::string_t GetName();
			Omega::bool_t IsSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::any_t GetValue(const Omega::string_t& strName);
			void SetValue(const Omega::string_t& strName, const Omega::any_t& value);
			Omega::string_t GetDescription();
			Omega::string_t GetValueDescription(const Omega::string_t& strName);
			void SetDescription(const Omega::string_t& strValue);
			void SetValueDescription(const Omega::string_t& strName, const Omega::string_t& strValue);
			Omega::Registry::IKey* OpenSubKey(const Omega::string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags = OpenExisting);
			std::set<Omega::string_t> EnumSubKeys();
			std::set<Omega::string_t> EnumValues();
			void DeleteKey(const Omega::string_t& strSubKey);
			void DeleteValue(const Omega::string_t& strName);
		};
	}
}

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_
