///////////////////////////////////////////////////////////////////////////////////
//
// This code is based on code by the Boost guys, 
// and is therefore a derivative work, covered by their license
//
///////////////////////////////////////////////////////////////////////////////////
//
// Boost Software License - Version 1.0 - August 17th, 2003
// 
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////////

// Preprocessor/base.h
//
// The basic macros
//

#if !defined(OMEGA_MAX_DEFINES)
#define OMEGA_MAX_DEFINES	256
#endif

#if !defined(OMEGA_CONCAT)
#define OMEGA_CONCAT(a,b)       OMEGA_CONCAT_I(a,b)
#define OMEGA_CONCAT_R(a,b)     OMEGA_CONCAT_R_I(a,b)
#define OMEGA_CONCAT_R2(a,b)    OMEGA_CONCAT_R2_I(a,b)
#ifdef _MSC_VER
#define OMEGA_CONCAT_I(a,b)     OMEGA_CONCAT_II(a ## b)
#define OMEGA_CONCAT_II(p0)     p0
#define OMEGA_CONCAT_R_I(a,b)   OMEGA_CONCAT_R_II(a ## b)
#define OMEGA_CONCAT_R_II(p0)   p0
#define OMEGA_CONCAT_R2_I(a,b)  OMEGA_CONCAT_R2_II(a ## b)
#define OMEGA_CONCAT_R2_II(p0)  p0
#else
#define OMEGA_CONCAT_I(a,b)     a ## b
#define OMEGA_CONCAT_R_I(a,b)   a ## b
#define OMEGA_CONCAT_R2_I(a,b)  a ## b
#endif
#endif 

#if !defined(OMEGA_STRINGIZE)
#define OMEGA_STRINGIZE_I(n)    #n
#define OMEGA_STRINGIZE(n)      OMEGA_STRINGIZE_I(n)
#endif

#if !defined(OMEGA_WIDEN_STRING)
#define OMEGA_WIDEN_STRING_II(STRING) L ## STRING
#define OMEGA_WIDEN_STRING_I(STRING)  OMEGA_WIDEN_STRING_II(STRING)
#define OMEGA_WIDEN_STRING(STRING)    OMEGA_WIDEN_STRING_I(STRING)
#endif
