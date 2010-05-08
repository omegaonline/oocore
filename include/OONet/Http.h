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

#ifndef OOCORE_HTTP_H_INCLUDED_
#define OOCORE_HTTP_H_INCLUDED_

#include <Omega/Omega.h>

namespace Omega
{
	namespace Net
	{
		namespace Http
		{
			interface IRequestNotify : public IObject
			{
				virtual void OnResponseStart(uint16_t nCode, const string_t& strMsg) = 0;
				virtual void OnResponseDataAvailable() = 0;
				virtual void OnResponseComplete() = 0;
				virtual void OnError(IException* pE) = 0;
			};

			interface IRequest : public IObject
			{
				typedef std::multimap<Omega::string_t,Omega::string_t> headers_t;

				virtual void Open(const string_t& strMethod, const string_t& strURL, IRequestNotify* pAsyncNotify = 0) = 0;
				virtual void Send(uint32_t cbBytes = 0, const byte_t* pData = 0) = 0;
				virtual void SetRequestHeader(const string_t& strHeader, const string_t& strValue) = 0;
				virtual uint16_t Status() = 0;
				virtual string_t StatusText() = 0;
				virtual headers_t GetResponseHeaders() = 0;
				virtual void ResponseBody(uint32_t& cbBytes, byte_t* pBody) = 0;
				virtual IO::IStream* ResponseStream() = 0;
				virtual void Abort() = 0;
				virtual bool_t WaitForResponse(uint32_t timeout) = 0;
			};

			// {72B33743-6E02-49a1-89C3-C8B988492EF4}
			OMEGA_EXPORT_OID(OID_StdHttpRequest);

			//inline void SplitURL(const string_t& strURL, string_t& strScheme, string_t& strHost, string_t& strPort, string_t& strUserName, string_t& strPassword, string_t& strResource, string_t& strQuery);

			// These interfaces are used for server-side processing
			namespace Server
			{
				interface IRequest : public IObject
				{
					typedef std::multimap<Omega::string_t,Omega::string_t> headers_t;

					virtual string_t Method() = 0;
					virtual string_t Resource() = 0;
					virtual headers_t GetRequestHeaders() = 0;
					virtual void RequestBody(uint32_t& cbBytes, byte_t* pBody) = 0;
					virtual IO::IStream* RequestStream() = 0;
				};

				interface IResponse : public IObject
				{
					virtual void SetResponseHeader(const string_t& strHeader, const string_t& strValue) = 0;
					virtual IO::IStream* Send(uint16_t uStatus, const string_t& strStatusText) = 0;
				};

				interface IRequestHandler : public IObject
				{
					virtual void Open(const string_t& strAbsURI) = 0;
					virtual void ProcessRequest(IRequest* pRequest, IResponse* pResponse) = 0;
				};
			}
		}
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net::Http, IRequestNotify, "{F47AB4ED-6C4C-4e9a-8502-850BA314A9CC}",

	// Methods
	OMEGA_METHOD_VOID(OnResponseStart,2,((in),uint16_t,nCode,(in),const string_t&,strMsg))
	OMEGA_METHOD_VOID(OnResponseDataAvailable,0,())
	OMEGA_METHOD_VOID(OnResponseComplete,0,())
	OMEGA_METHOD_VOID(OnError,1,((in),IException*,pE))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net::Http, IRequest, "{93EAA9CD-EADB-4b7b-9416-1C5699E050BE}",

	// Methods
	OMEGA_METHOD_VOID(Open,3,((in),const string_t&,strMethod,(in),const string_t&,strURL,(in),Net::Http::IRequestNotify*,pAsyncNotify))
	OMEGA_METHOD_VOID(SetRequestHeader,2,((in),const string_t&,strHeader,(in),const string_t&,strValue))
	OMEGA_METHOD_EX_VOID(Synchronous,30,Send,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,pData))
	OMEGA_METHOD(uint16_t,Status,0,())
	OMEGA_METHOD(string_t,StatusText,0,())
	OMEGA_METHOD(Omega::Net::Http::IRequest::headers_t,GetResponseHeaders,0,())
	OMEGA_METHOD_VOID(ResponseBody,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,pBody))
	OMEGA_METHOD(IO::IStream*,ResponseStream,0,())
	OMEGA_METHOD_VOID(Abort,0,())
	OMEGA_METHOD(bool_t,WaitForResponse,1,((in),uint32_t,timeout))
)

/*OMEGA_EXPORTED_FUNCTION_VOID(OOCore_Net_Http_SplitURL,8,((in),const Omega::string_t&,strURL,(out),Omega::string_t&,strScheme,(out),Omega::string_t&,strHost,(out),Omega::string_t&,strPort,(out),Omega::string_t&,strUserName,(out),Omega::string_t&,strPassword,(out),Omega::string_t&,strResource,(out),Omega::string_t&,strQuery))
void Omega::Net::Http::SplitURL(const Omega::string_t& strURL, Omega::string_t& strScheme, Omega::string_t& strHost, Omega::string_t& strPort, Omega::string_t& strUserName, Omega::string_t& strPassword, Omega::string_t& strResource, Omega::string_t& strQuery)
{
    OOCore_Net_Http_SplitURL(strURL,strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);
}*/

OMEGA_DEFINE_INTERFACE
(
	Omega::Net::Http::Server, IRequest, "{D25F4D19-A890-4b8c-A857-122A7A6884FE}",

	// Methods
	OMEGA_METHOD(string_t,Method,0,())
	OMEGA_METHOD(string_t,Resource,0,())
	OMEGA_METHOD(Omega::Net::Http::Server::IRequest::headers_t,GetRequestHeaders,0,())
	OMEGA_METHOD_VOID(RequestBody,2,((in),uint32_t&,cbBytes,(in)(size_is(cbBytes)),byte_t*,pBody))
	OMEGA_METHOD(IO::IStream*,RequestStream,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net::Http::Server, IResponse, "{2919F248-FE7A-4497-A592-44E1F4F02927}",

	// Methods
	OMEGA_METHOD_VOID(SetResponseHeader,2,((in),const string_t&,strHeader,(in),const string_t&,strValue))
	OMEGA_METHOD(IO::IStream*,Send,2,((in),uint16_t,uStatus,(in),const string_t&,strStatusText))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net::Http::Server, IRequestHandler, "{8533785B-4F7E-4f60-8159-FA827AE12459}",

	// Methods
	OMEGA_METHOD_VOID(Open,1,((in),const string_t&,strAbsURI))
	OMEGA_METHOD_VOID(ProcessRequest,2,((in),Net::Http::Server::IRequest*,pRequest,(in),Net::Http::Server::IResponse*,pResponse))
)

#endif // !defined(DOXYGEN)

#endif // OOCORE_HTTP_H_INCLUDED_
