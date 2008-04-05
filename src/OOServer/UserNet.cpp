///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer.h"

#include "./UserManager.h"

using namespace Omega;
using namespace OTL;

bool User::Manager::route_off(const ACE_Message_Block* mb, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs)
{
	void* TICKET_92;

	return MessageHandler::route_off(mb,dest_channel_id,deadline,attribs);
}
