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
	TypeInfo::IInterfaceInfo* GetInterfaceInfo(const guid_t& iid);
}

namespace
{
	class TypeInfoImpl :
			public ObjectBase,
			public TypeInfo::IInterfaceInfo
	{
	public:
		TypeInfoImpl();

		void init(const guid_t& iid, const wchar_t* pszName, const System::Internal::typeinfo_rtti* type_info);

		BEGIN_INTERFACE_MAP(TypeInfoImpl)
			INTERFACE_ENTRY(TypeInfo::IInterfaceInfo)
		END_INTERFACE_MAP()

	private:
		struct ParamInfo
		{
			string_t                      strName;
			ObjectPtr<Remoting::IMessage> type;
			TypeInfo::ParamAttributes_t   attribs;
			string_t                      strRef;
		};

		struct MethodInfo
		{
			string_t                      strName;
			TypeInfo::MethodAttributes_t  attribs;
			uint32_t                      timeout;
			ObjectPtr<Remoting::IMessage> return_type;
			std::vector<ParamInfo>        params;
		};

		string_t                            m_strName;
		guid_t                              m_iid;
		std::vector<MethodInfo>             m_methods;
		uint32_t                            m_base_methods;
		ObjectPtr<TypeInfo::IInterfaceInfo> m_ptrBase;

	// IInterfaceInfo members
	public:
		string_t GetName();
		guid_t GetIID();
		uint32_t GetMethodCount();
		IInterfaceInfo* GetBaseType();
		void GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, Remoting::IMessage*& return_type);
		void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, TypeInfo::ParamAttributes_t& attribs);
		byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib);
	};

	class TIMapImpl
	{
	public:
		void insert(const guid_t& iid, const wchar_t* pszName, const System::Internal::typeinfo_rtti* type_info);
		void remove(const guid_t& iid, const System::Internal::typeinfo_rtti* type_info);

		TypeInfo::IInterfaceInfo* get_type_info(const guid_t& iid);

	private:
		OOBase::RWMutex m_lock;

		struct ti_t
		{
			const wchar_t*                         pszName;
			const System::Internal::typeinfo_rtti* type_info;
		};

		std::multimap<guid_t,ti_t> m_ti_map;
	};
	typedef OOBase::Singleton<TIMapImpl,OOCore::DLL> TIMap;

	class CastException :
			public ExceptionImpl<Omega::ICastException>
	{
	public:
		static void Throw(const any_t& value, any_t::CastResult_t reason, const System::Internal::type_holder* typeDest);

		BEGIN_INTERFACE_MAP(CastException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Omega::ICastException>)
		END_INTERFACE_MAP()

	private:
		any_t m_value;
		any_t::CastResult_t m_reason;
		ObjectPtr<Remoting::IMessage> m_type;

	// Omega::ICastException members
	public:
		any_t GetValue()
		{
			return m_value;
		}

		any_t::CastResult_t GetReason()
		{
			return m_reason;
		}

		Remoting::IMessage* GetDestinationType()
		{
			return m_type.AddRef();
		}
	};

	void BuildTypeDetail(ObjectPtr<Remoting::IMessage>& td, const System::Internal::type_holder* th)
	{
		td->WriteValue(L"type",th->type);

		if (th->type == TypeInfo::typeObject)
		{
			td->WriteValue(L"iid",guid_t(*(const guid_base_t*)(th->next)));

			// Add terminating void if not already written...
			td->WriteValue(L"type",TypeInfo::Type_t(TypeInfo::typeVoid));
		}
		else if (th->next)
		{
			BuildTypeDetail(td,th->next);
		}
		else
		{
			// Add terminating void if not already written...
			td->WriteValue(L"type",TypeInfo::Type_t(TypeInfo::typeVoid));
		}

		if (th->type == TypeInfo::typeSTLMap ||
				th->type == TypeInfo::typeSTLMultimap)
		{
			// Add second part immediately after first part
			BuildTypeDetail(td,th[1].next);
		}
	}

	string_t BuildTypeString(const System::Internal::type_holder* th)
	{
		string_t strNext;
		if (th->type != TypeInfo::typeObject && th->next)
			strNext = BuildTypeString(th->next);

		switch (th->type)
		{
		case TypeInfo::typeVoid:
			return string_t(L"void");

		case TypeInfo::typeBool:
			return string_t(L"Omega::bool_t");

		case TypeInfo::typeByte:
			return string_t(L"Omega::byte_t");

		case TypeInfo::typeInt16:
			return string_t(L"Omega::int16_t");

		case TypeInfo::typeUInt16:
			return string_t(L"Omega::uint16_t");

		case TypeInfo::typeInt32:
			return string_t(L"Omega::int32_t");

		case TypeInfo::typeUInt32:
			return string_t(L"Omega::uint32_t");

		case TypeInfo::typeInt64:
			return string_t(L"Omega::int64_t");

		case TypeInfo::typeUInt64:
			return string_t(L"Omega::uint64_t");

		case TypeInfo::typeFloat4:
			return string_t(L"Omega::float4_t");

		case TypeInfo::typeFloat8:
			return string_t(L"Omega::float8_t");

		case TypeInfo::typeString:
			return string_t(L"Omega::string_t");

		case TypeInfo::typeGuid:
			return string_t(L"Omega::guid_t");

		case TypeInfo::typeAny:
			return string_t(L"Omega::any_t");

		case TypeInfo::typeObject:
			{
				ObjectPtr<TypeInfo::IInterfaceInfo> ptrIF;
				ptrIF.Attach(OOCore::GetInterfaceInfo(*(const guid_base_t*)(th->next)));
				return ptrIF->GetName();
			}

		case TypeInfo::typeSTLVector:
			return L"std::vector<" + strNext + (strNext.Right(1)==L">" ? L" >" : L">");

		case TypeInfo::typeSTLDeque:
			return L"std::deque<" + strNext + (strNext.Right(1)==L">" ? L" >" : L">");

		case TypeInfo::typeSTLList:
			return L"std::list<" + strNext + (strNext.Right(1)==L">" ? L" >" : L">");

		case TypeInfo::typeSTLSet:
			return L"std::set<" + strNext + (strNext.Right(1)==L">" ? L" >" : L">");

		case TypeInfo::typeSTLMultiset:
			return L"std::multiset<" + strNext + (strNext.Right(1)==L">" ? L" >" : L">");

		case TypeInfo::typeSTLMap:
			{
				string_t strNext2 = BuildTypeString(th[1].next);
				return L"std::map<" + strNext + L',' + strNext2 + (strNext2.Right(1)==L">" ? L" >" : L">");
			}

		case TypeInfo::typeSTLMultimap:
			{
				string_t strNext2 = BuildTypeString(th[1].next);
				return L"std::multimap<" + strNext + L',' + strNext2 + (strNext2.Right(1)==L">" ? L" >" : L">");
			}

		case TypeInfo::modifierConst:
			return strNext + L" const";

		case TypeInfo::modifierPointer:
			return strNext + L'*';

		case TypeInfo::modifierReference:
			return strNext + L'&';

		default:
			return string_t(L"Invalid type code: {0}") % th->type;
		}
	}
}

TypeInfoImpl::TypeInfoImpl() :
		m_base_methods(0)
{
}

void TypeInfoImpl::init(const guid_t& iid, const wchar_t* pszName, const System::Internal::typeinfo_rtti* type_info)
{
	m_iid = iid;
	m_strName = string_t(pszName,string_t::npos);

	// Init the base class
	if (type_info->base_type)
	{
		m_ptrBase.Attach(TIMap::instance()->get_type_info(*type_info->base_type));
		m_base_methods = m_ptrBase->GetMethodCount();
	}

	// Copy all the members into our own members... This is so the underlying pointers can be unloaded with their Dll
	for (const System::Internal::typeinfo_rtti::MethodInfo* pmi=(*type_info->pfnGetMethodInfo)(); pmi->pszName!=0; ++pmi)
	{
		MethodInfo mi;
		mi.strName = string_t(pmi->pszName,false);
		mi.attribs = pmi->attribs;
		mi.timeout = pmi->timeout;
		mi.return_type.Attach(Remoting::CreateMemoryMessage());
		BuildTypeDetail(mi.return_type,pmi->return_type);

		for (const System::Internal::typeinfo_rtti::ParamInfo* ppi=(*pmi->pfnGetParamInfo)(); ppi->pszName!=0; ++ppi)
		{
			ParamInfo pi;
			pi.strName = string_t(ppi->pszName,false);
			pi.attribs = ppi->attribs;
			pi.strRef = string_t(ppi->attrib_ref,string_t::npos);
			pi.type.Attach(Remoting::CreateMemoryMessage());

			BuildTypeDetail(pi.type,ppi->type);

			mi.params.push_back(pi);
		}

		m_methods.push_back(mi);
	}
}

string_t TypeInfoImpl::GetName()
{
	return m_strName;
}

guid_t TypeInfoImpl::GetIID()
{
	return m_iid;
}

uint32_t TypeInfoImpl::GetMethodCount()
{
	return static_cast<uint32_t>(m_methods.size() + m_base_methods);
}

TypeInfo::IInterfaceInfo* TypeInfoImpl::GetBaseType()
{
	return m_ptrBase;
}

void TypeInfoImpl::GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, Remoting::IMessage*& return_type)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetMethodInfo(method_idx,strName,attribs,timeout,param_count,return_type);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetMethodInfo requesting invalid method index");

	MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	strName = string_t(mi.strName.c_str(),string_t::npos);
	attribs = mi.attribs;
	timeout = mi.timeout;
	param_count = static_cast<byte_t>(mi.params.size());
	return_type = mi.return_type.AddRef();
}

void TypeInfoImpl::GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, TypeInfo::ParamAttributes_t& attribs)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetParamInfo(method_idx,param_idx,strName,type,attribs);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetParamInfo requesting invalid method index");

	MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	if (param_idx >= mi.params.size())
		OMEGA_THROW("GetParamInfo requesting invalid param index");

	ParamInfo& pi = mi.params.at(param_idx);

	strName = string_t(pi.strName.c_str(),string_t::npos);
	type = pi.type;
	attribs = pi.attribs;
	type = pi.type.AddRef();
}

byte_t TypeInfoImpl::GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetAttributeRef(method_idx,param_idx,attrib);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetAttributeRef requesting invalid method index");

	const MethodInfo& mi = m_methods.at(method_idx - m_base_methods);

	if (param_idx >= mi.params.size())
		OMEGA_THROW("GetAttributeRef requesting invalid param index");

	const ParamInfo& pi = mi.params.at(param_idx);

	if (!(pi.attribs & attrib))
		OMEGA_THROW("GetAttributeRef requesting non-ref param reference");

	byte_t idx = 0;
	for (std::vector<ParamInfo>::const_iterator i=mi.params.begin(); i!=mi.params.end(); ++i,++idx)
	{
		if (i->strName == pi.strRef)
			return idx;
	}

	OMEGA_THROW("GetAttributeRef failed to find reference parameter");
}

void TIMapImpl::insert(const guid_t& iid, const wchar_t* pszName, const System::Internal::typeinfo_rtti* type_info)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ti_t ti = { pszName, type_info };

	m_ti_map.insert(std::multimap<guid_t,ti_t>::value_type(iid,ti));
}

void TIMapImpl::remove(const guid_t& iid, const System::Internal::typeinfo_rtti* type_info)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (std::multimap<guid_t,ti_t>::iterator i=m_ti_map.find(iid); i!=m_ti_map.end() && i->first==iid;)
	{
		if (i->second.type_info == type_info)
			m_ti_map.erase(i++);
		else
			++i;
	}
}

TypeInfo::IInterfaceInfo* TIMapImpl::get_type_info(const guid_t& iid)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	std::multimap<guid_t,ti_t>::const_iterator i=m_ti_map.find(iid);
	if (i != m_ti_map.end())
	{
		ObjectPtr<ObjectImpl<TypeInfoImpl> > ptrTI = ObjectImpl<TypeInfoImpl>::CreateInstancePtr();
		ptrTI->init(iid,i->second.pszName,i->second.type_info);
		return ptrTI.AddRef();
	}

	throw INoInterfaceException::Create(iid);
}

TypeInfo::IInterfaceInfo* OOCore::GetInterfaceInfo(const guid_t& iid)
{
	return TIMap::instance()->get_type_info(iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Internal_RegisterAutoTypeInfo,3,((in),const guid_t&,iid,(in),const wchar_t*,pszName,(in),const void*,type_info))
{
	TIMap::instance()->insert(iid,pszName,static_cast<const System::Internal::typeinfo_rtti*>(type_info));
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Internal_UnregisterAutoTypeInfo,2,((in),const guid_t&,iid,(in),const void*,type_info))
{
	TIMap::instance()->remove(iid,static_cast<const System::Internal::typeinfo_rtti*>(type_info));
}

void CastException::Throw(const any_t& value, any_t::CastResult_t reason, const System::Internal::type_holder* typeDest)
{
	string_t strSource;
	switch (value.GetType())
	{
	case TypeInfo::typeVoid:
		strSource = L"void";
		break;

	case TypeInfo::typeBool:
		strSource = L"Omega::bool_t";
		break;

	case TypeInfo::typeByte:
		strSource = L"Omega::byte_t";
		break;

	case TypeInfo::typeInt16:
		strSource = L"Omega::int16_t";
		break;

	case TypeInfo::typeUInt16:
		strSource = L"Omega::uint16_t";
		break;

	case TypeInfo::typeInt32:
		strSource = L"Omega::int32_t";
		break;

	case TypeInfo::typeUInt32:
		strSource = L"Omega::uint32_t";
		break;

	case TypeInfo::typeInt64:
		strSource = L"Omega::int64_t";
		break;

	case TypeInfo::typeUInt64:
		strSource = L"Omega::uint64_t";
		break;

	case TypeInfo::typeFloat4:
		strSource = L"Omega::float4_t";
		break;

	case TypeInfo::typeFloat8:
		strSource = L"Omega::float8_t";
		break;

	case TypeInfo::typeString:
		strSource = L"Omega::string_t";
		break;

	case TypeInfo::typeGuid:
		strSource = L"Omega::guid_t";
		break;

	default:
		OMEGA_THROW("Invalid any_t");
	}

	string_t strDest = BuildTypeString(typeDest);

	string_t strReason;
	switch (reason)
	{
	case any_t::castOverflow:
		strReason = L"Source value would overflow";
		break;

	case any_t::castPrecisionLoss:
		strReason = L"Source value would lose precision";
		break;

	case any_t::castValid:
	case any_t::castUnrelated:
	default:
		strReason = L"Data types are unrelated";
		break;
	}

	ObjectImpl<CastException>* pNew = ObjectImpl<CastException>::CreateInstance();
	pNew->m_strDesc = string_t(L"Failed to convert from {0} to {1}: {2}") % strSource % strDest % strReason;
	pNew->m_value = value;
	pNew->m_reason = reason;
	pNew->m_type.Attach(Remoting::CreateMemoryMessage());
	BuildTypeDetail(pNew->m_type,typeDest);

	throw static_cast<ICastException*>(pNew);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_ICastException_Throw,3,((in),const any_t&,value,(in),any_t::CastResult_t,reason,(in),const System::Internal::type_holder*,typeDest))
{
	CastException::Throw(value,reason,typeDest);
}
