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

#ifndef OOCORE_SIMPLE_MARSHALLER_H_INCLUDED_
#define OOCORE_SIMPLE_MARSHALLER_H_INCLUDED_

namespace OOCore
{
	class SimpleMarshalContext :
			public OTL::ObjectBase,
			public Omega::Remoting::IMarshalContext
	{
	public:
		SimpleMarshalContext();
		void init(Omega::Remoting::MarshalFlags_t marshal_flags);
		
		BEGIN_INTERFACE_MAP(SimpleMarshalContext)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalContext)
		END_INTERFACE_MAP()

	private:
		Omega::Remoting::MarshalFlags_t m_marshal_flags;

	// IMarshalContext members
	public:
		void MarshalInterface(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void ReleaseMarshalData(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv);
		Omega::uint32_t GetSource();
	};
}


#endif // OOCORE_SIMPLE_MARSHALLER_H_INCLUDED_
