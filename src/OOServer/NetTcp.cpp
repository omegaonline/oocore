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

#include "./NetTcp.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_TcpProtocolHandler,"{4924E463-06A4-483b-9DAD-8BFD83ADCBFC}");

IO::IStream* User::TcpProtocolHandler::OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	return 0;
}
