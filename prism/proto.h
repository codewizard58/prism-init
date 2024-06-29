/* Allow function prototyping to be turned on and off */

#ifdef NO_PROTOS
#define PROTO(x) 
#define PROTO2(x,y)
#define PROTO3(x,y,z)
#define PROTO4(x,y,z,a)
#define PROTO5(x,y,z,a,b)
#else
#define PROTO(x) x
#define PROTO2(x,y) x,y
#define PROTO3(x,y,z) x,y,z
#define PROTO4(x,y,z,a) x,y,z,a
#define PROTO5(x,y,z,a,b) x,y,z,a,b
#endif /* NO_PROTOS */

#ifndef __PROTO
#ifndef NO_PROTOS
#define __PROTO(x) x
#else
#define __PROTO(x) ()
#endif
#endif

#ifdef NO_CONST
#ifndef const
#define const /* */
#endif /* const */
#endif /* NO_CONST */




