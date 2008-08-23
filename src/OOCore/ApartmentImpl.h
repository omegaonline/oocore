///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_APARTMENT_IMPL_H_INCLUDED_
#define OOCORE_APARTMENT_IMPL_H_INCLUDED_

namespace OOCore
{
	struct Message;
	class UserSession;

	class Apartment
	{
	public:
		Apartment(UserSession* pSession, ACE_CDR::UShort id);

		void close();

		void process_channel_close(ACE_CDR::ULong closed_channel_id);
		bool is_channel_open(ACE_CDR::ULong channel_id);

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_channel_om(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
		void process_request(const Message* pMsg, const ACE_Time_Value& deadline);
		
	private:
		ACE_RW_Thread_Mutex m_lock;
		UserSession*        m_pSession;
		ACE_CDR::UShort     m_id;

		std::map<ACE_CDR::ULong,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;
	};
}

#endif // OOCORE_APARTMENT_IMPL_H_INCLUDED_
