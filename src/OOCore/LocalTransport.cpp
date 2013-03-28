///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
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

#include "LocalTransport.h"

using namespace Omega;
using namespace OTL;

namespace
{
	class LocalTransportRecord;

	class LocalTransportSet :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Storage::ISet
	{
	public:
		BEGIN_INTERFACE_MAP(LocalTransportSet)
			INTERFACE_ENTRY(Storage::ISet)
		END_INTERFACE_MAP()

	private:
		struct Tag
		{
			enum Type
			{
				eValue,
				eRecord,
				eSet
			} m_tag;
			size_t m_idx;
		};

		OOBase::SpinLock m_lock;
		OOBase::Bag<Tag,OOBase::CrtAllocator> m_bagIndex;
		OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportSet> >,OOBase::CrtAllocator> m_mapSets;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportRecord> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		uint32_t GetCount();

		any_t GetValue(uint32_t position);
		void SetValue(uint32_t position, const any_t& val);

		Storage::IRecord* OpenRecord(uint32_t position, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IRecord* DeleteRecord(uint32_t position);

		Storage::ISet* OpenSet(uint32_t position, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::ISet* DeleteSet(uint32_t position);
	};

	class LocalTransportRecord :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Storage::IRecord
	{
	public:
		BEGIN_INTERFACE_MAP(LocalTransportRecord)
			INTERFACE_ENTRY(Storage::IRecord)
		END_INTERFACE_MAP()

	private:
		struct Tag
		{
			enum Type
			{
				eValue,
				eRecord,
				eSet
			} m_tag;
			size_t m_idx;
		};

		OOBase::SpinLock m_lock;
		OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash> m_mapIndex;
		OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportSet> >,OOBase::CrtAllocator> m_mapSets;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportRecord> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		any_t GetValue(const string_t& name);
		void SetValue(const string_t& name, const any_t& val);

		Storage::IRecord* OpenRecord(const string_t& name, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IRecord* DeleteRecord(const string_t& name);

		Storage::ISet* OpenSet(const string_t& name, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::ISet* DeleteSet(const string_t& name);
	};
}

uint32_t LocalTransportSet::GetCount()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	return m_bagIndex.size();
}

any_t LocalTransportSet::GetValue(uint32_t position)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag* tag = m_bagIndex.at(position);
	if (!tag)
		return any_t();
	else if (tag->m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (tag->m_tag == Tag::eSet)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a set using GetValue"));

	any_t result;
	if (!m_mapValues.find(tag->m_idx,result))
		OMEGA_THROW("Set index out of sync");

	return result;
}

void LocalTransportSet::SetValue(uint32_t position, const any_t& val)
{

}

Storage::IRecord* LocalTransportSet::OpenRecord(uint32_t position, Storage::OpenFlags_t)
{

}

Storage::IRecord* LocalTransportSet::DeleteRecord(uint32_t position)
{

}

Storage::ISet* LocalTransportSet::OpenSet(uint32_t position, Storage::OpenFlags_t flags)
{

}

Storage::ISet* LocalTransportSet::DeleteSet(uint32_t position)
{

}

any_t LocalTransportRecord::GetValue(const string_t& name)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (!m_mapIndex.find(name,tag))
		return any_t();
	else if (tag.m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (tag.m_tag == Tag::eSet)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a set using GetValue"));

	any_t result;
	if (!m_mapValues.find(tag.m_idx,result))
		OMEGA_THROW("Record index out of sync");

	return result;
}

void LocalTransportRecord::SetValue(const string_t& name, const any_t& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (m_mapIndex.find(name,tag))
	{
		if (tag.m_tag == Tag::eRecord)
			throw IAlreadyExistsException::Create(OOCore::get_text("A sub-record with the name {0} already exists") % name);
		else if (tag.m_tag == Tag::eSet)
			throw IAlreadyExistsException::Create(OOCore::get_text("A set with the name {0} already exists") % name);
		else if (val.GetType() == TypeInfo::typeVoid)
			m_mapValues.remove(tag.m_idx);
		else
		{
			any_t* pv = m_mapValues.find(tag.m_idx);
			if (!pv)
				OMEGA_THROW("Record index out of sync");
			*pv = val;
		}
	}
	else
	{
		tag.m_tag = Tag::eValue;
		int err = m_mapValues.insert(val,tag.m_idx);
		if (err)
			OMEGA_THROW(err);

		err = m_mapIndex.insert(name,tag);
		if (err)
		{
			m_mapValues.remove(tag.m_idx);
			OMEGA_THROW(err);
		}
	}
}

Storage::IRecord* LocalTransportRecord::OpenRecord(const string_t& name, Storage::OpenFlags_t flags)
{
	if (flags < Storage::OpenExisting || flags > Storage::CreateNew)
		OMEGA_THROW("Invalid value for flags in call to IRecord::OpenRecord");

	ObjectPtr<ObjectImpl<LocalTransportRecord> > ptrRecord;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (!m_mapIndex.find(name,tag))
	{
		if (flags == Storage::OpenExisting)
			throw INotFoundException::Create(OOCore::get_text("The record contains no sub-record named {0}") % name);

		ptrRecord = ObjectImpl<LocalTransportRecord>::CreateObject();

		tag.m_tag = Tag::eRecord;
		int err = m_mapRecords.insert(ptrRecord,tag.m_idx);
		if (err)
			OMEGA_THROW(err);

		err = m_mapIndex.insert(name,tag);
		if (err)
		{
			m_mapRecords.remove(tag.m_idx);
			OMEGA_THROW(err);
		}
	}
	else
	{
		if (tag.m_tag == Tag::eValue)
			throw IAlreadyExistsException::Create(OOCore::get_text("A value with the name {0} already exists") % name);
		else if (tag.m_tag == Tag::eSet)
			throw IAlreadyExistsException::Create(OOCore::get_text("A set with the name {0} already exists") % name);
		else if (flags == Storage::CreateNew)
			throw IAlreadyExistsException::Create(OOCore::get_text("The record already contains a sub-record named {0}") % name);
		else if (!m_mapRecords.find(tag.m_idx,ptrRecord))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::IRecord*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Storage::IRecord)));
}

Storage::IRecord* LocalTransportRecord::DeleteRecord(const string_t& name)
{
	ObjectPtr<ObjectImpl<LocalTransportRecord> > ptrRecord;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (m_mapIndex.find(name,tag))
	{
		if (tag.m_tag == Tag::eValue)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a value using DeleteRecord"));
		else if (tag.m_tag == Tag::eSet)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a set using DeleteRecord"));
		else if (!m_mapRecords.find(tag.m_idx,ptrRecord))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::IRecord*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Storage::IRecord)));
}

Storage::ISet* LocalTransportRecord::OpenSet(const string_t& name, Storage::OpenFlags_t flags)
{
	if (flags < Storage::OpenExisting || flags > Storage::CreateNew)
		OMEGA_THROW("Invalid value for flags in call to IRecord::OpenRecord");

	ObjectPtr<ObjectImpl<LocalTransportSet> > ptrSet;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (!m_mapIndex.find(name,tag))
	{
		if (flags == Storage::OpenExisting)
			throw INotFoundException::Create(OOCore::get_text("The record contains no set named {0}") % name);

		ptrSet = ObjectImpl<LocalTransportSet>::CreateObject();

		tag.m_tag = Tag::eSet;
		int err = m_mapSets.insert(ptrSet,tag.m_idx);
		if (err)
			OMEGA_THROW(err);

		err = m_mapIndex.insert(name,tag);
		if (err)
		{
			m_mapSets.remove(tag.m_idx);
			OMEGA_THROW(err);
		}
	}
	else
	{
		if (tag.m_tag == Tag::eValue)
			throw IAlreadyExistsException::Create(OOCore::get_text("A value with the name {0} already exists") % name);
		else if (tag.m_tag == Tag::eRecord)
			throw IAlreadyExistsException::Create(OOCore::get_text("A sub-record with the name {0} already exists") % name);
		else if (flags == Storage::CreateNew)
			throw IAlreadyExistsException::Create(OOCore::get_text("The record already contains a set named {0}") % name);
		else if (!m_mapSets.find(tag.m_idx,ptrSet))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::ISet*>(ptrSet->QueryInterface(OMEGA_GUIDOF(Storage::ISet)));
}

Storage::ISet* LocalTransportRecord::DeleteSet(const string_t& name)
{
	ObjectPtr<ObjectImpl<LocalTransportSet> > ptrSet;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (m_mapIndex.find(name,tag))
	{
		if (tag.m_tag == Tag::eValue)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a value using DeleteSet"));
		else if (tag.m_tag == Tag::eRecord)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a sub-record using DeleteSet"));
		else if (!m_mapSets.find(tag.m_idx,ptrSet))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::ISet*>(ptrSet->QueryInterface(OMEGA_GUIDOF(Storage::ISet)));
}

const Omega::guid_t OOCore::OID_LocalTransportMarshalFactory("{EEBD74BA-1C47-F582-BF49-92DFC17D83DE}");

void OOCore::LocalTransportMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext* /*pMarshalContext*/, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{

}
