
#include <OTL/OTL.h>

#include <map>

#if _MSC_VER >= 1400
#pragma warning(push)
#pragma warning(disable : 4996) // 'function' was declared deprecated 
#endif

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
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/image.h>
#include <wx/clipbrd.h>
#include <wx/valgen.h>
#include <wx/docview.h>
#include <wx/html/htmlwin.h>

#if _MSC_VER >= 1400
#pragma warning(pop)
#endif
