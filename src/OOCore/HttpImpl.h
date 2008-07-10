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

#ifndef OOCORE_HTTPIMPL_H_INCLUDED_
#define OOCORE_HTTPIMPL_H_INCLUDED_

#include <OOCore/Http.h>

namespace OOCore
{
	class HttpRequest :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<HttpRequest,&Omega::Net::Http::OID_StdHttpRequest,Omega::Activation::InProcess>,
		public Omega::Net::Http::IRequest
	{
	public:
		BEGIN_INTERFACE_MAP(HttpRequest)
			INTERFACE_ENTRY(Omega::Net::Http::IRequest)
		END_INTERFACE_MAP()

	private:
		OTL::ObjectPtr<Omega::Net::Http::IRequest> m_ptrImpl;
		bool                                       m_bAsync;

	// IRequest members
	public:
		void Open(const Omega::string_t& strMethod, const Omega::string_t& strURL, Omega::Net::Http::IRequestNotify* pAsyncNotify);
		void SetRequestHeader(const Omega::string_t& strHeader, const Omega::string_t& strValue);
		void Send(Omega::uint32_t cbBytes, const Omega::byte_t* pData);
		Omega::uint16_t Status();
		Omega::string_t StatusText();
		Omega::IEnumString* GetAllResponseHeaders();
		Omega::string_t GetResponseHeader(const Omega::string_t& strHeader);
		void ResponseBody(Omega::uint32_t& cbBytes, Omega::byte_t* pBody);
		Omega::IO::IStream* ResponseStream();
		void Abort();
		Omega::bool_t WaitForResponse(Omega::uint32_t timeout);
	};
}

#endif // OOCORE_HTTPIMPL_H_INCLUDED_
