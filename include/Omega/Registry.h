///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OMEGA_REGISTRY_H_INCLUDED_
#define OMEGA_REGISTRY_H_INCLUDED_

#include "./Omega.h"

namespace Omega
{
	namespace Registry
	{
		interface IKey : public IObject
		{
			enum OpenFlags
			{
				OpenExisting = 0,
				OpenCreate = 1,
				CreateNew = 2
			};
			typedef uint16_t OpenFlags_t;

			typedef std::set<string_t,std::less<string_t>,System::STLAllocator<string_t> > string_set_t;

			virtual string_t GetName() = 0;
			virtual bool_t IsKey(const string_t& key) = 0;
			virtual string_set_t EnumSubKeys() = 0;
			virtual IKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting) = 0;
			virtual void DeleteSubKey(const string_t& strKey) = 0;

			virtual bool_t IsValue(const string_t& name) = 0;
			virtual string_set_t EnumValues() = 0;
			virtual any_t GetValue(const string_t& name) = 0;
			virtual void SetValue(const string_t& name, const any_t& val) = 0;
			virtual void DeleteValue(const string_t& strValue) = 0;
		};

		// {EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}
		OOCORE_DECLARE_OID(OID_Registry_Instance);

		interface IOverlayKeyFactory : public IObject
		{
			virtual IKey* Overlay(const string_t& strOver, const string_t& strUnder) = 0;
		};

		// {7A351233-8363-BA15-B443-31DD1C8FC587}
		OOCORE_DECLARE_OID(OID_OverlayKeyFactory);
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::Registry, IKey, "{F33E828A-BF5E-4c26-A541-BDB2CA736DBD}",

	// Methods
	OMEGA_METHOD(string_t,GetName,0,())
	OMEGA_METHOD(bool_t,IsKey,1,((in),const string_t&,key))
	OMEGA_METHOD(Omega::Registry::IKey::string_set_t,EnumSubKeys,0,())
	OMEGA_METHOD(Registry::IKey*,OpenKey,2,((in),const string_t&,key,(in),Registry::IKey::OpenFlags_t,flags))
	OMEGA_METHOD_VOID(DeleteSubKey,1,((in),const string_t&,strKey))
	OMEGA_METHOD(bool_t,IsValue,1,((in),const string_t&,name))
	OMEGA_METHOD(Omega::Registry::IKey::string_set_t,EnumValues,0,())
	OMEGA_METHOD(any_t,GetValue,1,((in),const string_t&,name))
	OMEGA_METHOD_VOID(SetValue,2,((in),const string_t&,name,(in),const any_t&,val))
	OMEGA_METHOD_VOID(DeleteValue,1,((in),const string_t&,strValue))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Registry, IOverlayKeyFactory, "{D83FC506-5939-AB15-6018-A55090AB03DE}",

	// Methods
	OMEGA_METHOD(Registry::IKey*,Overlay,2,((in),const string_t&,strOver,(in),const string_t&,strUnder))
)

#endif // !defined(DOXYGEN)

#endif // OMEGA_REGISTRY_H_INCLUDED_
