#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#if defined(_DEBUG)
#error turn off _DEBUG
#endif

#include <malloc.h>
#include <string>

static UINT GetProperty(MSIHANDLE hInstall, const wchar_t* prop, std::wstring& val)
{
	DWORD dwLen = 0;
	UINT err = MsiGetPropertyW(hInstall,prop,L"",&dwLen);
	if (err == ERROR_MORE_DATA && dwLen > 0)
	{
		++dwLen;
		WCHAR* buf = static_cast<WCHAR*>(malloc(dwLen*sizeof(wchar_t)));
		if (!buf)
			return ERROR_INSTALL_FAILURE;

		err = MsiGetPropertyW(hInstall,prop,buf,&dwLen);
		if (err == ERROR_SUCCESS)
			val.assign(buf,dwLen);
		
		free(buf);
	}

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
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
	
	return ERROR_SUCCESS;
}

// 2 = Add, 1 = Update, 0 = No change, -1 = Failure
static int CheckUName(const std::wstring& strUName)
{
	// Check if the user exists, and/or if it does, does it need updating?
	// Lookup the account SID
	DWORD dwSidSize = 0;
	DWORD dwDnSize = 0;
	SID_NAME_USE use;
	if (!LookupAccountNameW(NULL,strUName.c_str(),NULL,&dwSidSize,NULL,&dwDnSize,&use))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return 2;
	}
	else if (dwSidSize==0)
		return -1;
	
	PSID pSid = static_cast<PSID>(malloc(dwSidSize));
	if (!pSid)
		return -1;

	wchar_t* pszDName = (wchar_t*)malloc(dwDnSize * sizeof(wchar_t));
	if (!pszDName)
	{
		free(pSid);
		return -1;
	}

	if (!LookupAccountNameW(NULL,strUName.c_str(),pSid,&dwSidSize,pszDName,&dwDnSize,&use))
	{
		free(pSid);
		free(pszDName);
		return -1;
	}

	free(pSid);
	free(pszDName);

	if (use != SidTypeUser)
		return 1;

	return 0;
}

extern "C" UINT __declspec(dllexport) __stdcall CheckUser(MSIHANDLE hInstall)
{
	std::wstring strUName;
	UINT err = GetProperty(hInstall,L"OMEGASANDBOXUNAME",strUName);
	if (err != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	::MessageBoxW(NULL,strUName.c_str(),L"CheckUser",MB_OK | MB_ICONINFORMATION);
	
	bool bDoAdd = true;

	CheckUName(strUName);

	if (bDoAdd)
		err = MsiSetPropertyW(hInstall,L"OOServer.AddUser.DoAdd",L"yes");

	return (err == ERROR_SUCCESS ? err : ERROR_INSTALL_FAILURE);
}

extern "C" UINT __declspec(dllexport) __stdcall AddUser(MSIHANDLE hInstall)
{
	std::wstring strUName,strPwd;
	UINT err = ParseUser(hInstall,strUName,strPwd);
	if (err != ERROR_SUCCESS)
		return err;

	::MessageBoxW(NULL,strUName.c_str(),L"Add User",MB_OK | MB_ICONINFORMATION);
	
	return ERROR_INSTALL_FAILURE;
}

extern "C" UINT __declspec(dllexport) __stdcall RollbackAddUser(MSIHANDLE hInstall)
{
	std::wstring strUName,strPwd;
	UINT err = ParseUser(hInstall,strUName,strPwd);
	if (err != ERROR_SUCCESS)
		return err;
		
	::MessageBoxW(NULL,strUName.c_str(),L"Rollback User",MB_OK | MB_ICONINFORMATION);
	
	return err;
}

extern "C" UINT __declspec(dllexport) __stdcall DisableUser(MSIHANDLE hInstall)
{
	::MessageBoxW(NULL,L"",L"Remove User",MB_OK | MB_ICONINFORMATION);

	return ERROR_SUCCESS;
}
