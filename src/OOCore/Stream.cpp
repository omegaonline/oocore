///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::IO::IStream*,Omega_IO_OpenStream,2,((in),const Omega::string_t&,strEndPoint,(in),Omega::IO::IAsyncStreamCallback*,pCallback))
{
	uint32_t nStreamId = OOCore::GetInterProcessService()->OpenStream(strEndPoint);

	return 0;
}
