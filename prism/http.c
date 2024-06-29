/* http.c
 * By P.J.Churchyard
 * Copyright (c) 1995
 * All rights reserved.
 */

#include "copyrite.h"

#include <stdio.h>

#ifndef MSDOS
#include <sys/types.h>

#ifdef sun
#include <sys/time.h>
#endif
#ifdef linux
#include <sys/time.h>
#include <unistd.h>
#endif

#endif

#ifdef MSDOS

#include "defs.h"
#include "net.h"
#include <fcntl.h>

#else
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>

#define _O_BINARY 0
#endif

void sorry_user(), sorry2_user();

#include "mud_set.h"

#include <ctype.h>

#ifndef MSDOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


#include "db.h"
#include "interfac.h"
#include "config.h"

#include "os.h"
#include "bsx.h"

#ifdef OUTPUT_WRAP
#define OUTPUT_WIDTH 76
#endif

#define INPUT_EDIT

#ifdef MSDOS
struct timezone {
char * zone;
};

#endif

#ifndef MSC
#ifndef MSDOS
extern int      errno;
#endif
#endif

#include "descript.h"

extern int httpsock;
extern int ndescriptors;

void process_commands(PROTO(void));
int shove_sub();
void shovechars();
void shutdownsock(PROTO(struct descriptor_data *));
struct descriptor_data *initializesock(PROTO(int) );
void make_nonblocking(PROTO(int ));
void freeqs( PROTO(struct descriptor_data *));
void welcome_user(PROTO(struct descriptor_data *));
void check_connect(PROTO2(struct descriptor_data *, const char *));
void close_sockets();
const char *addrout (PROTO(long));
void dump_users(PROTO(struct descriptor_data *));
void set_signals(PROTO(void));
struct descriptor_data *new_connection(PROTO(int) );
void parse_connect (PROTO5(const char *, char *, char *, char *, struct descriptor_data *));
void set_userstring (PROTO2(char **, const char *));
int do_command (PROTO2(struct descriptor_data *, char *));
char *strsave (PROTO(const char *));
int make_socket(PROTO(int));
int queue_string(PROTO2(struct descriptor_data *, const char *));
int queue_write(PROTO3(struct descriptor_data *, const char *, int));
int process_output(PROTO(struct descriptor_data *));
int process_input(PROTO(struct descriptor_data *));
#ifndef MSDOS
void bailout(PROTO3(int, int, struct sigcontext *));
#endif

extern int quiet;               /* turn off notifies */
extern FILE *logfile;

extern int avail_descriptors;
extern int maxd;
/* pjc 12/12/93 */

#ifdef MSDOS
#ifndef _WINDOWS
struct timeval {
long    tv_sec,tv_usec;
};
#endif
#endif

#ifdef _WINDOWS
#define strncasecmp strnicmp
#endif

/* Networking stuff */
extern int tcp_port;

#ifndef EINTR
#define EINTR (-1)
#endif

#ifndef EMFILE
#define EMFILE (-1)
#endif


/* start of main part of this module */

extern char home_dir[];
extern char media_dir[];

struct fd_rec {
struct fd_rec *next;
char fname[16];
char fdir[256];
int refcnt;
};

struct fd_cache {
struct fd_cache *next;
struct fd_rec *fdr;
long curpos;
int fd;
};

struct http_rec {
struct http_rec *next;
int instate;
int incnt;
SOCKET sock;
int cnt, pos;
char inbuf[16];
char curdir[256];
char outbuf[8912];
int	outcnt, outpos;
struct fd_cache *fdc;
};


struct fd_rec *fdr_head;
struct fd_cache *fdc_head;


struct fd_cache *fdc_open(fn, dir)
char *fn, *dir;
{	struct fd_cache *f = (struct fd_cache *)malloc( sizeof(struct fd_cache));

    if( f == NULL)return NULL;
    
    if( media_dir[0] == '\0')
    	strcpy(media_dir, home_dir);
   	chdir(media_dir);
    	
	if( dir && dir[0]){
		chdir(dir);
	}

    f->fdr = NULL;
    f->next= NULL;
    f->curpos = 0l;
    f->fd = open(fn,_O_BINARY);
    if( f->fd < 0){
fprintf(logfile,"Failed to open '%s'/'%s'/'%s'\n", media_dir, dir, fn);
    	free(f);
    	return NULL;
    }
	
	return f;
}

int fdc_close( fdc)
struct fd_cache *fdc;
{
	close(fdc->fd);
	free(fdc);
	
	return 0;
}

int fdc_read(fdc, buf, cnt)
struct fd_cache *fdc;
char *buf;
int cnt;
{
	return read(fdc->fd, buf, cnt);
}


struct http_rec *http_head;

struct http_rec *free_http( hr)
struct http_rec *hr;
{	struct http_rec *hrn;

	if( hr == http_head){
		http_head = hr->next;
		free(hr);
		hr = http_head;
		return hr;
	}
/* find previous one and unlink it */
	hrn = http_head;
	while(hrn != NULL && hrn->next != hr)hrn = hrn->next;
	if( hrn ){
		hrn->next = hr->next;
		free(hr);	
		return hrn->next;
	}else{
		free(hr);	/* should not get here... */
	}
	return NULL;
}

int http_safefn(buf)
char *buf;
{	char *p;
	int cnt;
	
	p = buf;
	for(cnt=0; cnt < 12 && *p; cnt++)p++;
	*p = '\0';
	if( p != buf){
		do {
			p--;
			if( *p == '.'){
				if( p[1] && p[2] && p[3]){	/* three chars after here */
					p[4] = '\0';	/* only support 3 char extensions */
				}
				break;
			}
		}while( p != buf);
	}
	if( p != buf){
		do {
			p--;
			if( *p == '.')
				*p = '0';
		}while( p != buf);
	}
	return 0;
}	


int do_http(inset, outset)
mud_set *inset, *outset;
{   struct http_rec *hr = http_head, *hrn;
	int ch;
	char *p;
	int cnt, cnt2;

	if( hr == NULL)
		return 0;

	while(hr){
		if( hr->instate == -1){ /* doing output */
			if( hr->outcnt == 0){
				hr->outcnt = fdc_read(hr->fdc, hr->outbuf, 8192);
				hr->outpos = 0;
				if( hr->outcnt <= 0){
					fdc_close(hr->fdc);
					hr->fdc = NULL;
					if( hr->inbuf[0] == 'h'){
						hr->inbuf[0] = 'a';
						hr->fdc = fdc_open(hr->inbuf, hr->curdir);
					}
					hr->outcnt = 0;
					hr->outpos = 0;

					if( hr->fdc == NULL){
						mud_close(hr->sock);
						hr = free_http(hr);
						if( hr == NULL)
							break;
						continue;
					}					
				}
			}
			if( MUD_ISSET(hr->sock, outset)){
				cnt = hr->outcnt - hr->outpos;
				if( cnt > 0){
					cnt2 = mud_write(hr->sock, &hr->outbuf[hr->outpos], cnt);
					if( cnt2 > 0){
						hr->outpos += cnt2;
						if( hr->outpos >= hr->outcnt){
							hr->outcnt = 0;
							hr->outpos = 0;
						}
					}else {
						if( errno != EWOULDBLOCK){
							hr->outcnt = 0;
							hr->outpos = 0;
							mud_close(hr->sock);
							hr = free_http(hr);
							if( hr == NULL)
								break;
							continue;
						}
					}
				}else {
					hr->outcnt = 0;
					hr->outpos = 0;
				}
			}
		}else{
			if( hr->cnt == 0){
				hr->pos = 0;
				if( MUD_ISSET(hr->sock, inset)){
					hr->cnt = mud_read(hr->sock, hr->outbuf, 8192);
					if( hr->cnt <= 0){
						mud_close(hr->sock);
						hr = free_http(hr);
						if( hr == NULL)
							break;
						continue;
					}
				}
			}
			while(hr->pos < hr->cnt){
				switch(hr->instate){
				case 0:
					hr->inbuf[0] = 'g';
					hr->incnt = 0;
					hr->instate = 1;
				case 1:
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					hr->inbuf[hr->incnt] = ch;
					if( ch == ' ')hr->instate = 3;
					else if( ch == '/' || ch == '\\')hr->instate = 4;
					else if( hr->incnt >= 12)hr->instate = 5;
					else if( ch == '\r' )hr->instate = 7;	/* wait for \n */
					else if( ch == '\n' )hr->instate = 8;
					break;
				case 3:	/* check for method */
					if( hr->incnt != 4){
						hr->instate = 5;
						break;
					}
					if( strncasecmp(&hr->inbuf[1], "GET", 3)){
						hr->instate = 5;
						break;
					}
					hr->incnt = 0;
					hr->inbuf[0] = 'h';
					hr->instate = 6;    /* start looking for path */
					break;
				case 5:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n'){
						hr->instate = 2;
						break;
					}
					break;
				case 2:     /* special stop state */
					hr->pos = hr->cnt;
					break;
				case 4:		/* try to change directory */
					hr->inbuf[hr->incnt] = '\0';
					chdir(media_dir);
					ch = hr->inbuf[0];
					hr->inbuf[0] = 'd';
					http_safefn(hr->inbuf);

					p = hr->curdir;
					if( hr->curdir[0]){
						chdir(hr->curdir);
						chdir(hr->inbuf);
						for(cnt = 0; cnt < 250 && *p; cnt++)p++;
						*p++ = '\\';
					}
					strcpy(p, hr->inbuf);
					hr->incnt = 0;
					hr->inbuf[0] = ch;
					hr->instate = 6;
					break;
				case 6:
				case 13: 
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					hr->inbuf[hr->incnt] = ch;
					if( ch == ' ')hr->instate = 9;
					else if( ch == '/' || ch == '\\')hr->instate = 4;
					else if( ch == '\r')hr->instate = 7;
					else if( ch == '\n')hr->instate = 8;
#ifdef MSDOS
					else if( hr->instate == 6 && hr->incnt >= 4){/* axxxx*.yyy */
						if( ch == '.'){
							hr->instate = 13;
							break;
						}
						if( hr->incnt >= 8)
							hr->incnt--;
					}else if( hr->incnt >= 12)hr->incnt--;
#else
					else if( hr->incnt >= 12)hr->instate = 5;
#endif
					break;
				case 7:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n')hr->instate = 8;
					break;
				case 9:	/* look for \r\n\r\n */
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\r')hr->instate = 10;
					break;
				case 10:					
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n')hr->instate = 11;
					else if( ch != '\r')hr->instate = 9;
					break;
				case 11:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch != '\r')hr->instate = 9;
					else hr->instate = 12;
					break;
				case 12:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch != '\n'){
						hr->instate = 9;
						break;
					}
	/* fall through */
				case 8:	/* end of request */
					hr->inbuf[hr->incnt] = '\0';
					http_safefn(hr->inbuf);
					hr->instate = -1;	/* end input */
					hr->fdc = fdc_open(hr->inbuf, hr->curdir);
					hr->pos = hr->incnt;
					if( hr->fdc == NULL){
						hr->inbuf[0] = 'a';
						hr->fdc = fdc_open(hr->inbuf, hr->curdir);
						if( hr->fdc == NULL)
							hr->instate = 2;
					}
					break;
				}
				if( hr->instate == 2 || hr->instate == -1)
					break;		
			}/* end while */
			if( hr->instate == 2){
				mud_close(hr->sock);
				hr = free_http(hr);
				if( hr == NULL)
					break;
				continue;
			}
			if( hr->pos >= hr->cnt)
				hr->cnt = 0;
		}
	
		hr = hr->next;
	}
	return 0;
}

int http_setfd(inset, outset)
mud_set *inset, *outset;
{	struct http_rec *hr = http_head;

	if( hr == NULL)
		return 0;
	while(hr){
		if( hr->instate == -1){	/* done input */
			MUD_SET(hr->sock, outset);
		}else{
			MUD_SET(hr->sock, inset);
		}
		hr = hr->next;
	}
	return 0;
}

int new_http(sock)
SOCKET sock;
{	struct http_rec *hr = (struct http_rec *)malloc( sizeof(struct http_rec));
	SOCKET nc;
	
	nc = mud_accept(sock);
	if( nc == INVALID_SOCKET)
		return 1;
		
	if( hr == NULL)
		return 1;
	hr->next = http_head;
	http_head= hr;
	hr->instate = 0;
	hr->outcnt = 0;
	hr->sock = nc;
	hr->incnt= -1;
	hr->fdc = NULL;
	hr->cnt = 0;
	hr->pos = 0;
	hr->inbuf[0] = '\0';
	hr->curdir[0] = '\0';		
	return 0;
}

