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

#ifndef OOCORE_RECORD_H_INCLUDED_
#define OOCORE_RECORD_H_INCLUDED_

namespace OOCore
{
	class Record;

	class Array :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Omega::Storage::IArray
	{
	public:
		Array() : m_max(0)
		{}

		BEGIN_INTERFACE_MAP(Array)
			INTERFACE_ENTRY(Omega::Storage::IArray)
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
		Omega::uint32_t  m_max;
		OOBase::HashTable<Omega::uint32_t,Tag,OOBase::CrtAllocator> m_mapIndex;
		OOBase::HandleTable<size_t,Omega::any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,OTL::ObjectPtr<OTL::ObjectImpl<Array> >,OOBase::CrtAllocator> m_mapArrays;
		OOBase::HandleTable<size_t,OTL::ObjectPtr<OTL::ObjectImpl<Record> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		Omega::uint32_t GetCount();

		Omega::any_t GetValue(Omega::uint32_t position);
		void SetValue(Omega::uint32_t position, const Omega::any_t& val);

		Omega::Storage::IRecord* OpenRecord(Omega::uint32_t position, Omega::Storage::OpenFlags_t flags = Omega::Storage::OpenExisting);
		Omega::Storage::IRecord* DeleteRecord(Omega::uint32_t position);

		Omega::Storage::IArray* OpenArray(Omega::uint32_t position, Omega::Storage::OpenFlags_t flags = Omega::Storage::OpenExisting);
		Omega::Storage::IArray* DeleteArray(Omega::uint32_t position);
	};

	class Record :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Omega::Storage::IRecord
	{
	public:
		BEGIN_INTERFACE_MAP(Record)
			INTERFACE_ENTRY(Omega::Storage::IRecord)
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
		OOBase::HashTable<Omega::string_t,Tag,OOBase::CrtAllocator,OOCore::StringHash> m_mapIndex;
		OOBase::HandleTable<size_t,Omega::any_t,OOBase::CrtAllocator> m_mapValues;
		OOBase::HandleTable<size_t,OTL::ObjectPtr<OTL::ObjectImpl<Array> >,OOBase::CrtAllocator> m_mapArrays;
		OOBase::HandleTable<size_t,OTL::ObjectPtr<OTL::ObjectImpl<Record> >,OOBase::CrtAllocator> m_mapRecords;

	public:
		Omega::any_t GetValue(const Omega::string_t& name);
		void SetValue(const Omega::string_t& name, const Omega::any_t& val);

		Omega::Storage::IRecord* OpenRecord(const Omega::string_t& name, Omega::Storage::OpenFlags_t flags = Omega::Storage::OpenExisting);
		Omega::Storage::IRecord* DeleteRecord(const Omega::string_t& name);

		Omega::Storage::IArray* OpenArray(const Omega::string_t& name, Omega::Storage::OpenFlags_t flags = Omega::Storage::OpenExisting);
		Omega::Storage::IArray* DeleteArray(const Omega::string_t& name);
	};
}

#endif // OOCORE_RECORD_H_INCLUDED_
