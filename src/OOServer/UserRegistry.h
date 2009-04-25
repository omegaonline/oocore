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
			void Init(Manager* pManager, const Omega::string_t& strKey, const Omega::int64_t& key);
			
			BEGIN_INTERFACE_MAP(Key)
				INTERFACE_ENTRY(Omega::Registry::IKey)
			END_INTERFACE_MAP()

		private:
			Manager*        m_pManager;
			Omega::string_t m_strKey;
			Omega::int64_t  m_key;

			int GetValueType_i(const Omega::string_t& strName, Omega::Registry::ValueType_t& vtype);

		// IRegistry members
		public:
			Omega::bool_t IsSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::string_t GetStringValue(const Omega::string_t& strName);
			Omega::int64_t GetIntegerValue(const Omega::string_t& strName);
			void GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer);
			void SetStringValue(const Omega::string_t& strName, const Omega::string_t& strValue);
			void SetIntegerValue(const Omega::string_t& strName, const Omega::int64_t& uValue);
			void SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val);
			Omega::string_t GetDescription();
			Omega::string_t GetValueDescription(const Omega::string_t& strName);
			void SetDescription(const Omega::string_t& strValue);
			void SetValueDescription(const Omega::string_t& strName, const Omega::string_t& strValue);
			Omega::Registry::ValueType_t GetValueType(const Omega::string_t& strName);
			Omega::Registry::IKey* OpenSubKey(const Omega::string_t& strSubKey, Omega::Registry::IKey::OpenFlags_t flags = OpenExisting);
			Omega::IEnumString* EnumSubKeys();
			Omega::IEnumString* EnumValues();
			void DeleteKey(const Omega::string_t& strSubKey);
			void DeleteValue(const Omega::string_t& strName);	
		};
	}
}

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_
