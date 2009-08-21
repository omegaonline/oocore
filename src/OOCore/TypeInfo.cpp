///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	Omega::TypeInfo::ITypeInfo* GetTypeInfo(const Omega::guid_t& iid);
}

namespace
{
	class TypeInfoImpl : 
		public ObjectBase,
		public TypeInfo::ITypeInfo
	{
	public:
		TypeInfoImpl();

		void init(const guid_t& iid, const wchar_t* pszName, const System::MetaInfo::typeinfo_rtti* type_info);

		BEGIN_INTERFACE_MAP(TypeInfoImpl)
			INTERFACE_ENTRY(TypeInfo::ITypeInfo)
		END_INTERFACE_MAP()

	private:
		struct ParamInfo
		{
			std::wstring                strName;
			TypeInfo::Types_t           type;
			TypeInfo::ParamAttributes_t attribs;
			std::wstring                strRef;
			guid_t                      iid;
		};

		struct MethodInfo
		{
			std::wstring                 strName;
			TypeInfo::MethodAttributes_t attribs;
			uint32_t                     timeout;
			TypeInfo::Types_t            return_type;
			std::vector<ParamInfo>       params;
		};

		std::wstring                   m_strName;
		guid_t                         m_iid;
		std::vector<MethodInfo>        m_methods;
		uint32_t                       m_base_methods;
		ObjectPtr<TypeInfo::ITypeInfo> m_ptrBase;

	// ITypeInfo members
	public:
		virtual string_t GetName();
		virtual guid_t GetIID();
		virtual uint32_t GetMethodCount();
		virtual ITypeInfo* GetBaseType();
		virtual void GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, TypeInfo::Types_t& return_type);
		virtual void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, TypeInfo::Types_t& type, TypeInfo::ParamAttributes_t& attribs);
		virtual byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib);
		virtual guid_t GetParamIid(uint32_t method_idx, byte_t param_idx);
	};

	class TIMapImpl
	{
	public:
		void insert(const guid_t& iid, const wchar_t* pszName, const System::MetaInfo::typeinfo_rtti* type_info);
		void remove(const guid_t& iid, const System::MetaInfo::typeinfo_rtti* type_info);
		
		TypeInfo::ITypeInfo* get_type_info(const guid_t& iid);

	private:
		OOBase::RWMutex m_lock;

		struct ti_t
		{
			const wchar_t*                         pszName;
			const System::MetaInfo::typeinfo_rtti* type_info;
		};

		std::multimap<guid_t,ti_t> m_ti_map;
	};
	typedef OOBase::Singleton<TIMapImpl,OOCore::DLL> TIMap;
}

TypeInfoImpl::TypeInfoImpl() :
	m_base_methods(0)
{
}

void TypeInfoImpl::init(const guid_t& iid, const wchar_t* pszName, const System::MetaInfo::typeinfo_rtti* type_info)
{
	m_iid = iid;
	m_strName = pszName;

	// Init the base class
	if (type_info->base_type)
	{
		m_ptrBase.Attach(TIMap::instance()->get_type_info(*type_info->base_type));
		m_base_methods = m_ptrBase->GetMethodCount();
	}

	// Copy all the members into our own members...
	try
	{
		for (const System::MetaInfo::typeinfo_rtti::MethodInfo* pmi=(*type_info->pfnGetMethodInfo)();pmi->pszName!=0;++pmi)
		{
			MethodInfo mi;
			mi.strName = pmi->pszName,
			mi.attribs = pmi->attribs,
			mi.timeout = pmi->timeout,
			mi.return_type = pmi->return_type;

			for (const System::MetaInfo::typeinfo_rtti::ParamInfo* pi=pmi->params;pi->pszName!=0;++pi)
			{
				ParamInfo p;
				p.strName = pi->pszName;
				p.attribs = pi->attribs;
				p.type = pi->type;
				p.strRef = pi->attrib_ref;
				if (pi->iid)
					p.iid = *pi->iid;
				else
					p.iid = guid_t::Null();

				mi.params.push_back(p);
			}

			m_methods.push_back(mi);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

string_t TypeInfoImpl::GetName()
{
	return m_strName.c_str();
}

guid_t TypeInfoImpl::GetIID()
{
	return m_iid;
}

uint32_t TypeInfoImpl::GetMethodCount()
{
	return static_cast<uint32_t>(m_methods.size() + m_base_methods);
}

TypeInfo::ITypeInfo* TypeInfoImpl::GetBaseType()
{
	return m_ptrBase;
}

void TypeInfoImpl::GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, TypeInfo::Types_t& return_type)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetMethodInfo(method_idx,strName,attribs,timeout,param_count,return_type);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW(L"GetMethodInfo requesting invalid method index");

	const MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	strName = mi.strName.c_str();
	attribs = mi.attribs;
	timeout = mi.timeout;
	param_count = static_cast<byte_t>(mi.params.size());
	return_type = mi.return_type;
}

void TypeInfoImpl::GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, TypeInfo::Types_t& type, TypeInfo::ParamAttributes_t& attribs)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetParamInfo(method_idx,param_idx,strName,type,attribs);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW(L"GetParamInfo requesting invalid method index");

	const MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	if (param_idx >= mi.params.size())
		OMEGA_THROW(L"GetParamInfo requesting invalid param index");

	const ParamInfo& pi = mi.params.at(param_idx);

	strName = pi.strName.c_str();
	type = pi.type;
	attribs = pi.attribs;
}

byte_t TypeInfoImpl::GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetAttributeRef(method_idx,param_idx,attrib);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW(L"GetAttributeRef requesting invalid method index");

	const MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	if (param_idx >= mi.params.size())
		OMEGA_THROW(L"GetAttributeRef requesting invalid param index");

	const ParamInfo& pi = mi.params.at(param_idx);

	if (!(pi.attribs & attrib))
		OMEGA_THROW(L"GetAttributeRef requesting non-ref param reference");

	byte_t idx = 0;
	for (std::vector<ParamInfo>::const_iterator i=mi.params.begin();i!=mi.params.end();++i,++idx)
	{
		if (i->strName == pi.strRef)
			return idx;
	}

	OMEGA_THROW(L"GetAttributeRef failed to find reference parameter");
}

guid_t TypeInfoImpl::GetParamIid(uint32_t method_idx, byte_t param_idx)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetParamIid(method_idx,param_idx);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW(L"GetParamIid requesting invalid method index");

	const MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	if (param_idx >= mi.params.size())
		OMEGA_THROW(L"GetParamIid requesting invalid param index");

	return mi.params.at(param_idx).iid;
}

void TIMapImpl::insert(const guid_t& iid, const wchar_t* pszName, const System::MetaInfo::typeinfo_rtti* type_info)
{
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		ti_t ti = { pszName, type_info };

		m_ti_map.insert(std::multimap<guid_t,ti_t>::value_type(iid,ti));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void TIMapImpl::remove(const guid_t& iid, const System::MetaInfo::typeinfo_rtti* type_info)
{
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::multimap<guid_t,ti_t>::iterator i=m_ti_map.find(iid);i!=m_ti_map.end() && i->first==iid;)
		{
			if (i->second.type_info == type_info)
				m_ti_map.erase(i++);
			else
				++i;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

TypeInfo::ITypeInfo* TIMapImpl::get_type_info(const guid_t& iid)
{
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::multimap<guid_t,ti_t>::iterator i=m_ti_map.find(iid);
		if (i != m_ti_map.end())
		{
			ObjectPtr<ObjectImpl<TypeInfoImpl> > ptrTI = ObjectImpl<TypeInfoImpl>::CreateInstancePtr();
			ptrTI->init(iid,i->second.pszName,i->second.type_info);
			return ptrTI.AddRef();
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
}

TypeInfo::ITypeInfo* OOCore::GetTypeInfo(const guid_t& iid)
{
	return TIMap::instance()->get_type_info(iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoTypeInfo,3,((in),const Omega::guid_t&,iid,(in),const wchar_t*,pszName,(in),const void*,type_info))
{
	TIMap::instance()->insert(iid,pszName,static_cast<const System::MetaInfo::typeinfo_rtti*>(type_info));
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_UnregisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),const void*,type_info))
{
	TIMap::instance()->remove(iid,static_cast<const System::MetaInfo::typeinfo_rtti*>(type_info));
}
