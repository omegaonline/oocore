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

void OOCore::LocalTransport::init(OOBase::CDRStream& stream, OOBase::Proactor* proactor, const OOBase::Timeout& timeout)
{
	OOBase::RefPtr<OOBase::Buffer> ptrBuffer = OOBase::Buffer::create(1500);
	if (!ptrBuffer)
		throw ISystemException::OutOfMemory();

	int err = 0;
	pid_t pid = 0;

#if defined(_WIN32)
	OOBase::StackAllocator<128> allocator;
	OOBase::LocalString str(allocator);
	if (!stream.read(pid) || !stream.read_string(str))
		OMEGA_THROW(stream.last_error());

	m_ptrSocket = proactor->connect(str.c_str(),err,timeout);
	if (err)
		OMEGA_THROW(err);

#elif defined(HAVE_UNISTD_H)
	int fd_ = -1;
	if (!stream.read(pid) || !stream.read(fd_))
		OMEGA_THROW(stream.last_error());

	OOBase::POSIX::SmartFD fd(fd_);

	m_ptrSocket = proactor->attach(fd,err);
	if (err)
		OMEGA_THROW(err);

	fd.detach();
#endif

	err = m_ptrSocket->recv(this,&LocalTransport::on_recv1,ptrBuffer,s_header_length);
	if (err)
	{
		m_ptrSocket = NULL;
		OMEGA_THROW(err);
	}

	m_strName = string_t::constant("local://{0}") % pid;
}

void OOCore::LocalTransport::on_recv1(OOBase::Buffer* buffer, int err)
{
	uint32_t nReadLen = 0;
	if (!err && buffer->length() == s_header_length)
	{
		OOBase::CDRStream header(buffer);

		// Read the payload specific data
		header.read_endianess();

		// Read the version byte
		byte_t version = 1;
		header.read(version);
		if (version >= 1)
		{
			// Room for 1 byte here!

			// Read the length
			header.read(nReadLen);
		}

		err = header.last_error();
	}

	if (!err && nReadLen)
	{
		buffer->reset();
		err = m_ptrSocket->recv(this,&LocalTransport::on_recv2,buffer,nReadLen);
	}

	if (err || !nReadLen)
		on_close(err);
}

void OOCore::LocalTransport::on_recv2(OOBase::Buffer* buffer, int err)
{
	if (!err)
	{

	}

	if (!err)
	{
		buffer->reset();
		if (buffer->space() > 4096)
		{
			OOBase::RefPtr<OOBase::Buffer> ptrBuffer = OOBase::Buffer::create(1500);
			if (!ptrBuffer)
				err = ERROR_OUTOFMEMORY;
			else
				buffer = ptrBuffer;
		}

		if (!err)
			err = m_ptrSocket->recv(this,&LocalTransport::on_recv1,buffer,s_header_length);
	}

	if (err)
		on_close(err);
}

void OOCore::LocalTransport::on_close(int err)
{
	// Notify...
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	for (OOBase::HandleTable<uint32_t,ObjectPtr<Remoting::ITransportNotify> >::iterator i=m_mapNotify.begin();i != m_mapNotify.end();++i)
		i->value->OnClose();

	guard.release();

	// Close the socket
	m_ptrSocket = NULL;
}

Remoting::IMessage* OOCore::LocalTransport::CreateMessage()
{
	return ObjectImpl<OOCore::CDRMessage>::CreateObject();
}

void OOCore::LocalTransport::SendMessage(Remoting::IMessage* pMessage)
{

}

string_t OOCore::LocalTransport::GetURI()
{
	return m_strName;
}

uint32_t OOCore::LocalTransport::RegisterNotify(const guid_t& iid, IObject* pObject)
{
	uint32_t nCookie = 0;

	if (iid == OMEGA_GUIDOF(Remoting::ITransportNotify))
	{
		ObjectPtr<Remoting::ITransportNotify> ptrNotify = OTL::QueryInterface<Remoting::ITransportNotify>(pObject);
		if (!ptrNotify)
			throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::ITransportNotify));

		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		int err = m_mapNotify.insert(ptrNotify,nCookie);
		if (err)
			OMEGA_THROW(err);
	}

	return nCookie;
}

void OOCore::LocalTransport::UnregisterNotify(uint32_t cookie)
{
	if (cookie)
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		m_mapNotify.remove(cookie);
	}
}

Notify::INotifier::iid_list_t OOCore::LocalTransport::ListNotifyInterfaces()
{
	Notify::INotifier::iid_list_t list;
	list.push_back(OMEGA_GUIDOF(Remoting::ITransportNotify));
	return list;
}

const guid_t OOCore::OID_LocalTransportMarshalFactory("{EEBD74BA-1C47-F582-BF49-92DFC17D83DE}");

void OOCore::LocalTransportMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext* /*pMarshalContext*/, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{

}
