/* Misc.h by P.J.Churchyard */

#ifndef _MISC_
#define _MISC_

#ifndef NCONNECTIONS
#ifdef MSDOS
#ifndef _WINDOWS
#define NCONNECTIONS 4
#else
#define NCONNECTIONS 16
#endif
#else
#define NCONNECTIONS 32
#endif
#endif

struct netinfo{
struct netinfo *next;
int	type;		/* Type of network connection (index into netdevtable) */
int	data;		/* Used by type field to ident the connection */
};

struct netdev{
int (*chknew)();
int (*init)(PROTO(int));
int (*open)(PROTO2(int,struct netinfo *));
int (*close)(PROTO2(int, struct netinfo *));
int (*read)(PROTO4(int, struct netinfo *, char *, int));
int (*write)(PROTO4(int, struct netinfo *, char *, int));
int (*select)(PROTO2( int *, int *));
int (*shut)(PROTO(PROTO2( int, struct netinfo *)));
char * (*name)(PROTO2(int, struct netinfo *));
int (*term)();
};

extern int no_check();
extern int no_init(PROTO(int));
extern int no_open(PROTO2(int, struct netinfo *));
extern int no_close(PROTO2(int, struct netinfo *));
extern int no_read(PROTO4(int, struct netinfo *, char *, int));
extern int no_write(PROTO4(int, struct netinfo *, char *, int));
extern int no_sel(PROTO2(int *, int *));

#define NET_NONE { no_check, no_init, no_open, no_close,no_read, no_write, no_sel, NULL, NULL, NULL}

extern int net_new_con(PROTO2(int,int));
extern struct netinfo *net_connections[];

int mud_echo(PROTO2(int, int));
int mud_open();
int mud_read(PROTO3(int, char *, int));
int mud_write(PROTO3(int, char *, int));
char * mud_name(PROTO(int));


#endif /* _MISC_ */


