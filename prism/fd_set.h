/* fd_set.c
 */
#ifndef NFDBITS
#define NFDBITS 32
#endif

#ifndef FD_SET

#ifndef MSDOS

#ifndef FD_SET

typedef long fd_set;
#define FD_SET(n, p)    ((*p) |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((*p) &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((*p) & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif  FD_SET

#else
typedef int fd_set;

#define FD_ZERO(x) (*(x)) = 0;
#define FD_SET( d,x) (*(x)) |= (1 << (d%16))
#define FD_ISSET( d, x) (*(x)) & (1 << (d%16))
#endif

#endif

