#ifndef OOCORE_VERSION_H_INCLUDED_
#define OOCORE_VERSION_H_INCLUDED_

//////////////////////////////////////////////
// Version defines

#define OOCORE_MAJOR_VERSION  0
#define OOCORE_MINOR_VERSION  5
#define OOCORE_PATCH_VERSION  0

#define OOCORE_VERSION_III(n)        #n
#define OOCORE_VERSION_II(a,b,c)     OOCORE_VERSION_III(a.b.c)
#define OOCORE_VERSION_I(a,b,c)      OOCORE_VERSION_II(a,b,c)
#define OOCORE_VERSION               OOCORE_VERSION_I(OOCORE_MAJOR_VERSION,OOCORE_MINOR_VERSION,OOCORE_PATCH_VERSION)

#endif // OOCORE_VERSION_H_INCLUDED_
