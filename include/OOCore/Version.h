#ifndef OOCORE_VERSION_H_INCLUDED_
#define OOCORE_VERSION_H_INCLUDED_

//////////////////////////////////////////////
// Version defines

#define OMEGA_MAJOR_VERSION  0
#define OMEGA_MINOR_VERSION  3
#define OMEGA_BUILD_VERSION  3

#define OMEGA_VERSION_III(n)        #n
#define OMEGA_VERSION_II(a,b,c)     OMEGA_VERSION_III(a.b.c)
#define OMEGA_VERSION_I(a,b,c)	    OMEGA_VERSION_II(a,b,c)
#define OMEGA_VERSION			    OMEGA_VERSION_I(OMEGA_MAJOR_VERSION,OMEGA_MINOR_VERSION,OMEGA_BUILD_VERSION)

#endif // OOCORE_VERSION_H_INCLUDED_
