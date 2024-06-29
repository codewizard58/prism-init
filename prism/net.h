/* net.h */

#ifndef __NET_H
#define __NET_H

#ifdef WINSOCK

#include <winsock.h>

#else

#ifdef MSDOS
#ifndef NONET
#include <sys/tk_types.h>
#endif
#endif

#endif

#ifndef INVALID_SOCKET
#define SOCKET int
#define INVALID_SOCKET (-1)

#endif

extern int nocom1;

#endif



