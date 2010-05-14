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

#ifndef OOCORE_LOOP_CHANNEL_H_INCLUDED_
#define OOCORE_LOOP_CHANNEL_H_INCLUDED_

#include "Channel.h"

namespace OOCore
{
	class LoopChannel :
			public ChannelBase
	{
	public:
		static Omega::IObject* create(Omega::uint32_t channel_id, const Omega::guid_t& message_oid, const Omega::guid_t& iid);

		BEGIN_INTERFACE_MAP(LoopChannel)
			INTERFACE_ENTRY_CHAIN(ChannelBase)
		END_INTERFACE_MAP()

	protected:
		LoopChannel() {}

	public:
		Omega::bool_t IsConnected();
		void GetManager(const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
	};
}

#endif // OOCORE_LOOP_CHANNEL_H_INCLUDED_
