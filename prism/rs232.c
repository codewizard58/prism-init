/* rs232.c */
#define CHARWAIT 10

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "proto.h"
#include "defs.h"

#include "misc.h"
#include "fd_set.h"

static int rs232started;

#ifndef NO232COMS
#include "coms232.h"
/* Basic Console IO functions called via the netdev table */
static int rs232devindex= -1;

/* init just clear the started flag */
int rs232_init(n)
int n;
{
	rs232devindex= n;
	rs232started= 0;
	serini();
}

int rs232_open(fd, net)
int fd;
struct netinfo *net;
{
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
			if(chval()){
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
		while(serput(ch)== -1);
	}
	return cnt2;
}

int rs232_check()
{
	if(rs232started)return 0;

	net_new_con(rs232devindex, 1);
	rs232started = 1;
	return 1;
}

int rs232_sel( in, out)
int *in, *out;
{	int n;
	struct netinfo *net2;
	for(n = 0; n <NCONNECTIONS; n++){
		net2 = net_connections[n];
		if(net2 && net2->type == rs232devindex){
			if( chaval())
				continue;
		}else {
			if(out)
				*out&= ~(1 << n);
		}
		if(in)
			*in &= ~(1 << n);
	}
	return 0;
}

#endif
