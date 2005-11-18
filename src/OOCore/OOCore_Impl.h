//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OOCORE_IMPL_H_INCLUDED_
#define OOCORE_OOCORE_IMPL_H_INCLUDED_

// This is a shoddy fixup for compilers with broken explicit template specialisation
#if (__GNUC__) && (__GNUC__ <= 3)
	#define EXPLICIT_TEMPLATE(m,t)	template m<t>
#else
	#define EXPLICIT_TEMPLATE(m,t)	m<t>
#endif

#endif // OOCORE_OOCORE_IMPL_H_INCLUDED_
