///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#include "LoopChannel.h"

using namespace Omega;
using namespace OTL;

namespace 
{
	class LoopMarshaller :
		public ObjectBase,
		public Remoting::IMarshaller
	{
	public:
		LoopMarshaller() : m_pChannel(0)
		{}

		void init(OOCore::LoopChannel* pChannel);

		BEGIN_INTERFACE_MAP(LoopMarshaller)
			INTERFACE_ENTRY(Remoting::IMarshaller)
		END_INTERFACE_MAP()

	private:
		OOCore::LoopChannel* m_pChannel;

	// IMarshaller members
	public:
		void MarshalInterface(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject);
		void ReleaseMarshalData(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject);
		void UnmarshalInterface(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t& iid, IObject*& pObject);
		Remoting::IMessage* CreateMessage();
		IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout = 0);
	};
}

void LoopMarshaller::init(OOCore::LoopChannel* pChannel)
{
	// We write pointers in uint64_ts
	static_assert(sizeof(Omega::IObject*) <= sizeof(uint64_t),"How wide are your pointers?");

	m_pChannel = pChannel;
}

void LoopMarshaller::MarshalInterface(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t&, IObject* pObject)
{
	pMessage->WriteStructStart(name,L"$loop_marshal");
	pMessage->WriteUInt64(L"ptr64",reinterpret_cast<uint64_t>(pObject));
	pMessage->WriteStructEnd(name);

	// Make sure we AddRef()
	pObject->AddRef();
}

void LoopMarshaller::ReleaseMarshalData(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t&, IObject* pObject)
{
	// Make sure we Release()
	pObject->Release();

	pMessage->ReadStructStart(name,L"$loop_marshal");
	pMessage->ReadUInt64(L"ptr64");
	pMessage->ReadStructEnd(name);
}

void LoopMarshaller::UnmarshalInterface(const wchar_t* name, Remoting::IMessage* pMessage, const guid_t&, IObject*& pObject)
{
	pMessage->ReadStructStart(name,L"$loop_marshal");

	pObject = reinterpret_cast<IObject*>(pMessage->ReadUInt64(L"ptr64"));

	pMessage->ReadStructEnd(name);
}

Remoting::IMessage* LoopMarshaller::CreateMessage()
{
	return m_pChannel->CreateMessage();
}

IException* LoopMarshaller::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	return m_pChannel->SendAndReceive(attribs,pSend,pRecv,timeout);
}

IObject* OOCore::LoopChannel::create(uint32_t channel_id, const guid_t& message_oid, const guid_t& iid)
{
	ObjectPtr<ObjectImpl<LoopChannel> > ptrChannel = ObjectImpl<LoopChannel>::CreateInstancePtr();

	ptrChannel->init(channel_id,Remoting::Same,0,message_oid);

	return ptrChannel->QueryInterface(iid);
}

IException* OOCore::LoopChannel::SendAndReceive(TypeInfo::MethodAttributes_t, Remoting::IMessage*, Remoting::IMessage*&, uint32_t)
{
	throw Remoting::IChannelClosedException::Create();
}

void OOCore::LoopChannel::GetManager(const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<ObjectImpl<LoopMarshaller> > ptrMarshaller = ObjectImpl<LoopMarshaller>::CreateInstancePtr();
	ptrMarshaller->init(this);

	pObject = ptrMarshaller->QueryInterface(iid);
}