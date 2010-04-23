///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of Custom, the Omega Online WiX installer custom action.
//
// Custom is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Custom is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Custom.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <ntsecapi.h>
#include <lm.h>

#if defined(_DEBUG)
#error turn off _DEBUG
#endif

#include <malloc.h>
#include <string>
#include <sstream>
#include <stdarg.h>

static const wchar_t s_szUName[] = L"_OMEGA_SANDBOX_USER_";
static const wchar_t s_szPwd[] = L"4th_(*%LGe895y^$N|2";
static const wchar_t s_szPwdKey[] = L"L$OOServer_sandbox_pwd";

static std::wstring format_msg(DWORD dwErr, HMODULE hModule)
{
	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
	if (hModule)
		dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
	else
		dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;

	LPVOID lpBuf;
	if (::FormatMessageW(
		dwFlags,
		hModule,
		dwErr,
		0,
		(LPWSTR)&lpBuf,
		0,	NULL))
	{
		std::wstring res((LPCWSTR)lpBuf);
		while (*res.rbegin() == L'\r' || *res.rbegin() == L'\n')
			res = res.substr(0,res.size()-1);

		LocalFree(lpBuf);
		return res;
	}
	else
	{
		return L"Unknown error";
	}
}

static std::wstring FormatMessage(DWORD dwErr = GetLastError())
{
	std::wostringstream ret;
	ret.setf(std::ios_base::hex,std::ios_base::basefield);
	ret << "(0x" << dwErr << ") ";

	if (!(dwErr & 0xC0000000))
		ret << format_msg(dwErr,NULL);
	else
		ret << format_msg(dwErr,GetModuleHandleW(L"NTDLL.DLL"));
		
	return ret.str();
}

static void InstallMessage(MSIHANDLE hInstall, INSTALLMESSAGE msg_type, const wchar_t* pMsg, UINT vals = 0, ...)
{
	PMSIHANDLE hRecord = MsiCreateRecord(1);
	if (hRecord)
	{
		va_list argptr;
		va_start(argptr,vals);

		// field 0 is the template
		MsiRecordSetStringW(hRecord, 0, pMsg);

		for (UINT v=1;v<=vals;++v)
		{
			// field v, to be placed in [v] placeholder
			MsiRecordSetStringW(hRecord, v, va_arg(argptr,wchar_t*));
		}

		// send message to running installer
		MsiProcessMessage(hInstall, msg_type, hRecord);
	}
}

static UINT GetProperty(MSIHANDLE hInstall, const wchar_t* prop, std::wstring& val)
{
	DWORD dwLen = 0;
	UINT err = MsiGetPropertyW(hInstall,prop,L"",&dwLen);
	if (err == ERROR_MORE_DATA && dwLen > 0)
	{
		++dwLen;
		WCHAR* buf = static_cast<WCHAR*>(malloc(dwLen*sizeof(wchar_t)));
		if (!buf)
		{
			InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"Out of memory");
			return ERROR_INSTALL_FAILURE;
		}

		err = MsiGetPropertyW(hInstall,prop,buf,&dwLen);
		if (err == ERROR_SUCCESS)
			val.assign(buf,dwLen);
		else
			InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"Failed to access property [1]: [2]",2,prop,FormatMessage(err).c_str());
		
		free(buf);
	}

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}

static UINT SetProperty(MSIHANDLE hInstall, const wchar_t* prop, const wchar_t* val)
{
	UINT err = MsiSetPropertyW(hInstall,prop,val);
	if (err != ERROR_SUCCESS)
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"Failed to set property [1] to [2], [3]",3,prop,val,FormatMessage(err).c_str());

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}

static void BuildUnicodeString(LSA_UNICODE_STRING& strOut, const wchar_t* strIn)
{
	strOut.Buffer = const_cast<PWSTR>(strIn);
	size_t len = wcslen(strOut.Buffer);
	strOut.Length = static_cast<USHORT>(len * sizeof(wchar_t));
	strOut.MaximumLength = static_cast<USHORT>((len+1) * sizeof(wchar_t));
}

static UINT ParseUser(MSIHANDLE hInstall, std::wstring& strUName, std::wstring& strPwd)
{
	std::wstring all;
	UINT err = GetProperty(hInstall,L"CustomActionData",all);
	if (err != ERROR_SUCCESS)
		return err;

	size_t pos = all.find(L'|');
	if (pos == std::wstring::npos)
		strUName = all;
	else
	{
		strUName = all.substr(0,pos);
		strPwd = all.substr(pos+1);
	}

	if (strPwd == L"default")
	{
		// Open the local account policy...
		LSA_HANDLE hPolicy;
		LSA_OBJECT_ATTRIBUTES oa = {0};
		DWORD err = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_GET_PRIVATE_INFORMATION,&hPolicy));
		if (err != ERROR_SUCCESS)
		{
			InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT|MB_OK|MB_ICONERROR),L"LsaOpenPolicy failed: [1]",1,FormatMessage(err).c_str());
			return ERROR_INSTALL_FAILURE;
		}

		LSA_UNICODE_STRING szKey;
		BuildUnicodeString(szKey,s_szPwdKey);

		PLSA_UNICODE_STRING pszVal;
		if (LsaRetrievePrivateData(hPolicy,&szKey,&pszVal) == ERROR_SUCCESS)
		{
			strPwd.assign(pszVal->Buffer,pszVal->Length / sizeof(wchar_t));
			LsaFreeMemory(pszVal);

			InstallMessage(hInstall,INSTALLMESSAGE_INFO,L"Re-using existing sandbox password");
		}
		
		LsaClose(hPolicy);
		
		if (strPwd == L"default")
			strPwd = s_szPwd;
	}
	
	return ERROR_SUCCESS;
}

static int FindUName(MSIHANDLE hInstall, std::wstring& strUName)
{
	if (strUName == L"default")
	{
		// Default username
		strUName = s_szUName;

		UINT err = SetProperty(hInstall,L"OMEGASANDBOXUNAME",strUName.c_str());
		if (err != ERROR_SUCCESS)
			return err;
	}

	return ERROR_SUCCESS;
}

// 2 = Add, 1 = Update, 0 = No change, -1 = Failure
static int CheckUName(MSIHANDLE hInstall, const std::wstring& strUName)
{
	// Check if the user exists, and/or if it does, does it need updating?
	
	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa = {0};
	DWORD err = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_LOOKUP_NAMES,&hPolicy));
	if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT|MB_OK|MB_ICONERROR),L"LsaOpenPolicy failed: [1]",1,FormatMessage(err).c_str());
		return -1;
	}
		
	// Lookup the account SID
	LSA_UNICODE_STRING szUName;
	BuildUnicodeString(szUName,strUName.c_str());
	
	PLSA_REFERENCED_DOMAIN_LIST pDL = 0;
	PLSA_TRANSLATED_SID2 pSIDs = 0;
	err = LsaNtStatusToWinError(LsaLookupNames2(hPolicy,0,1,&szUName,&pDL,&pSIDs));
	if (err == ERROR_NONE_MAPPED || err == ERROR_TRUSTED_DOMAIN_FAILURE)
	{
		LsaClose(hPolicy);
		return 2;
	}
	else if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaLookupNames2 '[1]' failed: [2]",2,strUName.c_str(),FormatMessage(err).c_str());
		LsaClose(hPolicy);
		return -1;
	}

	int ret = -1;	
	if (pSIDs[0].Use != SidTypeUser)
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_ERROR|MB_OK|MB_ICONERROR),L"Account '[1]' is not a user account",1,strUName.c_str());
	else
	{
		// Validate account
		PLSA_UNICODE_STRING pRightsList;
		ULONG ulRightsCount = 0;
		err = LsaNtStatusToWinError(LsaEnumerateAccountRights(hPolicy,pSIDs[0].Sid,&pRightsList,&ulRightsCount));
		if (err != ERROR_SUCCESS)
			ret = 1;
		else
		{
			bool bFound = false;
			for (ULONG i=0;i<ulRightsCount && !bFound;i++)
			{
				if (wcscmp(pRightsList[i].Buffer,SE_BATCH_LOGON_NAME) == 0)
					bFound = true;
			}

			ret = (bFound ? 0 : 1);
		}
	}

	LsaFreeMemory(pDL);
	LsaFreeMemory(pSIDs);
	LsaClose(hPolicy);

	return ret;
}

extern "C" UINT __declspec(dllexport) __stdcall CheckUser(MSIHANDLE hInstall)
{
	std::wstring strUName;
	UINT err = GetProperty(hInstall,L"OMEGASANDBOXUNAME",strUName);
	if (err != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	err = FindUName(hInstall,strUName);
	if (err != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	int ret = CheckUName(hInstall,strUName);
	switch (ret)
	{
	case 2:
		InstallMessage(hInstall,INSTALLMESSAGE_INFO,L"New local user [1] required",1,strUName.c_str());
		err = SetProperty(hInstall,L"OOServer.AddUser.DoAdd",L"yes");
		break;

	case 1:
		InstallMessage(hInstall,INSTALLMESSAGE_INFO,L"User [1] requires updating to add SeBatchLogonRight privilege",1,strUName.c_str());
		err = SetProperty(hInstall,L"OOServer.AddUser.DoAdd",L"update");
		break;

	case 0:
		InstallMessage(hInstall,INSTALLMESSAGE_INFO,L"User [1] is valid",1,strUName.c_str());
		break;

	default:
	case -1:
		err = ERROR_INSTALL_FAILURE;
		break;
	}

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}

extern "C" UINT __declspec(dllexport) __stdcall AddUser(MSIHANDLE hInstall)
{
	std::wstring strUName,strPwd;
	if (ParseUser(hInstall,strUName,strPwd) != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	USER_INFO_2	info =
	{
		(LPWSTR)strUName.c_str(),  // usri2_name;
		(LPWSTR)strPwd.c_str(),    // usri2_password;
		0,                         // usri2_password_age;
		USER_PRIV_USER,            // usri2_priv;
		NULL,                      // usri2_home_dir;
		L"This account is used by the Omega Online sandbox to control access to system resources", // usri2_comment;
		UF_SCRIPT | UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT, // usri2_flags;
		NULL,                      // usri2_script_path;
		0,                         // usri2_auth_flags;
		L"Omega Online sandbox user account",   // usri2_full_name;
		L"This account is used by the Omega Online sandbox to control access to system resources", // usri2_usr_comment;
		0,                         // usri2_parms;
		NULL,                      // usri2_workstations;
		0,                         // usri2_last_logon;
		0,                         // usri2_last_logoff;
		TIMEQ_FOREVER,             // usri2_acct_expires;
		USER_MAXSTORAGE_UNLIMITED, // usri2_max_storage;
		0,                         // usri2_units_per_week;
		NULL,                      // usri2_logon_hours;
		(DWORD)-1,                 // usri2_bad_pw_count;
		(DWORD)-1,                 // usri2_num_logons;
		NULL,                      // usri2_logon_server;
		0,                         // usri2_country_code;
		0,                         // usri2_code_page;
	};
	NET_API_STATUS nerr = NetUserAdd(NULL,2,(LPBYTE)&info,NULL);
	if (nerr != NERR_Success)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"NetUserAdd failed: [1]",1,FormatMessage(nerr).c_str());
		return ERROR_INSTALL_FAILURE;
	}

	// Add reg key
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,L"Software\\Omega Online\\OOServer\\Install",0,NULL,0,KEY_WRITE,NULL,&hKey,NULL) == ERROR_SUCCESS)
	{
		RegSetValueExW(hKey,L"added_user",0,REG_SZ,(LPBYTE)strUName.c_str(),(strUName.size()+1)*sizeof(wchar_t));
		RegCloseKey(hKey);
	}

	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa = {0};
	DWORD err = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy));
	if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaOpenPolicy failed: [1]",1,FormatMessage(err).c_str());
		return ERROR_INSTALL_FAILURE;
	}
		
	// Lookup the account SID
	LSA_UNICODE_STRING szUName;
	BuildUnicodeString(szUName,strUName.c_str());
	
	PLSA_REFERENCED_DOMAIN_LIST pDL = 0;
	PLSA_TRANSLATED_SID2 pSIDs = 0;
	err = LsaNtStatusToWinError(LsaLookupNames2(hPolicy,0,1,&szUName,&pDL,&pSIDs));
	if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaLookupNames2 '[1]' failed: [2]",2,strUName.c_str(),FormatMessage(err).c_str());
		LsaClose(hPolicy);
		return ERROR_INSTALL_FAILURE;
	}

	if (pSIDs[0].Use != SidTypeUser)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"Account '[1]' is not a user account",1,strUName.c_str());
		err = ERROR_INSTALL_FAILURE;
	}
	else
	{
		// Add correct right...
		LSA_UNICODE_STRING szName;
		BuildUnicodeString(szName,SE_BATCH_LOGON_NAME);
		
		err = LsaNtStatusToWinError(LsaAddAccountRights(hPolicy,pSIDs[0].Sid,&szName,1));
		if (err != ERROR_SUCCESS)
			InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaAddAccountRights failed: [1]",1,FormatMessage(err).c_str());
		else
		{
			// Remove all other priviledges
			PLSA_UNICODE_STRING pRightsList;
			ULONG ulRightsCount = 0;
			err = LsaNtStatusToWinError(LsaEnumerateAccountRights(hPolicy,pSIDs[0].Sid,&pRightsList,&ulRightsCount));
			if (err != ERROR_SUCCESS)
				InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaEnumerateAccountRights failed: [1]",1,FormatMessage(err).c_str());
			else
			{
				for (ULONG i=0;i<ulRightsCount;i++)
				{
					if (wcscmp(pRightsList[i].Buffer,SE_BATCH_LOGON_NAME) != 0)
					{
						err = LsaNtStatusToWinError(LsaRemoveAccountRights(hPolicy,pSIDs[0].Sid,FALSE,&pRightsList[i],1));
						if (err != ERROR_SUCCESS)
						{
							InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaRemoveAccountRights failed: [1]",1,FormatMessage(err).c_str());
							break;
						}
					}
				}
			}
		}
	}

	LsaFreeMemory(pDL);
	LsaFreeMemory(pSIDs);

	if (err == ERROR_SUCCESS)
	{
		LSA_UNICODE_STRING szKey;
		BuildUnicodeString(szKey,s_szPwdKey);

		LSA_UNICODE_STRING szVal;
		BuildUnicodeString(szVal,strPwd.c_str());

		err = LsaNtStatusToWinError(LsaStorePrivateData(hPolicy,&szKey,&szVal));
	}

	LsaClose(hPolicy);

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}

extern "C" UINT __declspec(dllexport) __stdcall RemoveUser(MSIHANDLE hInstall)
{
	// Ignore all errors here... we don't stop...
	std::wstring strUName,strPwd;
	if (ParseUser(hInstall,strUName,strPwd) == ERROR_SUCCESS)
	{
		// Check reg key
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"Software\\Omega Online\\OOServer\\Install",0,KEY_READ | KEY_WRITE,&hKey) == ERROR_SUCCESS)
		{
			wchar_t szBuf[1024] = {0};
			DWORD dwLen = 1023;
			RegQueryValueExW(hKey,L"added_user",NULL,NULL,(LPBYTE)&szBuf,&dwLen);
			
			if (strUName == szBuf && NetUserDel(NULL,strUName.c_str()) == NERR_Success)
			{
				// Remove added key
				RegDeleteValueW(hKey,L"added_user");

				// Remove the password
				LSA_HANDLE hPolicy;
				LSA_OBJECT_ATTRIBUTES oa = {0};
				if (LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy) == ERROR_SUCCESS)
				{
					LSA_UNICODE_STRING szKey;
					BuildUnicodeString(szKey,s_szPwdKey);

					LsaStorePrivateData(hPolicy,&szKey,NULL);

					LsaClose(hPolicy);
				}
			}
						
			RegCloseKey(hKey);
		}
	}
		
	return ERROR_SUCCESS;
}

extern "C" UINT __declspec(dllexport) __stdcall UpdateUser(MSIHANDLE hInstall)
{
	std::wstring strUName,strPwd;
	if (ParseUser(hInstall,strUName,strPwd) != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa = {0};
	DWORD err = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy));
	if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaOpenPolicy failed: [1]",1,FormatMessage(err).c_str());
		return ERROR_INSTALL_FAILURE;
	}
		
	// Lookup the account SID
	LSA_UNICODE_STRING szUName;
	BuildUnicodeString(szUName,strUName.c_str());
	
	PLSA_REFERENCED_DOMAIN_LIST pDL = 0;
	PLSA_TRANSLATED_SID2 pSIDs = 0;
	err = LsaNtStatusToWinError(LsaLookupNames2(hPolicy,0,1,&szUName,&pDL,&pSIDs));
	if (err != ERROR_SUCCESS)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaLookupNames2 '[1]' failed: [2]",2,strUName.c_str(),FormatMessage(err).c_str());
		LsaClose(hPolicy);
		return ERROR_INSTALL_FAILURE;
	}

	if (pSIDs[0].Use != SidTypeUser)
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"Account '[1]' is not a user account",1,strUName.c_str());
		err = ERROR_INSTALL_FAILURE;
	}
	else
	{
		// Add correct right...
		LSA_UNICODE_STRING szName;
		BuildUnicodeString(szName,SE_BATCH_LOGON_NAME);
		
		err = LsaNtStatusToWinError(LsaAddAccountRights(hPolicy,pSIDs[0].Sid,&szName,1));
		if (err != ERROR_SUCCESS)
			InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"LsaAddAccountRights failed: [1]",1,FormatMessage(err).c_str());
	}

	LsaFreeMemory(pDL);
	LsaFreeMemory(pSIDs);
	LsaClose(hPolicy);

	// Confirm logon is possible...
	HANDLE hToken;
	if (!LogonUserW((LPWSTR)strUName.c_str(),NULL,(LPWSTR)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,&hToken))
	{
		InstallMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_FATALEXIT |MB_OK|MB_ICONERROR),L"User account '[1]' has an invalid password or is unsuitable",1,strUName.c_str());
		err = ERROR_INSTALL_FAILURE;
	}
	else
		CloseHandle(hToken);

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}
