///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#include "./OOServer_User.h"
#include "./NetHttp.h"
#include "./NetTcp.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_HttpProtocolHandler,"{EDB0676F-70B0-4e49-AACC-E8478F615277}");

namespace User
{
#if defined(OMEGA_WIN32)

	// Use the WinHTTP stuff under Win32
	class WinHttpWrapper
	{
	public:
		WinHttpWrapper()
		{
			m_hLibrary = LoadLibraryW(L"WinHttp.dll");
			if (m_hLibrary)
			{
				m_pIE = (pfnWinHttpGetIEProxyConfigForCurrentUser)GetProcAddress(m_hLibrary,"WinHttpGetIEProxyConfigForCurrentUser");
				m_pOpen = (pfnWinHttpOpen)GetProcAddress(m_hLibrary,"WinHttpOpen");
				m_pGP = (pfnWinHttpGetProxyForUrl)GetProcAddress(m_hLibrary,"WinHttpGetProxyForUrl");
				m_pClose = (pfnWinHttpCloseHandle)GetProcAddress(m_hLibrary,"WinHttpCloseHandle");
			}
		}

		~WinHttpWrapper()
		{
			if (m_hLibrary)
				FreeLibrary(m_hLibrary);
		}

		bool WinHttpGetProxyForUrl(const string_t& strURL, string_t& strProxy)
		{
			if (!m_pOpen || !m_pGP || !m_pClose)
				return false;

			HINTERNET hHttpSession = (*m_pOpen)(L"WinHTTP AutoProxy Sample/1.0",1,NULL,NULL,0);
			if (!hHttpSession)
				return false;

			WINHTTP_AUTOPROXY_OPTIONS options = {0};
			options.dwFlags = 1;
			options.dwAutoDetectFlags = 1 | 2;
			options.fAutoLogonIfChallenged = TRUE;

			WINHTTP_PROXY_INFO result = {0};

			bool bOk = false;
			if ((*m_pGP)(hHttpSession,strURL.c_str(),&options,&result))
			{
				if (result.dwAccessType == 3)
					strProxy = result.lpszProxy;

				if (result.lpszProxy)
					GlobalFree(result.lpszProxy);

				if (result.lpszProxyBypass)
					GlobalFree(result.lpszProxyBypass);

				bOk = true;
			}

			(*m_pClose)(hHttpSession);

			return bOk;
		}

		bool WinHttpGetIEProxyConfigForCurrentUser(const string_t& strURL, string_t& strProxy)
		{
			if (!m_pIE)
				return false;

			WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config = {0};

			if (!(*m_pIE)(&config))
				return false;

			bool bOk = false;
			if (config.lpszAutoConfigUrl)
				bOk = WinHttpGetProxyFromPAC(strURL,config.lpszAutoConfigUrl,strProxy);

			if (!bOk && config.lpszProxy)
				strProxy = config.lpszProxy;

			if (config.lpszAutoConfigUrl)
				GlobalFree(config.lpszAutoConfigUrl);

			if (config.lpszProxy)
				GlobalFree(config.lpszProxy);

			if (config.lpszProxyBypass)
				GlobalFree(config.lpszProxyBypass);

			return bOk;
		}

	private:
		typedef LPVOID HINTERNET;

		typedef struct
		{
			BOOL    fAutoDetect;
			LPWSTR  lpszAutoConfigUrl;
			LPWSTR  lpszProxy;
			LPWSTR  lpszProxyBypass;
		} WINHTTP_CURRENT_USER_IE_PROXY_CONFIG;

		typedef struct
		{
			DWORD   dwFlags;
			DWORD   dwAutoDetectFlags;
			LPCWSTR lpszAutoConfigUrl;
			LPVOID  lpvReserved;
			DWORD   dwReserved;
			BOOL    fAutoLogonIfChallenged;
		}
		WINHTTP_AUTOPROXY_OPTIONS;

		typedef struct
		{
			DWORD  dwAccessType;
			LPWSTR lpszProxy;
			LPWSTR lpszProxyBypass;
		}
		WINHTTP_PROXY_INFO;

		typedef BOOL (WINAPI *pfnWinHttpGetIEProxyConfigForCurrentUser)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG * pProxyConfig);
		typedef HINTERNET (WINAPI *pfnWinHttpOpen)(LPCWSTR pwszUserAgent,DWORD dwAccessType,LPCWSTR pwszProxyName, LPCWSTR pwszProxyBypass, DWORD dwFlags);
		typedef BOOL (WINAPI *pfnWinHttpGetProxyForUrl)(HINTERNET hSession, LPCWSTR lpcwszUrl, WINHTTP_AUTOPROXY_OPTIONS *pAutoProxyOptions, WINHTTP_PROXY_INFO *pProxyInfo);
		typedef BOOL (WINAPI *pfnWinHttpCloseHandle)(HINTERNET hInternet);

		pfnWinHttpGetIEProxyConfigForCurrentUser m_pIE;
		pfnWinHttpOpen                           m_pOpen;
		pfnWinHttpGetProxyForUrl                 m_pGP;
		pfnWinHttpCloseHandle                    m_pClose;
		HINSTANCE                                m_hLibrary;

		bool WinHttpGetProxyFromPAC(const string_t& strURL, const string_t strPAC, string_t& strProxy)
		{
			if (!m_pOpen || !m_pGP || !m_pClose)
				return false;

			HINTERNET hHttpSession = (*m_pOpen)(L"WinHTTP AutoProxy Sample/1.0",1,NULL,NULL,0);
			if (!hHttpSession)
				return false;

			WINHTTP_AUTOPROXY_OPTIONS options = {0};
			options.dwFlags = 2;
			options.dwAutoDetectFlags = 1 | 2;
			options.lpszAutoConfigUrl = strPAC.c_str();
			options.fAutoLogonIfChallenged = TRUE;

			WINHTTP_PROXY_INFO result = {0};

			bool bOk = false;
			if ((*m_pGP)(hHttpSession,strURL.c_str(),&options,&result))
			{
				if (result.dwAccessType == 3)
					strProxy = result.lpszProxy;

				if (result.lpszProxy)
					GlobalFree(result.lpszProxy);

				if (result.lpszProxyBypass)
					GlobalFree(result.lpszProxyBypass);

				bOk = true;
			}

			(*m_pClose)(hHttpSession);

			return bOk;
		}
	};
	static WinHttpWrapper win_http_wrapper;

#endif

}

User::HttpProtocolHandler::HttpProtocolHandler()
{
	if (m_db.open(":memory:") == 0)
	{
		const char szSQL[] = 
			"BEGIN TRANSACTION; "
			"CREATE TABLE Proxy "
			"("
				"URL TEXT NOT NULL, "
				"ProxyURL TEXT, "
				"UNIQUE(URL) "
			");"
			"CREATE INDEX IF NOT EXISTS idx_Proxy ON Proxy(URL);"
			"ANALYZE Proxy;"
			"COMMIT;";
				
		m_db.exec(szSQL);
	}
}

Net::IConnectedStream* User::HttpProtocolHandler::OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify)
{
	// Find the TCP protocol handler
	string_t strHandler;
	ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\tcp"))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\tcp");
		if (ptrKey->IsValue(L"Handler"))
			strHandler = ptrKey->GetStringValue(L"Handler");
	}
	
	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\tcp"))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\tcp");
			if (ptrKey->IsValue(L"Handler"))
				strHandler = ptrKey->GetStringValue(L"Handler");
		}
	}

	guid_t oid = guid_t::Null();
	if (!strHandler.IsEmpty())
	{
		oid = guid_t::FromString(strHandler);
		if (oid == guid_t::Null())
			oid = Activation::NameToOid(strHandler);
	}
	
	if (oid == guid_t::Null())
		oid = OID_TcpProtocolHandler;

	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Get the protocol
	string_t strProtocol = strEndpoint.Left(pos);

	// Check for localhost
	string_t strAddr = strEndpoint.Mid(pos+3);
	size_t pos2 = strAddr.Find(L':');
	if (pos2 != string_t::npos)
		strAddr = strAddr.Left(pos2);

	// Find the proxy if any...
	string_t strProxy;
	if (strAddr.CompareNoCase(L"127.0.0.1")==0 || 
		strAddr.CompareNoCase(L"localhost")==0)
	{
		strProxy = strEndpoint.Mid(pos+3);
	}
	else
	{
		strProxy = FindProxy(strEndpoint,strProtocol);
		if (strProxy.IsEmpty())
			strProxy = strEndpoint.Mid(pos+3);
	}

	// Build the correct proxy address...
	string_t strEnd = L"tcp:" + strProxy.ToLower();
	
	// Make sure we have a default port
	if (strProxy.Find(L':') == string_t::npos)
	{
		if (strProtocol == L"http")
			strEnd += L":80";		
		else if (strProtocol == L"https")
			strEnd += L":443";
	}
	
	// Create a Protocol Handler
	OTL::ObjectPtr<Omega::Net::IProtocolHandler> ptrHandler(oid);

	// Create a stream
	return ptrHandler->OpenStream(strEnd,pNotify);
}

string_t User::HttpProtocolHandler::FindProxy(const string_t& strURL, const string_t& strProtocol)
{
	// Check our local db first
	ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> ptrStmt = 
		m_db.prepare_statement("SELECT ProxyURL FROM Proxy WHERE URL = %Q;",strURL.ToUTF8().c_str());
	
	if (!ptrStmt.null() && ptrStmt->step() == SQLITE_ROW)
		return string_t(ptrStmt->column_text(0),true);
	
	string_t strProxy;

	ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"Proxy"))
			strProxy = ptrKey->GetStringValue(L"Proxy");
	}

	if (strProxy.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"Proxy"))
				strProxy = ptrKey->GetStringValue(L"Proxy");
		}
	}

	// 'none' is valid...
	if (strProxy== L"none")
		strProxy.Clear();
	else if (strProxy.IsEmpty())
	{
#if defined(OMEGA_WIN32)
		if (!win_http_wrapper.WinHttpGetProxyForUrl(strURL,strProxy))
			win_http_wrapper.WinHttpGetIEProxyConfigForCurrentUser(strURL,strProxy);
#else
		strProxy = string_t(ACE_OS::getenv("http_proxy"),false);
#endif
	}

	// Update database
	ptrStmt = m_db.prepare_statement("INSERT INTO Proxy (URL,ProxyURL) VALUES (%Q,%Q);",strURL.ToUTF8().c_str(),strProxy.ToUTF8().c_str());
	if (!ptrStmt.null())
		ptrStmt->step();

	return strProxy;
}
