
// -*- C++ -*-
// Test_export.h,v 0.1 2005/07/11 14:25:31 rtaylor
// Definition for Win32 Export directives.
// This file is generated by hand
// ------------------------------
#ifndef TEST_EXPORT_H
#define TEST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TEST_HAS_DLL)
#  define TEST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TEST_HAS_DLL */

#if !defined (TEST_HAS_DLL)
#  define TEST_HAS_DLL 1
#endif /* ! TEST_HAS_DLL */

#if defined (TEST_HAS_DLL) && (TEST_HAS_DLL == 1)
#  if defined (TEST_BUILD_DLL)
#    define Test_Export ACE_Proper_Export_Flag
#    define TEST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TEST_BUILD_DLL */
#    define Test_Export ACE_Proper_Import_Flag
#    define TEST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TEST_BUILD_DLL */
#else /* TEST_HAS_DLL == 1 */
#  define Test_Export
#  define TEST_SINGLETON_DECLARATION(T)
#  define TEST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TEST_HAS_DLL == 1 */

// Set TEST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TEST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TEST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TEST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TEST_NTRACE */

#if (TEST_NTRACE == 1)
#  define TEST_TRACE(X)
#else /* (TEST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TEST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TEST_NTRACE == 1) */

#endif /* TEST_EXPORT_H */

// End of auto generated file.