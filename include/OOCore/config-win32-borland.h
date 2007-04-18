#ifndef OOCORE_CONFIG_BORLAND_H_INCLUDED_
#define OOCORE_CONFIG_BORLAND_H_INCLUDED_

#define OMEGA_MAX_DEFINES     124

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new (std::nothrow) CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#error Unfortunately there is a major issue with Bcc and templates!

// See http://www.boost.org/more/borland_cpp.html 
// section "Deduction of constant arguments to function templates"
// for further information
// You're welcome to do a work-around!

#endif // OOCORE_CONFIG_BORLAND_H_INCLUDED_
