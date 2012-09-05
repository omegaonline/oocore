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
		virtual ~TypeInfoImpl();

		void init(const guid_t& iid, const string_t& strName, const System::Internal::typeinfo_rtti* type_info);

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
			string_t                       strName;
			TypeInfo::MethodAttributes_t   attribs;
			ObjectPtr<Remoting::IMessage>  return_type;
			OOBase::Stack<ParamInfo>*      params;
		};

		string_t                              m_strName;
		guid_t                                m_iid;
		OOBase::Stack<MethodInfo>             m_methods;
		uint32_t                              m_base_methods;
		ObjectPtr<TypeInfo::IInterfaceInfo>   m_ptrBase;

	// IInterfaceInfo members
	public:
		string_t GetName();
		guid_t GetIID();
		uint32_t GetMethodCount();
		IInterfaceInfo* GetBaseType();
		void GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, byte_t& param_count, Remoting::IMessage*& return_type);
		void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, TypeInfo::ParamAttributes_t& attribs);
		byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib);
	};

	class TIMapImpl
	{
	public:
		void insert(const void* key, const guid_t& iid, const char* pszName, const System::Internal::typeinfo_rtti* type_info);
		void remove(const void* key, const guid_t& iid);

		TypeInfo::IInterfaceInfo* get_type_info(const guid_t& iid);

	private:
		OOBase::RWMutex m_lock;

		struct ti_t
		{
			const char*                            pszName;
			const void*                            key;
			const System::Internal::typeinfo_rtti* type_info;
		};

		OOBase::Table<guid_t,ti_t> m_ti_map;
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
		td->WriteValue(string_t::constant("type"),th->type);

		if (th->type == TypeInfo::typeObject)
		{
			td->WriteValue(string_t::constant("iid"),guid_t(*(const guid_base_t*)(th->next)));

			// Add terminating void if not already written...
			td->WriteValue(string_t::constant("type"),TypeInfo::Type_t(TypeInfo::typeVoid));
		}
		else if (th->next)
		{
			BuildTypeDetail(td,th->next);
		}
		else if (th->type != TypeInfo::typeVoid)
		{
			// Add terminating void if not already written...
			td->WriteValue(string_t::constant("type"),TypeInfo::Type_t(TypeInfo::typeVoid));
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
			return string_t::constant("void");

		case TypeInfo::typeBool:
			return string_t::constant("Omega::bool_t");

		case TypeInfo::typeByte:
			return string_t::constant("Omega::byte_t");

		case TypeInfo::typeInt16:
			return string_t::constant("Omega::int16_t");

		case TypeInfo::typeUInt16:
			return string_t::constant("Omega::uint16_t");

		case TypeInfo::typeInt32:
			return string_t::constant("Omega::int32_t");

		case TypeInfo::typeUInt32:
			return string_t::constant("Omega::uint32_t");

		case TypeInfo::typeInt64:
			return string_t::constant("Omega::int64_t");

		case TypeInfo::typeUInt64:
			return string_t::constant("Omega::uint64_t");

		case TypeInfo::typeFloat4:
			return string_t::constant("Omega::float4_t");

		case TypeInfo::typeFloat8:
			return string_t::constant("Omega::float8_t");

		case TypeInfo::typeString:
			return string_t::constant("Omega::string_t");

		case TypeInfo::typeGuid:
			return string_t::constant("Omega::guid_t");

		case TypeInfo::typeAny:
			return string_t::constant("Omega::any_t");

		case TypeInfo::typeObject:
			{
				string_t strIID = OOCore::get_text("Unknown interface {0}") % guid_t(*(const guid_base_t*)(th->next)).ToString();
				ObjectPtr<TypeInfo::IInterfaceInfo> ptrIF = OOCore::GetInterfaceInfo(*(const guid_base_t*)(th->next));
				if (ptrIF)
					strIID = ptrIF->GetName();
				return strIID;
			}

		case TypeInfo::typeSTLVector:
			return "std::vector<" + strNext + (strNext.Right(1)==">" ? " >" : ">");

		case TypeInfo::typeSTLDeque:
			return "std::deque<" + strNext + (strNext.Right(1)==">" ? " >" : ">");

		case TypeInfo::typeSTLList:
			return "std::list<" + strNext + (strNext.Right(1)==">" ? " >" : ">");

		case TypeInfo::typeSTLSet:
			return "std::set<" + strNext + (strNext.Right(1)==">" ? " >" : ">");

		case TypeInfo::typeSTLMultiset:
			return "std::multiset<" + strNext + (strNext.Right(1)==">" ? " >" : ">");

		case TypeInfo::typeSTLMap:
			{
				string_t strNext2 = BuildTypeString(th[1].next);
				return "std::map<" + strNext + ',' + strNext2 + (strNext2.Right(1)==">" ? " >" : ">");
			}

		case TypeInfo::typeSTLMultimap:
			{
				string_t strNext2 = BuildTypeString(th[1].next);
				return "std::multimap<" + strNext + ',' + strNext2 + (strNext2.Right(1)==">" ? " >" : ">");
			}

		case TypeInfo::modifierConst:
			return strNext + string_t::constant(" const");

		case TypeInfo::modifierPointer:
			return strNext + '*';

		case TypeInfo::modifierReference:
			return strNext + '&';

		default:
			return OOCore::get_text("Invalid type code: {0}") % th->type;
		}
	}
}

template class OOBase::Singleton<TIMapImpl,OOCore::DLL>;

TypeInfoImpl::TypeInfoImpl() :
		m_base_methods(0)
{
}

TypeInfoImpl::~TypeInfoImpl()
{
	for (MethodInfo mi;m_methods.pop(&mi);)
		delete mi.params;
}

void TypeInfoImpl::init(const guid_t& iid, const string_t& strName, const System::Internal::typeinfo_rtti* type_info)
{
	m_iid = iid;
	m_strName = strName;

	// Init the base class
	if (type_info->base_type)
	{
		m_ptrBase = TIMap::instance().get_type_info(*type_info->base_type);
		m_base_methods = m_ptrBase->GetMethodCount();
	}

	// Copy all the members into our own members... This is so the underlying pointers can be unloaded with their Dll
	for (const System::Internal::typeinfo_rtti::MethodInfo* pmi=(*type_info->pfnGetMethodInfo)(); pmi->pszName!=0; ++pmi)
	{
		MethodInfo mi;
		mi.strName = pmi->pszName;
		mi.attribs = pmi->attribs;
		mi.return_type = Remoting::CreateMemoryMessage();
		BuildTypeDetail(mi.return_type,pmi->return_type);

		mi.params = new (OOCore::throwing) OOBase::Stack<ParamInfo>();
		
		for (const System::Internal::typeinfo_rtti::ParamInfo* ppi=(*pmi->pfnGetParamInfo)(); ppi->pszName!=0; ++ppi)
		{
			ParamInfo pi;
			pi.strName = ppi->pszName;
			pi.attribs = ppi->attribs;
			pi.strRef = ppi->attrib_ref;
			pi.type = Remoting::CreateMemoryMessage();

			BuildTypeDetail(pi.type,ppi->type);

			int err = mi.params->push(pi);
			if (err != 0)
				OMEGA_THROW(err);
		}

		int err = m_methods.push(mi);
		if (err != 0)
			OMEGA_THROW(err);
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

void TypeInfoImpl::GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, byte_t& param_count, Remoting::IMessage*& return_type)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetMethodInfo(method_idx,strName,attribs,param_count,return_type);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetMethodInfo requesting invalid method index");

	MethodInfo* mi = m_methods.at(method_idx - m_base_methods);
	if (!mi)
		OMEGA_THROW("GetMethodInfo requesting invalid method index");

	strName = mi->strName;
	attribs = mi->attribs;
	param_count = mi->params ? static_cast<byte_t>(mi->params->size()) : 0;
	return_type = mi->return_type.AddRef();
}

void TypeInfoImpl::GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, Remoting::IMessage*& type, TypeInfo::ParamAttributes_t& attribs)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetParamInfo(method_idx,param_idx,strName,type,attribs);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetParamInfo requesting invalid method index");

	MethodInfo* mi = m_methods.at(method_idx - m_base_methods);
	if (!mi)
		OMEGA_THROW("GetParamInfo requesting invalid method index");

	if (!mi->params || param_idx >= mi->params->size())
		OMEGA_THROW("GetParamInfo requesting invalid param index");

	ParamInfo* pi = mi->params->at(param_idx);
	if (!pi)
		OMEGA_THROW("GetParamInfo requesting invalid param index");

	strName = pi->strName;
	type = pi->type;
	attribs = pi->attribs;
	type = pi->type.AddRef();
}

byte_t TypeInfoImpl::GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib)
{
	if (method_idx < m_base_methods)
		return m_ptrBase->GetAttributeRef(method_idx,param_idx,attrib);

	if (method_idx >= GetMethodCount())
		OMEGA_THROW("GetAttributeRef requesting invalid method index");

	const MethodInfo* mi = m_methods.at(method_idx - m_base_methods);
	if (!mi)
		OMEGA_THROW("GetAttributeRef requesting invalid method index");

	if (!mi->params || param_idx >= mi->params->size())
		OMEGA_THROW("GetAttributeRef requesting invalid param index");

	const ParamInfo* pi = mi->params->at(param_idx);
	if (!pi)
		OMEGA_THROW("GetAttributeRef requesting non-ref param reference");

	if (!(pi->attribs & attrib))
		OMEGA_THROW("GetAttributeRef requesting non-ref param reference");

	for (size_t i=0; i<mi->params->size(); ++i)
	{
		if (mi->params->at(i)->strName == pi->strRef)
			return static_cast<byte_t>(i);
	}

	OMEGA_THROW("GetAttributeRef failed to find reference parameter");
}

void TIMapImpl::insert(const void* key, const guid_t& iid, const char* pszName, const System::Internal::typeinfo_rtti* type_info)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ti_t ti = { pszName, key, type_info };

	int err = m_ti_map.insert(iid,ti);
	if (err != 0)
		OMEGA_THROW(err);
}

void TIMapImpl::remove(const void* key, const guid_t& iid)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_ti_map.find_first(iid); i < m_ti_map.size() && *m_ti_map.key_at(i)==iid;)
	{
		if (m_ti_map.at(i)->key == key)
			m_ti_map.remove_at(i);
		else
			++i;
	}
}

TypeInfo::IInterfaceInfo* TIMapImpl::get_type_info(const guid_t& iid)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	ti_t ti;
	if (m_ti_map.find(iid,ti))
	{
		ObjectPtr<ObjectImpl<TypeInfoImpl> > ptrTI = ObjectImpl<TypeInfoImpl>::CreateObject();
		ptrTI->init(iid,ti.pszName,ti.type_info);
		return ptrTI.Detach();
	}

	return NULL;
}

TypeInfo::IInterfaceInfo* OOCore::GetInterfaceInfo(const guid_t& iid)
{
	return TIMap::instance().get_type_info(iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Internal_RegisterAutoTypeInfo,4,((in),const void*,key,(in),const guid_t&,iid,(in),const char*,pszName,(in),const void*,type_info))
{
	TIMap::instance().insert(key,iid,pszName,static_cast<const System::Internal::typeinfo_rtti*>(type_info));
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Internal_UnregisterAutoTypeInfo,2,((in),const void*,key,(in),const guid_t&,iid))
{
	TIMap::instance().remove(key,iid);
}

void CastException::Throw(const any_t& value, any_t::CastResult_t reason, const System::Internal::type_holder* typeDest)
{
	string_t strSource;
	switch (value.GetType())
	{
	case TypeInfo::typeBool:
		strSource = string_t::constant("Omega::bool_t");
		break;

	case TypeInfo::typeByte:
		strSource = string_t::constant("Omega::byte_t");
		break;

	case TypeInfo::typeInt16:
		strSource = string_t::constant("Omega::int16_t");
		break;

	case TypeInfo::typeUInt16:
		strSource = string_t::constant("Omega::uint16_t");
		break;

	case TypeInfo::typeInt32:
		strSource = string_t::constant("Omega::int32_t");
		break;

	case TypeInfo::typeUInt32:
		strSource = string_t::constant("Omega::uint32_t");
		break;

	case TypeInfo::typeInt64:
		strSource = string_t::constant("Omega::int64_t");
		break;

	case TypeInfo::typeUInt64:
		strSource = string_t::constant("Omega::uint64_t");
		break;

	case TypeInfo::typeFloat4:
		strSource = string_t::constant("Omega::float4_t");
		break;

	case TypeInfo::typeFloat8:
		strSource = string_t::constant("Omega::float8_t");
		break;

	case TypeInfo::typeString:
		strSource = string_t::constant("Omega::string_t");
		break;

	case TypeInfo::typeGuid:
		strSource = string_t::constant("Omega::guid_t");
		break;

	case TypeInfo::typeVoid:
	default:
		strSource = string_t::constant("void");
		break;
	}

	string_t strDest = BuildTypeString(typeDest);

	string_t strReason;
	switch (reason)
	{
	case any_t::castOverflow:
		strReason = OOCore::get_text("Source value would overflow");
		break;

	case any_t::castPrecisionLoss:
		strReason = OOCore::get_text("Source value would lose precision");
		break;

	case any_t::castValid:
	case any_t::castUnrelated:
	default:
		strReason = OOCore::get_text("Data types are unrelated");
		break;
	}

	ObjectPtr<ObjectImpl<CastException> > pNew = ObjectImpl<CastException>::CreateObject();
	pNew->m_strDesc = OOCore::get_text("Failed to convert from {0} to {1}: {2}") % strSource % strDest % strReason;
	pNew->m_value = value;
	pNew->m_reason = reason;
	pNew->m_type = Remoting::CreateMemoryMessage();
	BuildTypeDetail(pNew->m_type,typeDest);

	throw static_cast<ICastException*>(pNew.Detach());
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_ICastException_Throw,3,((in),const any_t&,value,(in),any_t::CastResult_t,reason,(in),const System::Internal::type_holder*,typeDest))
{
	CastException::Throw(value,reason,typeDest);
}
