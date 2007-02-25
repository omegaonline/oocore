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
#include <ace/OS.h>

#if defined(ACE_WIN32)
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include <OOCore/Preprocessor/base.h>

namespace Session
{
#if defined(ACE_WIN32)
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
	#define OMEGA_BOOTSTRAP_FILE "ooserver.bootstrap"

	#if defined(ACE_WIN32)

		ACE_TString strFilename = ACE_TEXT(OMEGA_CONCAT("C:\\",OMEGA_BOOTSTRAP_FILE));

		ACE_TCHAR szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPath(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_CURRENT,szBuf);
		if SUCCEEDED(hr)
		{
			ACE_TCHAR szBuf2[MAX_PATH] = {0};
			if (PathCombine(szBuf2,szBuf,ACE_TEXT("OmegaOnline")))
			{
				if (!PathFileExists(szBuf2) && ACE_OS::mkdir(szBuf2) != 0)
					return strFilename;
			
				if (PathCombine(szBuf,szBuf2,ACE_TEXT(OMEGA_BOOTSTRAP_FILE)))
					strFilename = szBuf;
			}
		}

		return strFilename;

	#else

		void* TODO; // Sort this out!

		return ACE_TString(ACE_TEXT(OMEGA_CONCAT("/tmp/",OMEGA_BOOTSTRAP_FILE)));

	#endif
	}


};

#endif // OOCORE_SESSION_H_INCLUDED_