///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOSERVER_REGISTRY_CMN_H_INCLUDED_
#define OOSERVER_REGISTRY_CMN_H_INCLUDED_

namespace Registry
{
	class BadNameException :
		public ExceptionImpl<IBadNameException>
	{
	public:
		BEGIN_INTERFACE_MAP(BadNameException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IBadNameException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetName()
		{
			return m_strName;
		}

		static void ValidateSubKey(const string_t& strSubKey, const string_t& strSource)
		{
			if (strSubKey.IsEmpty() ||
				strSubKey == L"\\" ||
				strSubKey.Right(1) == L"\\" ||
				strSubKey.Find(L"\\\\") != string_t::npos)
			{
				Throw(strSubKey,strSource);
			}
		}

		static void ValidateValue(const string_t& strName, const string_t& strSource)
		{
			if (strName.IsEmpty() || 
				strName.Find(L'\\') != string_t::npos)
			{
				Throw(strName,strSource);
			}
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<BadNameException>* pRE = ObjectImpl<BadNameException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = L"Invalid name for registry key or value: '{0}'.";
			pRE->m_strDesc %= name;
			throw static_cast<IBadNameException*>(pRE);
		}
	};

	class WrongValueTypeException :
		public ExceptionImpl<IWrongValueTypeException>
	{
	public:
		BEGIN_INTERFACE_MAP(WrongValueTypeException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IWrongValueTypeException>)
		END_INTERFACE_MAP()

		ValueType_t m_type;
		string_t    m_strValue;

	public:
		ValueType_t GetValueType()
		{
			return m_type;
		}

		string_t GetValueName()
		{
			return m_strValue;
		}

		static void Throw(string_t strValue, ValueType_t actual_type, const string_t& strSource)
		{
			ObjectImpl<WrongValueTypeException>* pRE = ObjectImpl<WrongValueTypeException>::CreateInstance();
			pRE->m_type = actual_type;
			pRE->m_strValue = strValue;
			pRE->m_strSource = strSource;
			
			pRE->m_strDesc = L"Incorrect registry value type, actual value type is {0}.";
			if (actual_type==String)
				pRE->m_strDesc %= L"String";
			else if (actual_type==Integer)
				pRE->m_strDesc %= L"Integer";
			else if (actual_type==Binary)
				pRE->m_strDesc %= L"Binary";
			else
				pRE->m_strDesc %= L"Corrupt!";

			throw static_cast<IWrongValueTypeException*>(pRE);
		}
	};

	class NotFoundException :
		public ExceptionImpl<INotFoundException>
	{
	public:
		BEGIN_INTERFACE_MAP(NotFoundException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<INotFoundException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<NotFoundException>* pRE = ObjectImpl<NotFoundException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = L"'{0}' not found.";
			pRE->m_strDesc %= name;
			throw static_cast<INotFoundException*>(pRE);
		}
	};

	class AlreadyExistsException :
		public ExceptionImpl<IAlreadyExistsException>
	{
	public:
		BEGIN_INTERFACE_MAP(AlreadyExistsException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IAlreadyExistsException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetKeyName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<AlreadyExistsException>* pRE = ObjectImpl<AlreadyExistsException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = L"Key '{0}' already exists.";
			pRE->m_strDesc %= name;
			throw static_cast<IAlreadyExistsException*>(pRE);
		}
	};

	class AccessDeniedException :
		public ExceptionImpl<IAccessDeniedException>
	{
	public:
		BEGIN_INTERFACE_MAP(AccessDeniedException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IAccessDeniedException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetKeyName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<AccessDeniedException>* pRE = ObjectImpl<AccessDeniedException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = L"Write attempt illegal for '{0}'.";
			pRE->m_strDesc %= name;
			throw static_cast<IAccessDeniedException*>(pRE);
		}
	};

	class MirrorKey : 
		public ObjectBase,
		public IKey
	{
	public:
		void Init(const string_t& strKey, IKey* pLocal, IKey* pSystem);
		
		BEGIN_INTERFACE_MAP(MirrorKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		string_t        m_strKey;
		ObjectPtr<IKey> m_ptrLocal;
		ObjectPtr<IKey> m_ptrSystem;
		
	// IRegistry members
	public:
		bool_t IsSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		string_t GetStringValue(const string_t& strName);
		int64_t GetIntegerValue(const string_t& strName);
		void GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer);
		void SetStringValue(const string_t& strName, const string_t& strValue);
		void SetIntegerValue(const string_t& strName, const int64_t& uValue);
		void SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val);
		string_t GetDescription();
		string_t GetValueDescription(const string_t& strName);
		void SetDescription(const string_t& strValue);
		void SetValueDescription(const string_t& strName, const string_t& strValue);
		ValueType_t GetValueType(const string_t& strName);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		std::set<string_t> EnumSubKeys();
		std::set<string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);	
	};
}

#endif // OOSERVER_REGISTRY_CMN_H_INCLUDED_
