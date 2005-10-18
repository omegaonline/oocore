
// -*- C++ -*-
// OOSvc_export.h,v 0.1 2005/07/11 14:25:31 rtaylor
// Definition for Win32 Export directives.
// This file is generated by hand
// ------------------------------
#ifndef OOSVC_EXPORT_H
#define OOSVC_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OOSVC_HAS_DLL)
#  define OOSVC_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OOSVC_HAS_DLL */

#if !defined (OOSVC_HAS_DLL)
#  define OOSVC_HAS_DLL 1
#endif /* ! OOSVC_HAS_DLL */

#if defined (OOSVC_HAS_DLL) && (OOSVC_HAS_DLL == 1)
#  if defined (OOSVC_BUILD_DLL)
#    define OOSvc_Export ACE_Proper_Export_Flag
#    define OOSVC_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OOSVC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OOSVC_BUILD_DLL */
#    define OOSvc_Export ACE_Proper_Import_Flag
#    define OOSVC_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OOSVC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OOSVC_BUILD_DLL */
#else /* OOSVC_HAS_DLL == 1 */
#  define OOSvc_Export
#  define OOSVC_SINGLETON_DECLARATION(T)
#  define OOSVC_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OOSVC_HAS_DLL == 1 */

// Set OOSVC_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OOSVC_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OOSVC_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OOSVC_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OOSVC_NTRACE */

#if (OOSVC_NTRACE == 1)
#  define OOSVC_TRACE(X)
#else /* (OOSVC_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OOSVC_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OOSVC_NTRACE == 1) */

#endif /* OOSVC_EXPORT_H */

// End of auto generated file.
