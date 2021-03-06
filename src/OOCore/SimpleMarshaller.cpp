///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

#include "CDRMessage.h"

using namespace Omega;
using namespace OTL;

namespace
{
	class SimpleMarshaller :
			public ObjectBase,
			public Remoting::IMarshaller
	{
	public:
		SimpleMarshaller();
		void init(Remoting::MarshalFlags_t marshal_flags);
		
		BEGIN_INTERFACE_MAP(SimpleMarshaller)
			INTERFACE_ENTRY(Remoting::IMarshaller)
		END_INTERFACE_MAP()

	private:
		Remoting::MarshalFlags_t m_marshal_flags;

	// IMarshaller members
	public:
		void MarshalInterface(const string_t& name, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject);
		void ReleaseMarshalData(const string_t& name, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject);
		void UnmarshalInterface(const string_t& name, Remoting::IMessage* pMessage, const guid_t& iid, IObject*& pObject);
		Remoting::IMessage* CreateMessage();
		IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv);
		uint32_t GetSource();
	};
}

SimpleMarshaller::SimpleMarshaller() : m_marshal_flags(Remoting::Same)
{
}

void SimpleMarshaller::init(Remoting::MarshalFlags_t marshal_flags)
{
	m_marshal_flags = marshal_flags;
}

void SimpleMarshaller::MarshalInterface(const string_t& strName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	// Write a header
	pMessage->WriteStructStart(strName,string_t::constant("$iface_marshal"));

	// See if object is NULL
	if (!pObject)
		pMessage->WriteValue(string_t::constant("$marshal_type"),byte_t(0));
	else
	{
		ObjectPtr<Remoting::IMarshal> ptrMarshal;

		// See if pObject is a SafeProxy wrapping a WireProxy...
		ObjectPtr<Remoting::IProxy> ptrProxy = Remoting::GetProxy(pObject);
		if (ptrProxy)
			ptrMarshal = ptrProxy.QueryInterface<Remoting::IMarshal>();

		// See if pObject does custom marshalling...
		if (!ptrMarshal)
			ptrMarshal = OTL::QueryInterface<Remoting::IMarshal>(pObject);

		// See if custom marshalling is possible...
		if (!ptrMarshal)
			OMEGA_THROW("Attempting to marshal incompatible object via SimpleMarshaller");

		guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(iid,m_marshal_flags);
		if (oid == guid_t::Null())
			OMEGA_THROW("Attempting to marshal incompatible object via SimpleMarshaller");

		// Write the marshalling oid
		pMessage->WriteValue(string_t::constant("$marshal_type"),byte_t(2));
		pMessage->WriteValue(string_t::constant("$oid"),oid);

		try
		{
			// Let the custom handle marshalling...
			ptrMarshal->MarshalInterface(this,pMessage,iid,m_marshal_flags);
		}
		catch (...)
		{
			pMessage->ReadValue(string_t::constant("$marshal_type"));
			pMessage->ReadValue(string_t::constant("$oid"));
			ptrMarshal->ReleaseMarshalData(this,pMessage,iid,m_marshal_flags);
			throw;
		}
	}

	// Write the struct end
	pMessage->WriteStructEnd();
}

void SimpleMarshaller::ReleaseMarshalData(const string_t& strName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	// Read the header
	pMessage->ReadStructStart(strName,string_t::constant("$iface_marshal"));

	byte_t flag = pMessage->ReadValue(string_t::constant("$marshal_type")).cast<byte_t>();
	if (flag == 0)
	{
		/* NOP */
	}
	else if (flag == 1)
	{
		OMEGA_THROW("Invalid marshal flag for SimpleMarshaller");
	}
	else if (flag == 2)
	{
		// Skip the guid...
		pMessage->ReadValue(string_t::constant("$oid"));

		// See if pObject does custom marshalling...
		ObjectPtr<Remoting::IMarshal> ptrMarshal;
		if (pObject)
			ptrMarshal = OTL::QueryInterface<Remoting::IMarshal>(pObject);

		if (!ptrMarshal)
			throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshal));

		ptrMarshal->ReleaseMarshalData(this,pMessage,iid,m_marshal_flags);
	}
	else
	{
		OMEGA_THROW("Invalid marshal flag");
	}

	pMessage->ReadStructEnd();
}

Remoting::IMessage* SimpleMarshaller::CreateMessage()
{
	OMEGA_THROW("Cannot call CreateMessage() on SimpleMarshaller");
}

void SimpleMarshaller::UnmarshalInterface(const string_t& /*strName*/, Remoting::IMessage* /*pMessage*/, const guid_t& /*iid*/, IObject*& /*pObject*/)
{
	OMEGA_THROW("Cannot call UnmarshalInterface() on SimpleMarshaller");
}

IException* SimpleMarshaller::SendAndReceive(TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* /*pSend*/, Remoting::IMessage*& /*pRecv*/)
{
	OMEGA_THROW("Cannot call SendAndReceive() on SimpleMarshaller");
}

uint32_t SimpleMarshaller::GetSource()
{
	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RespondException,2,((in),Remoting::IMessage*,pMessage,(in),IException*,pException))
{
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrPayload = ObjectImpl<OOCore::CDRMessage>::CreateObject();
	ObjectPtr<ObjectImpl<SimpleMarshaller> > ptrMarshaller = ObjectImpl<SimpleMarshaller>::CreateObject();

	ptrPayload->WriteStructStart(string_t::constant("ipc_response"),string_t::constant("$ipc_response_type"));
	ptrPayload->WriteValue(string_t::constant("$throw"),true);

	guid_t iid = pException->GetThrownIID();
	ObjectPtr<IObject> ptrQI = pException->QueryInterface(iid);
	if (ptrQI)
		ptrMarshaller->MarshalInterface(string_t::constant("exception"),ptrPayload,iid,ptrQI);
	else
		ptrMarshaller->MarshalInterface(string_t::constant("exception"),ptrPayload,OMEGA_GUIDOF(IException),pException);

	ptrPayload->WriteStructEnd();
	
	ptrMarshaller->MarshalInterface(string_t::constant("payload"),pMessage,OMEGA_GUIDOF(Remoting::IMessage),static_cast<Remoting::IMessage*>(ptrPayload));
}
