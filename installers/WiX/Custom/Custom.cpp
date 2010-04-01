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

extern "C" UINT __declspec(dllexport) __stdcall CheckUser(MSIHANDLE hInstall)
{
	std::wstring all;
	UINT err = GetProperty(hInstall,L"OMEGASANDBOXUNAME",all);
	if (err != ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	
	bool bDoAdd = true;

	// Check if the user exists, and/or if it does, does it need updating?

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
