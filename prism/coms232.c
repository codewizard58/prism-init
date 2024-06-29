/* Start the RS232 stuff */
/* rs232.c */
#define CHARWAIT 10

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "proto.h"
#include "defs.h"

#include "misc.h"
#ifndef FD_SET
#include "fd_set.h"
#endif

static int rs232started;
extern char *Lastcommand;
extern FILE *logfile;

#ifndef NO232COMS
#include "coms232.h"
/* Basic Console IO functions called via the netdev table */
static int rs232devindex= -1;
extern int nocom1;

extern void serrst();

/* On break handler */
cntrlc()
{
	fprintf(stderr,"\nAborted\n");
	if(Lastcommand)
		fprintf(stderr,"Last_command= '%s'\n", Lastcommand);
	netdevshut();
	exit(1);
}


/* init just clear the started flag */
int rs232_init(n)
int n;
{
	if(nocom1)
		return 0;
	rs232devindex= n;
	rs232started= 0;
	serini();
	signal(SIGINT, cntrlc);
}

int rs232_shut(fd, net)
int fd;
struct netinfo *net;
{
	if(nocom1)
		return 0;
	if( !net){
		serrst();
	}
	return 0;
}

int rs232_open(fd, net)
int fd;
struct netinfo *net;
{
	if( nocom1)
		return -1;
	return net->data;
}

int rs232_close(fd, net)
int fd;
struct netinfo *net;
{
	rs232started = 0;
	return 0;
}


int rs232_read(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{	int res = 0;
	int delay;

	delay = CHARWAIT;

	while(delay--){
		if(cnt > 0){
			if(chaval()){
				*buf = serget();
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

int rs232_write(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{	int cnt2=cnt, ch;

	while(cnt > 0){
		cnt--;
		ch = *buf++;
		if( ch == '\n')
			while(serput('\r') == -1);
		while(serput(ch)== -1);
	}
	return cnt2;
}

int rs232_check()
{
	if(rs232started || nocom1)
		return 0;

	net_new_con(rs232devindex, 1);
	rs232started = 1;
	return 1;
}

int rs232_select( in, out)
int *in, *out;
{	int n;
	struct netinfo *net2;

	for(n = 0; n <NCONNECTIONS; n++){
		net2 = net_connections[n];
		if(nocom1 == 0 && net2 && net2->type == rs232devindex){
			if( chaval())
				continue;
		}else {
			if(out){
				*out&= ~(1 << n);
			}
		}
		if(in){
			*in &= ~(1 << n);
		}
	}
	return 0;
}

char *rs232_name(fd, net)
int fd;
struct netinfo *net;
{
	return "COM1:";
}

#endif
