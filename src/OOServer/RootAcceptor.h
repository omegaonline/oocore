///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_ACCEPTOR_H_INCLUDED_
#define OOSERVER_ROOT_ACCEPTOR_H_INCLUDED_

#include "OOServer_Root.h"

namespace Root
{
	class MessageHandler;

	namespace UserAcceptor
	{
		OOSvrBase::AsyncSocket* accept(MessageHandler* message_handler, const std::string& path, OOBase::Socket::uid_t uid, int* perr, const OOBase::timeval_t* wait = 0);
	}
}

#endif // OOSERVER_ROOT_ACCEPTOR_H_INCLUDED_
