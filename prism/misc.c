/* Misc.c
 * By P.J.Churchyard 
 * Copywrite 1990,1991,1992 (c)
 * All rights reserved
 *
 * This file is used to interface a mixture of DOS stuff to
 * what would be the UNIX style IO functions.
 * 
 * Use netinfo structures to allow a mapping from handle to
 * connection specific info.
 * 
 * Index a table by the netinfo type field to get to comms functions.
 */

#define CHARWAIT 10

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#ifndef MSDOS
#include <sys/time.h>
#ifdef linux
#include <unistd.h>
#endif
#endif

#include "proto.h"

#ifdef MSDOS
#include "defs.h"
#ifndef NONET
#include <sys/tk_types.h>
#endif
#endif

#include "misc.h"
#include "mud_set.h"

#include "bsx.h"

#ifndef NO232COMS
#include "coms232.h"

#endif

#ifdef _WINDOWS
#include "mudcom.h"
#include <winsock.h>
#endif

#ifdef TCPIP
#include "tcpcoms.h"
#endif

extern char * Lastcommand;
extern FILE *logfile;

#ifdef MSDOS

#ifdef CONSOLE
extern int con_init(int);
extern int con_check();
extern int con_open(PROTO2(int, struct netinfo *));
extern int con_close(PROTO2(int, struct netinfo *));
extern int con_read(PROTO4(int, struct netinfo *, char *, int));
extern int con_write(PROTO4(int, struct netinfo *, char *, int));
extern int con_sel(PROTO2( int *, int *));
extern char *con_name(PROTO2( int, struct netinfo *));  /* Name of device */
#endif  /* CONSOLE */

#endif  /* MSDOS */

struct netdev netdevtable[] = {
#ifdef CONSOLE
	{ con_check, con_init, con_open, con_close, con_read, con_write, con_sel, NULL, con_name, NULL},
#endif
#ifdef _WINDOWS
	WINDOWS_DEVICES
#endif
#ifdef MSDOS
#ifndef _WINDOWS
#ifndef NO232COMS
	{ rs232_check, rs232_init, rs232_open, rs232_close, rs232_read, rs232_write, rs232_select ,rs232_shut, rs232_name, NULL},
#endif
#endif
#endif
#ifdef TCPIP
	TCPIP_DEVICES
#endif
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL} /* The Null entry */
};



struct netinfo *net_head;       /* Queue of new connections */
struct netinfo *net_connections[NCONNECTIONS];



int netdevsize()
{       static int size = -1;
	struct netdev *n = netdevtable;

	if( size >= 0)
		return size;
	size = 0;
	while(n->chknew){
		size++;
		n++;
	}
	if( size == 0){
fprintf(logfile,"netdevsize: NO devices configured!\n");
		exit(1);
		return size;
	}
	return size;
}

/* Initialise all the net devices */
int netdevinit()
{       int res = 0;
	struct netdev *n = netdevtable;
	int cnt = netdevsize(), i;

	for(i= 0; i < cnt; i++){
		if( netdevtable[i].init)
			res |= (*netdevtable[i].init)(i);
	}
	return res;
}

int netdevshut()
{       int res = 0;
	struct netdev *n = netdevtable;
	int cnt = netdevsize(), i;
	struct netinfo *net;

	/* first close all the connections */

	for(i=0; i < NCONNECTIONS; i++){
		if( (net = net_connections[i]) &&
			net->type >= 0 && net->type < cnt &&
			netdevtable[net->type].close){
			(*netdevtable[net->type].close)(i, net);
		}
	}


	for(i= 0; i < cnt; i++){
		if( netdevtable[i].term)
			res |= (*netdevtable[i].term)();
	}

	return res;
}


/* Create a new connection block and put it in the new queue */
int net_new_con(t, data)
int t, data;
{       struct netinfo *n;

	n = (struct netinfo *) malloc(sizeof(struct netinfo));
	if( !n)return 0;        /* No memory */
	n->type = t;
	n->data = data;
	n->next = net_head;
	net_head= n;
	return 1;
}

/* NULL device routines */

int no_check(){ return 0;}

int no_init(n)
int n;
{ return -1;}

int no_read(fd,net,buf,cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{
	return -1;
}

int no_write(fd, net,buf,cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{
	return -1;
}

int no_open(fd, net)
int fd;
struct netinfo *net;
{
	return -1;
}

int no_close(fd, net)
int fd;
struct netinfo *net;
{
	return -1;
}


int no_sel( in,out)
int *in, *out;
{
	*in = 0;
	*out= 0;
	return -1;
}

#ifdef CONSOLE
/* Basic Console IO functions called via the netdev table */
static int constarted;
static int condevindex= -1;

/* init just clear the started flag */
int con_init(n)
int n;
{
	condevindex= n;
	constarted= 0;
}

int con_open(fd, net)
int fd;
struct netinfo *net;
{
	return net->data;
}

int con_close(fd, net)
int fd;
struct netinfo *net;
{
	constarted = 0;
	return 0;
}


int con_read(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{       int res = 0;
	int delay;

	delay = CHARWAIT;

	while(delay--){
		if(cnt > 0){
			if(kbhit()){
				*buf = getch();
				mud_echo(*buf, fd);
				buf++;
				delay = CHARWAIT;
				cnt--;
				res++;
			}
		}else
			break;
	}
	return res;
}

int con_write(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{
	return write(net->data, buf, cnt);
}

int con_check()
{
	if(constarted)return 0;

	net_new_con(condevindex, 1);
	constarted = 1;
	return 1;
}

int con_sel( in, out)
int *in, *out;
{       int n;
	struct netinfo *net2;
	for(n = 1; n <NCONNECTIONS; n++){
		net2 = net_connections[n];
		if(net2 && net2->type == condevindex){
			if( kbhit())
				continue;
		}else {
			if(out){
				MUD_CLR(n, out);
		}
		if(in){
			MUD_CLR(n, in);
	}
	return 0;
}

/* return the name of the device */
char *con_name(fd, net)
int fd;
struct netinfo *net;
{
	return "CON:";
}

#endif /*CONSOLE*/

int mud_write(fd, buf, cnt)
int fd;
char *buf;
int cnt;
{       int res = cnt;
	struct netinfo *net;
	if( fd > 0 && fd < NCONNECTIONS){
		net = net_connections[fd];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return -1;      /* Error not in use */

	if( net->type >= 0 && net->type < netdevsize()){
		res = (netdevtable[net->type].write)(fd, net, buf, cnt);
	}else {
		return -1;
	}
	return res;
}

#ifdef MSDOS
/* MSDOS does not have cooked/raw input stuff so emulate it*/
/* Used by rs232 and console IO */

static int noechoflags[16]={
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};
static int linecnt[16] = { 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};

noecho( fd)
int fd;
{
	noechoflags[fd] = 1;
}

echo( fd)
int fd;
{
	noechoflags[fd] = 0;
}


mud_echo(ch, fd)
char ch;
int fd;
{       int *p = &linecnt[fd];

	if ( ch >= ' ' && ch != '\377'){
		if( noechoflags[fd] == 0)
			mud_write(fd, &ch, 1);
		*p += 1;
	} else { /* Non printing */
		switch(ch){
		case '\377':
		case '\010':
			if( *p ){
				*p -= 1;
				if( noechoflags[fd] == 0)
					mud_write(fd,"\010 \010", 3);
			}
			break;
		case '\030':    /* ^X */
			if( *p ){
				*p = 0;
				mud_write(fd,"^X\n", 3);
			}
			break;
		case '\022':    /* ^R */
			if( *p ){
				mud_write(fd,"^R\n", 3);
			}
			break;
		case '\n':
		case '\015':
			*p = 0;
			mud_write(fd, "\n", 1);
			break;
		}
	}
}
#endif /* MSDOS */

mud_read(fd, buf, cnt)
int fd;
char *buf;
int cnt;
{       int res = 0;
	int delay;
	char ch;
	struct netinfo *net;

	delay = CHARWAIT;

	if( fd > 0 && fd < NCONNECTIONS){
		net = net_connections[fd];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return -1;      /* Error not in use */

	if( net->type >= 0 && net->type < netdevsize()){
		res = (netdevtable[net->type].read)(fd, net, buf, cnt);
	}else {
		return -1;
	}
	return res;
}

mud_close(fd)
int fd;
{
	struct netinfo *net;
	int res;

	if( fd > 0 && fd < NCONNECTIONS){
		net = net_connections[fd];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return -1;      /* Error not in use */
	res = 0;
	if( net->type >= 0 && net->type < netdevsize() &&
		netdevtable[net->type].close){
		res = (netdevtable[net->type].close)(fd, net);
	}
	net_connections[fd] = NULL;
	free(net);

	return res;
}

/* Note fd 0 is used as a flag by select */

mud_open()
{       int n,res;
	struct netinfo *net;
	int fd;

	net = net_head;
	if( !net)
		return -1;              /* No pending requests */

/* Find a free netdev descriptor */
	for( fd= 1; fd < NCONNECTIONS; fd++){
		if( !net_connections[fd]){
			net_connections[fd] = net;
			net_head = net->next;
			net->next= NULL;
			break;
		}
	}
	if( fd == NCONNECTIONS)
		return -1;      /* No free connections */

	if( net->type >= 0 && net->type < netdevsize()){
		res = (netdevtable[net->type].open)(fd, net);
		if( res < 0){
fprintf(logfile,"NETOPEN failed (%d, %d)\n", net->type, net->data);
			free(net);
			net_connections[fd] = NULL;
		}else {
			res = fd;
		}
	}else {
		return -1;
	}
	return res;
}

char *mud_name(fd)
int fd;
{       char *res;
	struct netinfo *net;
	static char m_name[80];

	strcpy(m_name, "Unknown");

	if( fd > 0 && fd < NCONNECTIONS){
		net = net_connections[fd];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return m_name;  /* Error not in use */

	res = m_name;
	if( net->type >= 0 && net->type < netdevsize() &&
		netdevtable[net->type].name){
		res = (netdevtable[net->type].name)(fd, net);
	}
	return res;
}


edit_input_line( s)
char *s;
{       char *p;
	int n = 0;

	p = s;

	while(*s){
		switch(*s){
		case '\377':
		case '\010':
			if(n){
				n--;
				p--;
			}
			break;
		default:
			*p++ = *s;
			n++;
			break;
		}
		s++;
	}
	*p = *s;
}

/* Bit 0 is the select flag... */

int mud_select(maxd, i, o, p, t )
int maxd;
mud_set *i, *o, *p;
char *t;
{
	int delay = 2;  /* pjc 11/2/93 */
	int n, j;
	mud_set savedi;
	mud_set savedo;
	struct netinfo *net;
	mud_set ri;
	mud_set ro;

	if(i && MUD_ISSET(0, i)){
		for(n = 0; n < netdevsize(); n++){
			if(netdevtable[n].chknew && 
				netdevtable[n].chknew() > 0){
				/* Signal that a new connection needs processing*/
				MUD_ZERO(i);
				MUD_SET(0,i);
				return 1;
			}
		}
		MUD_CLR(0,i);
	}

	MUD_ZERO(&ro);
	MUD_ZERO(&ri);
	while(delay--){
		for(n = 0; n < netdevsize(); n++){
			if( netdevtable[n].select ){
				if( i)
					savedi = *i;
				else{
					MUD_ZERO(&savedi);
				}
				if( o)
					savedo = *o;
				else{
					MUD_ZERO(&savedo);
				}
				(netdevtable[n].select)(&savedi, &savedo);

				for(j=0; j <maxd; j++){
					if( MUD_ISSET(j, &savedo)){
						MUD_SET(j, &ro);
					}
				}
				for(j=0; j <maxd; j++){
					if( MUD_ISSET(j, &savedi)){
						MUD_SET(j, &ri);
					}
				}
			}
		}
	}
	if( o)
		*o = ro;
	if( i){
		*i = ri;
		return 1;
	}

	return 0;
	
}

mud_shutdown(fd, mode)
int fd, mode;
{       struct netinfo *net;
	int res;

	if( fd > 0 && fd < NCONNECTIONS){
		net = net_connections[fd];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return -1;      /* Error not in use */
	res = 0;
	if( net->type >= 0 && net->type < netdevsize() &&
		netdevtable[net->type].shut){
		res = (netdevtable[net->type].shut)(fd, net, mode);
	}
	return res;
}


int mud_accept(sock)
int sock;
{
       struct netinfo *net;
	int res;

	if( sock > 0 && sock < NCONNECTIONS){
		net = net_connections[sock];
	}else{
		net = NULL;
	}
	if( net == NULL)
		return -1;      /* Error not in use */

	return tcp_accept(net->data);
}


#ifdef MSDOS
/* Unix routines that we don't have */

/* Simulate the UNIX gettimeofday function */
#ifndef _WINDOWS
struct timeval {
long    tv_sec,tv_usec;
};
#endif

static int xx;

gettimeofday( tv , tm)
struct timeval *tv;
char *tm;
{       long now;

	
	if(tv){
		time(&now);
		tv->tv_sec = now;
		tv->tv_usec = xx*1000l;
		xx++;
		if(xx == 1000)
			xx = 0;
	}

}

/* Just do nothing in MSDOS */

alarm(x)
int x;
{ /* null routine */
}


/* Some UNIX interface routines */
int random()
{
	return rand();
}

int bcopy(s, d, n)
char *s,*d;
int n;
{
	while(n-- > 0)*d++ = *s++;
}

getdtablesize()
{
	return NCONNECTIONS+4;  
}

bzero(s,n)
char *s;
int n;
{
	if(s)
		while(n-- > 0)
			*s++ = '\0';
}


void abort()
{
	exit(0);
}

#ifdef _WINDOWS

void perror(char *s)
{
	printf("Perror(%s)\n",s);
}

int spawnvpe(int mode, char *cmd,char **argv, char **envp)
{
	printf("Spawn(%s)\n",cmd);
	return 0;
}

int spawnlp(int mode,char *cmd,...)
{
	return 0;
}

#endif  /* _windows */


#endif  /* MSDOS */

#ifdef HPUX

int random()
{
	return rand();
}

getdtablesize()
{
	return 28;
}

#endif

