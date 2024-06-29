/* fd_set.c
 */
#ifndef NFDBITS
#define NFDBITS 32
#endif

#ifndef MUD_SET

#ifndef MSDOS
#ifndef FD_SET
#ifdef __FD_SET
#define FD_SET __FD_SET
#endif
#endif

#ifndef FD_SET

typedef long fd_set;

#define FD_SET(n, p)    ((*p) |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((*p) &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((*p) & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))

#endif  /* FD_SET */

#define MUD_SET FD_SET
#define MUD_CLR FD_CLR
#define MUD_ZERO FD_ZERO
#define MUD_ISSET FD_ISSET


typedef fd_set mud_set;

#else


typedef int mud_set;

#define MUD_ZERO(x) (*(x)) = 0;
#define MUD_SET( d,x) (*(x)) |= (1 << (d%16))
#define MUD_CLR(d,x ) (*(x)) &= ~(1 << (d%16))
#define MUD_ISSET( d, x) (*(x)) & (1 << (d%16))

#endif	/* MSDOS */


#endif

