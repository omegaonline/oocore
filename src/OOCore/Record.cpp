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

#include "Record.h"

using namespace Omega;
using namespace OTL;

uint32_t OOCore::Array::GetCount()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_max)
	{
		for (OOBase::HashTable<uint32_t,Tag,OOBase::CrtAllocator>::iterator i=m_mapIndex.begin();i!=m_mapIndex.end();++i)
		{
			if (i->key > m_max)
				m_max = i->key;
		}
	}

	return m_max;
}

any_t OOCore::Array::GetValue(uint32_t position)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<uint32_t,Tag,OOBase::CrtAllocator>::iterator i = m_mapIndex.find(position);
	if (i == m_mapIndex.end())
		return any_t();
	else if (i->value.m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (i->value.m_tag == Tag::eArray)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get an array using GetValue"));

	OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator>::iterator j = m_mapValues.find(i->value.m_idx);
	if (j == m_mapValues.end())
		OMEGA_THROW("Array index out of sync");

	return j->value;
}

void OOCore::Array::SetValue(uint32_t position, const any_t& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<uint32_t,Tag,OOBase::CrtAllocator>::iterator i = m_mapIndex.find(position);
	if (i != m_mapIndex.end())
	{
		if (i->value.m_tag == Tag::eRecord)
			throw IAlreadyExistsException::Create(OOCore::get_text("A sub-record at position {0} already exists") % position);
		else if (i->value.m_tag == Tag::eArray)
			throw IAlreadyExistsException::Create(OOCore::get_text("An array at position {0} already exists") % position);
		else if (val.GetType() == TypeInfo::typeVoid)
		{
			m_mapValues.remove(i->value.m_idx);

			if (position == m_max)
				m_max = 0;
		}
		else
		{
			OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator>::iterator j = m_mapValues.find(i->value.m_idx);
			if (j == m_mapValues.end())
				OMEGA_THROW("Record index out of sync");
			j->value = val;
		}
	}
	else
	{
		Tag tag;
		tag.m_tag = Tag::eValue;
		int err = m_mapValues.insert(val,tag.m_idx);
		if (err)
			OMEGA_THROW(err);

		err = m_mapIndex.insert(position,tag);
		if (err)
		{
			m_mapValues.remove(tag.m_idx);
			OMEGA_THROW(err);
		}

		if (position > m_max)
			m_max = position;
	}
}

Storage::IRecord* OOCore::Array::OpenRecord(uint32_t position, Storage::OpenFlags_t)
{
	return NULL;
}

Storage::IRecord* OOCore::Array::DeleteRecord(uint32_t position)
{
	return NULL;
}

Storage::IArray* OOCore::Array::OpenArray(uint32_t position, Storage::OpenFlags_t flags)
{
	return NULL;
}

Storage::IArray* OOCore::Array::DeleteArray(uint32_t position)
{
	return NULL;
}

any_t OOCore::Record::GetValue(const string_t& name)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i == m_mapIndex.end())
		return any_t();
	else if (i->value.m_tag == Tag::eRecord)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get a sub-record using GetValue"));
	else if (i->value.m_tag == Tag::eArray)
		throw IAccessDeniedException::Create(OOCore::get_text("Attempt to get an array using GetValue"));

	OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator>::iterator j = m_mapValues.find(i->value.m_idx);
	if (j == m_mapValues.end())
		OMEGA_THROW("Record index out of sync");

	return j->value;
}

void OOCore::Record::SetValue(const string_t& name, const any_t& val)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i != m_mapIndex.end())
	{
		if (i->value.m_tag == Tag::eRecord)
			throw IAlreadyExistsException::Create(OOCore::get_text("A sub-record with the name {0} already exists") % name);
		else if (i->value.m_tag == Tag::eArray)
			throw IAlreadyExistsException::Create(OOCore::get_text("An array with the name {0} already exists") % name);
		else if (val.GetType() == TypeInfo::typeVoid)
			m_mapValues.remove(i->value.m_idx);
		else
		{
			OOBase::HandleTable<size_t,any_t,OOBase::CrtAllocator>::iterator j = m_mapValues.find(i->value.m_idx);
			if (j == m_mapValues.end())
				OMEGA_THROW("Record index out of sync");
			j->value = val;
		}
	}
	else
	{
		Tag tag;
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

Storage::IRecord* OOCore::Record::OpenRecord(const string_t& name, Storage::OpenFlags_t flags)
{
	if (flags < Storage::OpenExisting || flags > Storage::CreateNew)
		OMEGA_THROW("Invalid value for flags in call to IRecord::OpenRecord");

	ObjectPtr<ObjectImpl<Record> > ptrRecord;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i == m_mapIndex.end())
	{
		if (flags == Storage::OpenExisting)
			throw INotFoundException::Create(OOCore::get_text("The record contains no sub-record named {0}") % name);

		ptrRecord = ObjectImpl<Record>::CreateObject();

		Tag tag;
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
		if (i->value.m_tag == Tag::eValue)
			throw IAlreadyExistsException::Create(OOCore::get_text("A value with the name {0} already exists") % name);
		else if (i->value.m_tag == Tag::eArray)
			throw IAlreadyExistsException::Create(OOCore::get_text("An array with the name {0} already exists") % name);
		else if (flags == Storage::CreateNew)
			throw IAlreadyExistsException::Create(OOCore::get_text("The record already contains a sub-record named {0}") % name);
		else
		{
			OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<Record> >,OOBase::CrtAllocator>::iterator j = m_mapRecords.find(i->value.m_idx);
			if (j == m_mapRecords.end())
				OMEGA_THROW("Record index out of sync");

			ptrRecord = j->value;
		}
	}

	return static_cast<Storage::IRecord*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Storage::IRecord)));
}

Storage::IRecord* OOCore::Record::DeleteRecord(const string_t& name)
{
	ObjectPtr<ObjectImpl<Record> > ptrRecord;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i != m_mapIndex.end())
	{
		if (i->value.m_tag == Tag::eValue)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a value using DeleteRecord"));
		else if (i->value.m_tag == Tag::eArray)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete an array using DeleteRecord"));
		else
		{
			OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<Record> >,OOBase::CrtAllocator>::iterator j = m_mapRecords.find(i->value.m_idx);
			if (j == m_mapRecords.end())
				OMEGA_THROW("Record index out of sync");

			ptrRecord = j->value;
		}
	}

	return static_cast<Storage::IRecord*>(ptrRecord->QueryInterface(OMEGA_GUIDOF(Storage::IRecord)));
}

Storage::IArray* OOCore::Record::OpenArray(const string_t& name, Storage::OpenFlags_t flags)
{
	if (flags < Storage::OpenExisting || flags > Storage::CreateNew)
		OMEGA_THROW("Invalid value for flags in call to IRecord::OpenRecord");

	ObjectPtr<ObjectImpl<Array> > ptrArray;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i == m_mapIndex.end())
	{
		if (flags == Storage::OpenExisting)
			throw INotFoundException::Create(OOCore::get_text("The record contains no array named {0}") % name);

		ptrArray = ObjectImpl<Array>::CreateObject();

		Tag tag;
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
		if (i->value.m_tag == Tag::eValue)
			throw IAlreadyExistsException::Create(OOCore::get_text("A value with the name {0} already exists") % name);
		else if (i->value.m_tag == Tag::eRecord)
			throw IAlreadyExistsException::Create(OOCore::get_text("A sub-record with the name {0} already exists") % name);
		else if (flags == Storage::CreateNew)
			throw IAlreadyExistsException::Create(OOCore::get_text("The record already contains an array named {0}") % name);
		else
		{
			OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<Array> >,OOBase::CrtAllocator>::iterator j = m_mapArrays.find(i->value.m_idx);
			if (j == m_mapArrays.end())
				OMEGA_THROW("Record index out of sync");

			ptrArray = j->value;
		}
	}

	return static_cast<Storage::IArray*>(ptrArray->QueryInterface(OMEGA_GUIDOF(Storage::IArray)));
}

Storage::IArray* OOCore::Record::DeleteArray(const string_t& name)
{
	ObjectPtr<ObjectImpl<Array> > ptrArray;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	OOBase::HashTable<string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash>::iterator i = m_mapIndex.find(name);
	if (i != m_mapIndex.end())
	{
		if (i->value.m_tag == Tag::eValue)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a value using DeleteArray"));
		else if (i->value.m_tag == Tag::eRecord)
			throw IAccessDeniedException::Create(OOCore::get_text("Attempt to delete a sub-record using DeleteArray"));
		else
		{
			OOBase::HandleTable<size_t,ObjectPtr<ObjectImpl<Array> >,OOBase::CrtAllocator>::iterator j = m_mapArrays.find(i->value.m_idx);
			if (j == m_mapArrays.end())
				OMEGA_THROW("Record index out of sync");

			ptrArray = j->value;
		}
	}

	return static_cast<Storage::IArray*>(ptrArray->QueryInterface(OMEGA_GUIDOF(Storage::IArray)));
}
