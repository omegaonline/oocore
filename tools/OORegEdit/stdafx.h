
#define _UNICODE
#define UNICODE
#include <tchar.h>

#include <OTL/OTL.h>

#include <map>

#define wxUSE_GUI 1

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/fs_zip.h>
#include <wx/filesys.h>
#include <wx/image.h>
#include <wx/clipbrd.h>
#include <wx/valgen.h>
#include <wx/docview.h>
#include <wx/html/htmlwin.h>
