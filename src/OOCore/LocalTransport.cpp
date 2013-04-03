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

	class LocalTransportArray :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Storage::IArray
	{
	public:
		BEGIN_INTERFACE_MAP(LocalTransportArray)
			INTERFACE_ENTRY(Storage::IArray)
		END_INTERFACE_MAP()

	private:
		struct Tag
		{
			enum Type
			{
				eValue,
				eRecord,
				eArray
			} m_tag;
			size_t m_idx;
		};

		OOBase::SpinLock m_lock;
		OOBase::Vector<Tag,OOBase::CrtAllocator> m_vecIndex;
		OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportArray> >,OOBase::CrtAllocator> m_mapArrays;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportRecord> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		uint32_t GetCount();

		any_t GetValue(uint32_t position);
		void SetValue(uint32_t position, const any_t& val);

		Storage::IRecord* OpenRecord(uint32_t position, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IRecord* DeleteRecord(uint32_t position);

		Storage::IArray* OpenArray(uint32_t position, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IArray* DeleteArray(uint32_t position);
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
				eArray
			} m_tag;
			size_t m_idx;
		};

		OOBase::SpinLock m_lock;
		OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash> m_mapIndex;
		OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportArray> >,OOBase::CrtAllocator> m_mapArrays;
		OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<LocalTransportRecord> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		any_t GetValue(const string_t& name);
		void SetValue(const string_t& name, const any_t& val);

		Storage::IRecord* OpenRecord(const string_t& name, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IRecord* DeleteRecord(const string_t& name);

		Storage::IArray* OpenArray(const string_t& name, Storage::OpenFlags_t flags = Storage::OpenExisting);
		Storage::IArray* DeleteArray(const string_t& name);
	};
}

uint32_t LocalTransportArray::GetCount()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	return m_vecIndex.size();
}

any_t LocalTransportArray::GetValue(uint32_t position)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag* tag = m_vecIndex.at(position);
	if (!tag)
		return any_t();
	else if (tag->m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (tag->m_tag == Tag::eArray)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get an array using GetValue"));

	any_t result;
	if (!m_mapValues.find(tag->m_idx,result))
		OMEGA_THROW("Array index out of sync");

	return result;
}

void LocalTransportArray::SetValue(uint32_t position, const any_t& val)
{

}

Storage::IRecord* LocalTransportArray::OpenRecord(uint32_t position, Storage::OpenFlags_t)
{
	return NULL;
}

Storage::IRecord* LocalTransportArray::DeleteRecord(uint32_t position)
{
	return NULL;
}

Storage::IArray* LocalTransportArray::OpenArray(uint32_t position, Storage::OpenFlags_t flags)
{
	return NULL;
}

Storage::IArray* LocalTransportArray::DeleteArray(uint32_t position)
{
	return NULL;
}

any_t LocalTransportRecord::GetValue(const string_t& name)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (!m_mapIndex.find(name,tag))
		return any_t();
	else if (tag.m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (tag.m_tag == Tag::eArray)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get an array using GetValue"));

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
		else if (tag.m_tag == Tag::eArray)
			throw IAlreadyExistsException::Create(OOCore::get_text("An array with the name {0} already exists") % name);
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
		else if (tag.m_tag == Tag::eArray)
			throw IAlreadyExistsException::Create(OOCore::get_text("An array with the name {0} already exists") % name);
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
		else if (tag.m_tag == Tag::eArray)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete an array using DeleteRecord"));
		else if (!m_mapRecords.find(tag.m_idx,ptrRecord))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::IRecord*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Storage::IRecord)));
}

Storage::IArray* LocalTransportRecord::OpenArray(const string_t& name, Storage::OpenFlags_t flags)
{
	if (flags < Storage::OpenExisting || flags > Storage::CreateNew)
		OMEGA_THROW("Invalid value for flags in call to IRecord::OpenRecord");

	ObjectPtr<ObjectImpl<LocalTransportArray> > ptrArray;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (!m_mapIndex.find(name,tag))
	{
		if (flags == Storage::OpenExisting)
			throw INotFoundException::Create(OOCore::get_text("The record contains no array named {0}") % name);

		ptrArray = ObjectImpl<LocalTransportArray>::CreateObject();

		tag.m_tag = Tag::eArray;
		int err = m_mapArrays.insert(ptrArray,tag.m_idx);
		if (err)
			OMEGA_THROW(err);

		err = m_mapIndex.insert(name,tag);
		if (err)
		{
			m_mapArrays.remove(tag.m_idx);
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
			throw IAlreadyExistsException::Create(OOCore::get_text("The record already contains an array named {0}") % name);
		else if (!m_mapArrays.find(tag.m_idx,ptrArray))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::IArray*>(ptrArray->QueryInterface(OMEGA_GUIDOF(Storage::IArray)));
}

Storage::IArray* LocalTransportRecord::DeleteArray(const string_t& name)
{
	ObjectPtr<ObjectImpl<LocalTransportArray> > ptrArray;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Tag tag;
	if (m_mapIndex.find(name,tag))
	{
		if (tag.m_tag == Tag::eValue)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a value using DeleteArray"));
		else if (tag.m_tag == Tag::eRecord)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a sub-record using DeleteArray"));
		else if (!m_mapArrays.find(tag.m_idx,ptrArray))
			OMEGA_THROW("Record index out of sync");
	}

	return static_cast<Storage::IArray*>(ptrArray->QueryInterface(OMEGA_GUIDOF(Storage::IArray)));
}

void OOCore::LocalTransport::init(OOBase::CDRStream& stream, OOBase::Proactor* proactor)
{

}

Remoting::IMessage* OOCore::LocalTransport::CreateMessage()
{
	ObjectPtr<ObjectImpl<LocalTransportRecord> > ptrRecord = ObjectImpl<LocalTransportRecord>::CreateObject();
	return static_cast<Remoting::IMessage*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Remoting::IMessage)));
}

void OOCore::LocalTransport::SendMessage(Remoting::IMessage* pMessage)
{

}

string_t OOCore::LocalTransport::GetURI()
{
	return "local://pidfile";
}

const guid_t OOCore::OID_LocalTransportMarshalFactory("{EEBD74BA-1C47-F582-BF49-92DFC17D83DE}");

void OOCore::LocalTransportMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext* /*pMarshalContext*/, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{

}
