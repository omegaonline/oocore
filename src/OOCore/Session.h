/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE HEADER FILE *****
//
//	It will be included by modules run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOCORE_SESSION_H_INCLUDED_
#define OOCORE_SESSION_H_INCLUDED_

#include <ace/SString.h>

#ifdef ACE_WIN32
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include <OOCore/Preprocessor/base.h>

namespace Session
{
#ifdef ACE_WIN32
	typedef DWORD TOKEN;
#else
	typedef uid_t TOKEN;
#endif

// Make sure we are packed 
#if (defined(_MSC_VER) && _MSC_VER>=1300)
#pragma pack(push, 1)
#endif

	struct Request
	{
		typedef ACE_UINT16 Length;

        Length		cbSize;
		TOKEN		uid;
	};

	struct Response
	{
		typedef ACE_UINT16 Length;

		Length		cbSize;
		char		bFailure;
		union
		{
			int		err;
			u_short	uNewPort;
		};
	};

#if (defined(_MSC_VER) && _MSC_VER>=1300)
#pragma pack(pop)
#endif

	inline ACE_TString GetBootstrapFileName()
	{
	#define BOOTSTRAP_FILE "ooserver.bootstrap"

	#ifdef ACE_WIN32

		ACE_TCHAR szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPath(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			return ACE_TString(ACE_TEXT(OMEGA_CONCAT("C:\\",BOOTSTRAP_FILE)));

		ACE_TCHAR szBuf2[MAX_PATH] = {0};
		if (!PathCombine(szBuf2,szBuf,ACE_TEXT(BOOTSTRAP_FILE)))
			return ACE_TString(ACE_TEXT(OMEGA_CONCAT("C:\\",BOOTSTRAP_FILE)));

		return ACE_TString(szBuf2);

	#else

		#define BOOTSTRAP_PREFIX ACE_TEXT("/tmp/")
		
		return ACE_TString(ACE_TEXT(OMEGA_CONCAT(BOOTSTRAP_PREFIX,BOOTSTRAP_FILE)));

	#endif
	}


};

#endif // OOCORE_SESSION_H_INCLUDED_