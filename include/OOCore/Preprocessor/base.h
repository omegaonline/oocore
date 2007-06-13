// Preprocessor/base.h
//
// The basic macros
//

#if !defined(OMEGA_MAX_DEFINES)
#define OMEGA_MAX_DEFINES	256
#endif

#if !defined(OMEGA_CONCAT)
#define OMEGA_CONCAT(a,b)		OMEGA_CONCAT_I(a,b)
#define OMEGA_CONCAT_R(a,b)		OMEGA_CONCAT_R_I(a,b)
#ifdef _MSC_VER
#define OMEGA_CONCAT_I(a,b)		OMEGA_CONCAT_II(a ## b)
#define OMEGA_CONCAT_II(p0)		p0
#define OMEGA_CONCAT_R_I(a,b)	OMEGA_CONCAT_R_II(a ## b)
#define OMEGA_CONCAT_R_II(p0)	p0
#else
#define OMEGA_CONCAT_I(a,b)		a ## b
#define OMEGA_CONCAT_R_I(a,b)	a ## b
#endif
#endif 

#if !defined(OMEGA_STRINGIZE)
#define OMEGA_STRINGIZE_I(n)	#n
#define OMEGA_STRINGIZE(n)		OMEGA_STRINGIZE_I(n)
#endif
