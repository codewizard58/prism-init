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
#include <netinet/tcp.h>
#include <netdb.h>
#endif


#include "db.h"
#include "interfac.h"
#include "config.h"
#include "externs.h"

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

void process_commands( void);
void shutdownsock( struct descriptor_data *);
struct descriptor_data *initializesock( int);
void freeqs( struct descriptor_data *);
void check_connect( struct descriptor_data *, const char *);
void close_sockets();
struct descriptor_data *new_connection( int );
void parse_connect ( const char *, char *, char *, char *, struct descriptor_data *);
void set_userstring ( char **, const char *);
int do_command (struct descriptor_data *, char *);
char *strsave (const char *);
int make_socket( int);
int queue_string(struct descriptor_data *, const char *);
int queue_write(struct descriptor_data *, const char *, int);
#ifndef MSDOS
void bailout(int, int, struct sigcontext *);
#endif

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
#define strcasecmp stricmp
#endif

/* Networking stuff */
extern int tcp_port;

#ifndef EINTR
#define EINTR (-1)
#endif

#ifndef EMFILE
#define EMFILE (-1)
#endif

#ifdef _WINDOWS
#undef FD_ISSET
#define FD_ISSET(a, b) (a)
#endif

extern char *Name0(dbref obj), *Name1();

#include "fight.h"
extern struct player_levels player_level_table[];

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
char inbuf[128];
char curdir[256];
char outbuf[8912];
int	outcnt, outpos;
struct fd_cache *fdc;
/* pjc 12-may-97 
 * Add html output and http playing code
 */
char	user[64];
char	command[1024];
char	authbuf[128];
struct text_queue description;
struct text_queue output;

/* 5/23 */
struct http_rec *next_d;

/* persistant connection support */
int	content_length;
char *clp;

// formatting
int divtype;
};

extern struct descriptor_data *descriptor_list;
extern int out_history(struct http_rec *hr, struct descriptor_data *d);

struct fd_rec *fdr_head;
struct fd_cache *fdc_head;


struct fd_cache *fdc_open(char *fn, char *dir)
{	struct fd_cache *f = (struct fd_cache *)malloc( sizeof(struct fd_cache));

    if( f == NULL)return NULL;
    
    if( media_dir[0] == '\0'){
    	strcpy(media_dir, home_dir);
	}
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
 fprintf(logfile,"DEBUG open %s/%s/%s\n",media_dir, dir?dir:".", fn);
	
	return f;
}

int fdc_close( struct fd_cache *fdc)
{
	close(fdc->fd);
	free(fdc);
	
	return 0;
}

int fdc_read(struct fd_cache *fdc, char *buf, int cnt)
{
	return read(fdc->fd, buf, cnt);
}


struct http_rec *http_head;

struct http_rec *free_http( struct http_rec *hr)
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

int http_safefn(char *buf)
{	char *p;
	int cnt;
	
	p = buf;
	for(cnt=0; cnt < 12 && *p; cnt++)p++;
	*p = '\0';
	while( p != buf){
		p--;
		if( *p >= 'A' && *p <= 'Z'){
			*p += 'a' - 'A';
		}else if( (*p < '0' || *p > '9') && (*p <'a' || *p >'z') && *p != '_' && *p != '.' && *p != '-'){
			*p = '0';
		}
		if( *p == '.'){
			if( p[1] && p[2] && p[3]){	/* three chars after here */
				p[4] = '\0';	/* only support 3 char extensions */
			}
			break;
		}
	}
	while( p != buf){
			p--;
			if( *p >= 'A' && *p <= 'Z'){
				*p += 'a' - 'A';
			}else if( (*p < '0' || *p > '9') && (*p <'a' || *p >'z') && *p != '_' && *p != '.' && *p != '-'){
				*p = '0';
			}
			if( *p == '.'){
				*p = '0';
			}
	}
	return 0;
}	



char http_path[8192];
char http_path2[8192];
char http_alt_room[8192];

int http_class_path( char *buf, dbref obj, int depth, char *alt)
{	int cnt;
	char *p, *q;
	char *savebuf = buf;

	if( obj >= 0){
		cnt = http_class_path(buf, Class(obj), depth+1, NULL);
		buf = &buf[cnt];
	}

	q = buf;
	*q++ = '/';
	if( alt != NULL && *alt){
		p = alt;
	}else if( obj >=0){
		p = Name0(obj);
	}else {
		return 0;
	}
	if( depth == 0){
		*q++ = 'x';
	}else {
		*q++ = 'd';
		p = Name0(obj);
	}
	while(*q = *p++)q++;

	http_safefn(&buf[2]);

	q = buf;
	while(*q){
		q++;
		cnt++;
	}
	if( depth == 0){
		*q++ = '.';
		*q++ = 'h';
		*q++ = 't';
		*q++ = 'm';
		*q = '\0';
		cnt += 4;

		fprintf(logfile,"HTTP_CLASS_PATH: %s\n", savebuf);
	}
	return cnt;
}
	

int http_zone_path( char *buf, dbref obj, int depth)
{	int cnt;
	char *p, *q;

	if( obj < 0){
		return 0;
	}
	cnt = http_zone_path(buf, Zone(obj), depth+1);
	buf = &buf[cnt];

	p = Name0(obj);
	q = buf;
	*q++ = '/';
	*q++ = 'd';
	while(*q = *p++)q++;

	http_safefn(&buf[2]);

	q = buf;
	while(*q){
		q++;
		cnt++;
	}
	if( depth == 0){
		strcpy(q, "/xdefault.htm");
		cnt += 13;
	}
	return cnt;
}
	


int http_look_exits(struct descriptor_data *d, char *buf, int cnt, dbref loc, char *args, char *pre, char *post)
{	dbref thing;
	char	*p = &buf[cnt];
	char	*q, *q1;
	
	strcpy(p,"EXITS"); cnt += 5; p = &p[5];
    DOLIST(thing, Contents(loc)) {
	if( Typeof(thing) == TYPE_EXIT && 
	    ( (Dark(thing) && !Dark(loc)) )) {
	    /* something exists!  show him everything */
	    DOLIST(thing, Contents(loc)) {
		if( Typeof(thing) == TYPE_EXIT &&
		    ((Dark(thing) && !Dark(loc)) ) ) {
			if( pre){
				q = pre;
				while(cnt < 8180 && (*p = *q++)){
					if( *p == '%' && *q == 'n'){
						q++;
						q1 = Name(thing);
						while(cnt < 8180 && *q1 && *q1 != ';'){
							if( *q1 == ' '){
								*p++ = '%';
								*p++ = '2';
								*p++ = '0';
								cnt += 3;
								q1++;
							}else{
								*p++ = *q1++;
								cnt++;
							}
						}
					}else{
						cnt++;
						p++;
					}
				}
			}
			q =  Name(thing);
			while(cnt < 8180 && (*p = *q++) && *p != ';'){
				cnt++;
				p++;
			}
			if( post){
				q = post;
				while(cnt < 8180 && (*p = *q++)){
					cnt++;
					p++;
				}
			}
		}
	    }
	    break;		/* we're done */
	}
    }
	return cnt;
}

int http_look_contents(struct descriptor_data *d, char *buf, int cnt,dbref loc,char *args,char *pre,char *post)
{	dbref thing;
	char	*p = &buf[cnt];
	char	*q, *q1;

	fprintf(logfile,"LOOK CONTENTS\n");
	strcpy(p,"CONTENTS"); cnt += 8; p= &p[8];
	
    DOLIST(thing, Contents(loc)) {
	if( Typeof(thing) != TYPE_EXIT && 
	    ( (!Dark(thing) && !Dark(loc)) )) {
	    /* something exists!  show him everything */
	    DOLIST(thing, Contents(loc)) {
		if( Typeof(thing) != TYPE_EXIT &&
		    (!(Dark(thing) && !Dark(loc)) ) ) {
			if( pre){
				q = pre;
				while(cnt < 8180 && (*p = *q++)){
					if( *p == '%' && *q == 'n'){
						q++;
						q1 = Name(thing);
						while(cnt < 8180 && *q1 && *q1 != ';'){
							if( *q1 == ' '){
								*p++ = '%';
								*p++ = '2';
								*p++ = '0';
								cnt += 3;
								q1++;
							}else{
								*p++ = *q1++;
								cnt++;
							}
						}
					}else{
						cnt++;
						p++;
					}
				}
			}
			q = Name(thing);
			while(cnt < 8180 && (*p = *q++) && *p != ';'){
				cnt++;
				p++;
			}
			if( post){
				q = post;
				while(cnt < 8180 && (*p = *q++)){
					cnt++;
					p++;
				}
			}
		}
	    }
	    break;		/* we're done */
	}
    }
	return cnt;
}

char *set_dbpath(const char *dbname, char *buf)
{	char *p, *q;
	p = dbname;
	q = buf;

	*q++ = 'd';
	*q++ = '/';
	*q++ = 'd';
	while( *q = *p++)q++;

	http_safefn(&buf[3]);
	q = buf;
	while(*q)q++;
	return q;
}

extern int http_expandfile_r(struct descriptor_data *d, struct http_rec *hr, char *buf,
	int cnt, char *filename, char *dir);

char http_inc_buf[8192];

int do_include(struct descriptor_data *d, struct http_rec *hr, char *buf, int cnt, dbref obj, char *alt)
{	char *q, *q1;
	char *ipath = NULL;

	ipath = set_dbpath(curdb->name, http_inc_buf);

	if( obj >= 0){
		switch( Typeof(obj)){
		case TYPE_ROOM:
			strcpy(ipath, "/droom");
			break;
		case TYPE_PLAYER:
			strcpy(ipath, "/dplayer");
			break;
		case TYPE_THING:
			strcpy(ipath, "/dthing");
			break;
		default:
			strcpy(ipath, "/dexit");
			break;
		}
	}else {
		// assume alt is set.
		strcpy(ipath, "/droom");
	}
	fprintf(logfile, "DO INCLUDE: %s\n", http_inc_buf);

	while(*ipath)ipath++;

	if( obj >= 0 || (alt != NULL && *alt)){
		http_class_path(ipath, obj, 0, alt);
	}

	http_expandfile_r(d, hr, NULL, cnt, http_inc_buf, NULL);

	return 0;
}

int http_expandfile(struct descriptor_data *d, struct http_rec *hr, char *buf, int cnt, char *filename, char *dir)
{	struct fd_cache *fdc;
	char abuf[2],*p, *q, *q1;
	int i, pos, state;
	char	macro[64];
	dbref room,player,thing;
	struct text_block *cur = NULL;
	char	prebuf[128], postbuf[128];
	char *ipath;

	fprintf(logfile,"EXPAND: %s\n", filename);

	if( buf == NULL){
		cur = make_text_block("", 8192, 0);
		buf = cur->buf;
		p = buf;
		cnt = 0;
	}
	player = d->player;
	room = getloc(player);
	
	fdc = fdc_open(filename, dir);
	p = &buf[cnt];
	state = 0;
	if( fdc == NULL){
		if( cur){
			free_text_block(cur);
			cur = NULL;
		}
		return -1;
	}
    
    while(fdc != NULL && cnt < 8180 ){
    	i = fdc_read(fdc, abuf, 1);
    	if( i <= 0)
    		break;
    	if( state == 0){
			if( abuf[0] == '$'){
				state = 1;
				continue;
			}
	    	*p++ = abuf[0];
    		cnt++;
    	}else if( state == 1){
    		if( abuf[0] == '('){
    			state = 2;
    			pos = -1;
    		}else {
    			state = 0;
    			*p++ = '$';
    			cnt++;
    			if( abuf[0] != '$'){
    				*p++ = abuf[0];
    				cnt++;
    			}
    		}
    	}else if(state == 2){
    		pos++;
			if( pos == 62)pos--;
    		macro[pos] = abuf[0];
    		if( abuf[0] == ')'){
    			macro[pos] = '\0';
    			state = 0;
    			q = macro;
    			while(*q && *q != '.')q++;
    			if( *q == '.'){
					*q++ = '\0';
				}

				if( strcasecmp(macro, "include")==0 ){
					if( cnt > 0){
						cur->nchars = cnt;
						cur->buf[cnt] = '\0';
						*hr->output.tail = cur;
						hr->output.tail = &cur->nxt;
						cur = NULL;
						buf = NULL;
						cnt = 0;
					}
    				q1 = q;
    				while(*q1 && *q1 != '.')q1++;
    				if( *q1 == '.')*q1++ = '\0';

					fprintf(logfile,"EX_INCLUDE: %s\n", q);
					do_include(d, hr, NULL, 0, NOTHING, q);

					if( cur == NULL){
						// ready for next part.
						cur = make_text_block("", 8192, 0);
						buf = cur->buf;
						p = buf;
						cnt = 0;
					}

					thing = NOTHING;
				}else if( strcasecmp(macro, "room")==0){
	    			thing = room;
    			}else if( strcasecmp(macro, "player")==0){
    				thing = player;
				}else if( strcasecmp(macro, "history")==0){
					if( cnt > 0){
						cur->nchars = cnt;
						cur->buf[cnt] = '\0';
						*hr->output.tail = cur;
						hr->output.tail = &cur->nxt;
						cur = NULL;
						buf = NULL;
					}
					//fprintf(logfile,"HTTP flush %d before out_history\n", cnt);

					out_history(hr, d);

					cur = make_text_block("", 8192, 0);
					buf = cur->buf;
					p = buf;
					cnt = 0;

					thing = NOTHING;
    			}else {
    				thing = NOTHING;
    			}
    			if( thing != NOTHING){
    				q1 = q;
    				while(*q1 && *q1 != '.')q1++;
    				if( *q1 == '.')*q1++ = '\0';
    				
    				if( strcasecmp(q, "desc")==0){
    					q = Desc(thing);
    				}else if( strcasecmp(q, "name")==0){
    					q = Name1(thing);
    				}else if( strcasecmp(q, "contents")==0){
    					q = NULL;
    					cnt = http_look_contents(d, buf, cnt, thing, q1, NULL, ", ");
    					p = &buf[cnt];
    				}else if( strcasecmp(q, "include")==0){
						if( cnt > 0){
							cur->nchars = cnt;
							cur->buf[cnt] = '\0';
							*hr->output.tail = cur;
							hr->output.tail = &cur->nxt;
							cur = NULL;
							buf = NULL;
							cnt = 0;
						}
						
						do_include(d, hr, NULL, 0, thing, NULL);
						if( cur == NULL){
							// ready for next part.
							cur = make_text_block("", 8192, 0);
							buf = cur->buf;
							p = buf;
							cnt = 0;
						}

						q = NULL;
					}
					if( Typeof(thing) == TYPE_PLAYER){
						if( strcasecmp(q, "pennies")==0){
							sprintf(prebuf, "%d", Pennies(thing));
							q = prebuf;
						}else if( strcasecmp(q, "chp")==0){
							sprintf(prebuf, "%d", Chp(thing));
							q = prebuf;
						}else if( strcasecmp(q, "mhp")==0){
							sprintf(prebuf, "%d", Mhp(thing));
							q = prebuf;
						}
					}
    				if( q != NULL){
    					while((*p = *q++) && cnt < 8180){
    						cnt++;
    						p++;
    					}
    				}
    			}
    		}
    	}
    }
    if( fdc != NULL){
    	fdc_close(fdc);
	}
	if( cur != NULL){
		cur->nchars = cnt;
		*hr->output.tail = cur;
		hr->output.tail = &cur->nxt;
	}
    
	return cnt;
}

// walk back up the path to find the file to output

int http_expandfile_r(struct descriptor_data *d, struct http_rec *hr, char *buf,
	int cnt, char *filename, char *dir)
{	char *q, *lastfn;
	char *stoppos;

	q = filename;
	stoppos = q;
	while(*q && *q != '/')q++;
	if( *q == '/'){
		stoppos = q;
		while(*q && *q != '/')q++;
		if( *q == '/'){
			while(*q && *q != '/')q++;
			stoppos = q;
		}
	}
	while( *q)q++;

	// backup to last seperator
	while(q != stoppos && *q != '/'){
		q--;
	}
	lastfn = &q[1];

	if( http_expandfile(d, hr, NULL, 0, filename, NULL) >= 0){
		return 0;
	}

	// backup looking for default.htm

	while( q != stoppos){
		q--;
		while(q != stoppos && *q != '/'){
			q--;
		}
		if( *q == '/'){
			strcpy(&q[1], lastfn);
			lastfn = &q[1];
		}else {
			break;
		}
		if( http_expandfile(d, hr, NULL, 0, filename, NULL) >= 0){
			return 0;
		}
	}

	return 0;
}


char *decode64(char *buf)
{
	int cnt= 0;
	int state=0;
	int	ch;
	char *p, *q;
	char *abuf = buf;

	while(*buf){
		ch = *buf++;
	    if( ch >= 'A' && ch <= 'Z')
	       	ch = ch -'A';
	    else if( ch >= 'a' && ch <= 'z')
	    	ch = ch -'a'+26;
	    else if( ch >= '0' && ch <= '9')
	    	ch = ch - '0' + 52;
	    else if( ch == '+')
	    	ch = 62;
	    else if( ch == '=')
	    	break;
	    else ch = 63;
	    
	    if( state == 0){
	    	abuf[cnt] = ch << 2;
	    	state++;
	    }else if( state == 1){
	    	abuf[cnt] |= 3&(ch >> 4);
	    	cnt++;
	    	abuf[cnt] = ch << 4;
	    	state++;
	    }else if( state == 2){
	    	abuf[cnt] |= 0xf & (ch >> 2);
	    	cnt++;
	    	abuf[cnt] = ch << 6;
	    	state++;
	    }else {
	    	abuf[cnt] |= ch & 0x3f;
	    	cnt++;
	    	state = 0;
	    }
	    if( cnt >= 60)
	    	break;
	}
	abuf[cnt] = '\0';

	return abuf;		
}

int dehex(char *buf)
{   char *p, *q, *sp;
	int ch;

	p = buf;
	q = buf;
	while(*p){
		if( *p == '%'){
			ch = -1;
			sp = p;
			p++;
			if( *p >= '0' && *p <= '9'){
				ch = (*p - '0')<<4;
			}else if( *p >= 'a' && *p <= 'f'){
				ch = (*p - 'a' + 10)<<4;
			}else if( *p >= 'A' && *p <= 'F'){
				ch = (*p - 'A' + 10)<<4;
			}else{
				p--;
			}
			if( ch != -1){
				p++;
				if( *p >= '0' && *p <= '9'){
					ch |= (*p - '0');
				}else if( *p >= 'a' && *p <= 'f'){
					ch |= (*p - 'a' + 10);
				}else if( *p >= 'A' && *p <= 'F'){
					ch |= (*p - 'A' + 10);
				}else {
					p--;
				}
			}
			if( ch == -1 || ch == '<'){
				p = sp;
				*q = *p;
			}else {
				*q = ch;
			}
			p++;
			q++;
		}else{
			*q++ = *p++;
		}
	}
	*q = '\0';
	return 0;
}

/* Attach this http request to a prism connection descriptor
 */

int attach_http(struct descriptor_data *d, struct http_rec *hr)
{	struct http_rec *cp, *hrn = NULL;

	cp = (struct http_rec *)d->http_list;
	while(cp != NULL && cp != hr)
		cp = cp->next_d;
	if( cp == NULL){
		hr->next_d = (struct http_rec *)d->http_list;
		d->http_list = hr;
	}
	return 0;
}

		
int detach_http(struct descriptor_data *d, struct http_rec *hr)
{	struct http_rec *cp, *hrn = NULL;

	cp = (struct http_rec *)d->http_list;
	if( cp == hr){
		d->http_list = hr->next_d;
	}else while(cp){
		if( cp->next_d == hr){
			cp->next_d = hr->next_d;
			break;
		}
		cp = cp->next_d;
	}
	return 0;
}

void detach_all_http(struct descriptor_data *dx)
{	struct http_rec *hr;

	while( hr = (struct http_rec *)dx->http_list){
		fprintf(logfile,"DETACHALL: detach %x from %x\n", hr, dx);
		detach_http(dx, hr);
		hr->instate = -3;	// flag the disconnect
	}

}

int http_show(struct http_rec *hr, descriptor *d)
{	dbref player, loc;
	dbref zone,classof;
	char *zn;
	char *p, *q, *dbpath=NULL;
	char *p2;

	if( d == NULL){
		return 1;
	}
	
	player = d->player;
	loc = Location( player);
	classof = Class(loc);
	zone = zoneof(loc);

	// fprintf(logfile,"HTTP SHOW: player=%d loc=%d classof=%d zone=%d\n", player, loc, classof, zone);
	// fprintf(logfile,"HTTP SHOW: media=%s, alt=%s\n", media_name(loc, NULL), http_alt_room);

	// first see if room has it's own html file.
	q = set_dbpath(curdb->name, http_path);

	// end of dbname
	dbpath = q;
	*q++ = '/';
	*q++ = 'd';
	*q++ = 'r';
	*q++ = 'o';
	*q++ = 'o';
	*q++ = 'm';
	*q = '\0';

	http_class_path( q, loc, 0, http_alt_room);

	// fprintf(logfile, "HTTP CLASS_PATH: %s\n", http_path);
	if( http_expandfile(d, hr, NULL, 0, http_path, NULL) >= 0){
		http_alt_room[0] = '\0';
		return 0;
	}
	q = &dbpath[6];
	while(*q)q++;

	// backup to last seperator
	while(q != &dbpath[6] && *q != '/'){
		q--;
	}
	if( *q == '/'){
		if( http_alt_room[0]){
			strcpy(&q[1], http_alt_room);
		}else {
			strcpy(&q[1], "xdefault.htm");
		}
	}
	if( http_expandfile(d, hr, NULL, 0, http_path, NULL) >= 0){
		http_alt_room[0] = '\0';
		return 0;
	}

	// backup looking for default.htm

	while( q != &dbpath[6]){
		q--;
		while(q != &dbpath[6] && *q != '/'){
			q--;
		}
		if( *q == '/'){
			if( http_alt_room[0]){
				q[1] = 'x';
				strcpy(&q[2], http_alt_room);
				p2 = &q[2];
				while(*p2)p2++;
				*p2++ = '.';
				*p2++ = 'h';
				*p2++ = 't';
				*p2++ = 'm';
				*p2++ = '\0';

			}else {
				strcpy(&q[1], "xdefault.htm");
			}
		}else {
			break;
		}
		if( http_expandfile(d, hr, NULL, 0, http_path, NULL) >= 0){
			http_alt_room[0] = '\0';
			return 0;
		}
	}

	// backup looking for zone
	q = dbpath;
	*q++ = '/';
	*q++ = 'd';
	*q++ = 'z';
	*q++ = 'o';
	*q++ = 'n';
	*q++ = 'e';
	*q = '\0';

	http_zone_path( q, zone, 0);

	// fprintf(logfile, "HTTP ZONE_PATH: %s\n", http_path);
	if( http_expandfile(d, hr, NULL, 0, http_path, NULL) >= 0){
		http_alt_room[0] = '\0';
		return 0;
	}

	http_alt_room[0] = '\0';
	return 1;
}

int guest_map[64];

int process_http_request(struct http_rec *hr)
{	char *p, *q, *pass;
	dbref player,victim;
	char cmd[128];
	const char *dbname="";
	int idx, guest;
    struct descriptor_data *d=NULL;
	database *savdb = curdb;

	if( dblist[0] && dblist[0]->name){
		dbname =  dblist[0]->name;
	}
	//fprintf(logfile,"PROCESS_HTTP(%x)cmd='%s' auth='%s'\n", hr, hr->user, hr->authbuf);

	if( strcmp(hr->user, "listdb")==0){
		fprintf(logfile,"HTTP: return dblist\n");
		p = &hr->outbuf[0]; 
		strcpy(hr->outbuf, "HTTP/1.0 200 OK\r\n");while(*p)p++;
		// fprintf(logfile,"HTTP: outbuf=%s\n", hr->outbuf);
		strcpy(p,"Pragma: no-cache\r\nCache-Control: no-cache\r\n");while(*p)p++;
		strcpy(p,"Content-Type: text/html\r\n\r\n");while(*p)p++;

		// fprintf(logfile,"HTTP: numdb=%d\n", nextdatabase);
		for(idx=0; idx < nextdatabase; idx++){
			strcpy(p, dblist[idx]->name);while(*p)p++;
			strcpy(p,"\r\n");while(*p)p++;
		}
		fflush(logfile);
		curdb = savdb;
		return 0;
	}

	// process the authinfo to get user,db and pass
	if( hr->authbuf[0] == '\0'){
		strcpy(hr->outbuf, "HTTP/1.0 401 Not Authorized\r\n");
		p = hr->outbuf; while(*p)p++;
		strcpy(p, "WWW-Authenticate: basic realm=\"prism-user\"\r\n");while(*p)p++;
		strcpy(p,"\r\n");while(*p)p++;
		curdb = savdb;
		return 0;

	}
	q = hr->authbuf;
	while(*q == ' ')q++;
	while(*q && *q != ' ')q++;
	while(*q == ' ')q++;

	q = decode64(q);
	pass = q;
	while(*pass && *pass != ':'){
		if( *pass == '@'){
			dbname = &pass[1];
			*pass = '\0';
		}
		pass++;
	}
	if( *pass == ':'){
		*pass++ = '\0';
	}
	strncpy(cmd, hr->user, 127);
	cmd[127] = '\0';

	strncpy(hr->user, q, 63);
	hr->user[63] = '\0';

	// fprintf(logfile,"HTTP: user=%s db=%s\n", q, dbname);

	// find the database to try..
	for(idx = 0; idx < nextdatabase; idx++){
		if( strcmp(dblist[idx]->name, dbname)==0){
			curdb = dblist[idx];
			break;
		}
	}
	if( idx == nextdatabase){
		// check for start locations..
		if( strcmp(dbname, "dungeon")==0){
			idx = 0;
		}else if( strcmp(dbname, "adventure")==0){
			idx = 0;
		}
//		if( idx == 0){
//			fprintf(logfile,"HTTP VIRTUAL DB: %s\n", dbname);
//		}
	}
	if( idx == nextdatabase){
		// don't know what database.
		fprintf(logfile,"HTTP: user=%s DB NOT FOUND=%s\n", q, dbname);
	    strcpy(hr->outbuf, "HTTP/1.0 404 DB not found\r\n");
		p = hr->outbuf; while(*p)p++;
		strcpy(p,"Pragma: no-cache\r\nCache-Control: no-cache\r\n");while(*p)p++;
		strcpy(p,"Content-Type: text/html\r\n\r\n");while(*p)p++;
		curdb = savdb;
		return 0;
	}

	if( strcmp(cmd, "guest")==0){
		fprintf(logfile, "HTTP Attach to a guest player\n");
		d = NULL;
		//fprintf(logfile,"PLAYER=%d curdb=%x\n", player, curdb);
		for(idx = 0; idx < 64; idx++ ){
			guest_map[idx] = 0;
		}

		for (d = descriptor_list; d; d = d->next) {
			if( d->connected==2 &&
				!(Flags(d->player)&(ROBOT)) ){
				strcpy(cmd, Name(d->player));
				if( cmd[0] == 'g' && cmd[1] == 'u' && cmd[2] == 'e' && cmd[3] == 's' && cmd[4] == 't'){
					idx = atoi( &cmd[5]);
					idx = idx - 1000;
					if( idx > 32*64){
						idx = 32*64 -1;
					}
					if( idx < 0){
						idx = 0;
					}
					guest_map[idx / 32] |= 1 << ( idx % 32);
				}
			}
		}
		if( d == NULL){
			fprintf(logfile, "HTTP: no guest connected\n");
			fprintf(logfile, "HTTP: map %08x %08x\n", guest_map[0], guest_map[1]);
		}else {
			fprintf(logfile, "HTTP: map %08x %08x\n", guest_map[0], guest_map[1]);
		}
		for( guest=0; guest < 64; guest++){
			if( guest_map[guest] != 0xffffffff){
				idx = guest;
				for(guest = 0; guest < 32; guest++){
					if( (guest_map[idx] & ( 1 << guest) ) == 0){
						break;
					}
				}
				guest = guest + 1000 + idx*32;
				break;
			}
		}
		if( guest >= 1000){
			sprintf(cmd, "guest%4d", guest);
			fprintf(logfile, "HTTP: guest=%s\n", cmd);

			victim = NOTHING;
			player = lookup_player( cmd);
			if( player == NOTHING){
	/* connect player to http session */
				d = initializesock(httpsock);
				d->curdb = curdb;
	fprintf(logfile,"DEBUG create player(%s)@%s (%s)\n", hr->user, curdb->name, pass);

				player = create_player(cmd, cmd, d);
				if( player != NOTHING){
	fprintf(logfile,"CREATED new guest %s(%d)\n", Name(player), player);
					d->player = player;
					d->connected = 2;
					d->caps |= CAP_HTML|CAP_TEXT;
					d->histlen = 20;
				}
				d = NULL;
				victim = player;
			}else {
				victim = connect_player(cmd, cmd, d);
			}
			// pass the guest name back
			strcpy(hr->outbuf, "HTTP/1.0 200 Guest OK\r\n");
			p = hr->outbuf; while(*p)p++;
			strcpy(p,"\r\n");while(*p)p++;
			strcpy(p, cmd);while(*p)p++;

			player = NOTHING;
			curdb = savdb;

			// check virtual database (start room)
#ifdef NO_QUIT
			do_no_quit(victim);
#endif
			if( strcmp(dbname, "dungeon")==0){
				moveto( victim, 3765);
			}else if( strcmp(dbname, "adventure")==0){
				moveto( victim, 5504);
			}
			return 0;
		}
	}

	if( strcmp(cmd, "cr")==0){
		victim=NOTHING;
		player = lookup_player(hr->user);
		if( player == NOTHING){
			if( !ok_player_name(hr->user)){
				fprintf(logfile,"CREATE bad user %s\n", hr->user);
			}else if( !ok_password(pass)){
				fprintf(logfile,"CREATE bad pass %s\n", hr->user);
			}else {
/* connect player to http session */
				d = initializesock(httpsock);
				d->curdb = curdb;
fprintf(logfile,"DEBUG create player(%s)@%s (%s)\n", hr->user, curdb->name, pass);

				player = create_player(hr->user, pass, d);
				if( player != NOTHING){
fprintf(logfile,"CREATED new user %s(%d)\n", Name(player), player);
					d->player = player;
					d->connected = 2;
					d->caps |= CAP_HTML|CAP_TEXT;
					d->histlen = 20;
				}
				d = NULL;
				victim = player;
			}
		}else {
			fprintf(logfile, "CREATE player %s already exists\n", hr->user);
		}
	}else {
		victim = connect_player(hr->user, pass, d);
	}

	if( victim == NOTHING){
		strcpy(hr->outbuf, "HTTP/1.0 401 Not Authorized\r\n");
		p = hr->outbuf; while(*p)p++;
		sprintf(p, "WWW-Authenticate: basic realm=\"prism-user-%s%s%s\"\r\n", hr->user, dbname?"@":"", dbname?dbname:"");while(*p)p++;
		strcpy(p,"\r\n");while(*p)p++;

		player = NOTHING;
		curdb = savdb;
		return 0;
	}
	if( strcmp(cmd, "cr")==0 ||
		strcmp(cmd, "co")==0){
		strcpy(hr->outbuf, "HTTP/1.0 200 Authorized OK\r\n");
		p = hr->outbuf; while(*p)p++;
		strcpy(p,"\r\n");while(*p)p++;

		fprintf(logfile,"HTTP 200 Authorized\n");
		player = NOTHING;
		curdb = savdb;
		return 0;
	}
	if( cmd[0] != '\0' && strcmp(cmd, "login")!= 0 && strcmp(cmd, "history")!= 0){
		if( strcmp(cmd, hr->user) != 0){
			fprintf(logfile, "HTTP: cmd=%s user=%s\n", cmd, hr->user);
			strcpy(hr->outbuf, "HTTP/1.0 401 Not Authorized\r\n");
			p = hr->outbuf; while(*p)p++;
			sprintf(p, "WWW-Authenticate: basic realm=\"prism-user-%s%s%s\"\r\n", hr->user, dbname?"@":"", dbname?dbname:"");while(*p)p++;
			strcpy(p,"\r\n");while(*p)p++;

			player = NOTHING;
			curdb = savdb;
			return 0;
		}
	}

	player = victim;

	d = NULL;
	//fprintf(logfile,"PLAYER=%d curdb=%x\n", player, curdb);

	for (d = descriptor_list; d; d = d->next) {
		if( d->connected==2 &&
			d->curdb == curdb &&
			!bad_dbref(d->player, 0) &&
			player == d->player &&
			(d->caps & CAP_HTML) &&
			!(Flags(d->player)&(ROBOT)) ){
				//fprintf(logfile,"DEBUG: found player %s@%s, caps=%x\n", hr->user, curdb->name, d->caps);
			break;
		}
	}
	if( d == NULL){
		// fprintf(logfile, "New player connection\n");
/* connect player to http session */
		d = initializesock(httpsock);
						
		d->player = player;
		d->connected = 2;
		d->curdb = curdb;
		d->caps |= CAP_HTML|CAP_TEXT;
		d->histlen = 20;
		d->ticks = timenow;
		d->last_time = timenow;
		// fprintf(logfile,"HTML CONNECT %s\n", Name(player));
	}

	if(d != NULL){
					
		q = hr->command;
		while(*q){
			if(*q == '+')*q = ' ';q++;
		}
		q = hr->command;
		if( *q == '?' && q[1] == '='){
			q+= 2;
		}else if( *q == '?' && q[1] == 'c' && q[2] == 'm' && q[3] == 'd' && q[4] == '='){
			q+= 5;
		}
		if( *q == '\0') {
			if( strcmp(cmd, "history")== 0){
				//fprintf(logfile,"HTTP: history\n");
				strcpy(hr->outbuf, "HTTP/1.0 200 OK\r\n");
				p = hr->outbuf; while(*p)p++;
				strcpy(p,"Pragma: no-cache\r\nCache-Control: no-cache\r\n");while(*p)p++;
				strcpy(p,"Content-Type: text/html\r\n\r\n");while(*p)p++;

				// finally attach to the game engine.
				hr->instate = -4;
				attach_http(d, hr);
				d->ticks = timenow;

				curdb = savdb;
				return 0;
			}
		}else{
			dehex(q);
			save_command(d, q);
		}
	    strcpy(hr->outbuf, "HTTP/1.0 200 OK\r\n");
		p = hr->outbuf; while(*p)p++;
		strcpy(p,"Pragma: no-cache\r\nCache-Control: no-cache\r\n");while(*p)p++;
		strcpy(p,"Content-Type: text/html\r\n\r\n");while(*p)p++;

		// finally attach to the game engine.
		hr->instate = -2;
		attach_http(d, hr);
	}else{
		hr->instate = -1;
	}

	curdb = savdb;
	return 0;
}


int http_pack_history(history *h)
{	int cnt =0, cnt2=0;
	struct text_block *t, *tn, *tx;

	t = h->txt.head;
	while(t){
		cnt2++;
		cnt += t->nchars;
		t = t->nxt;
	}
	if( cnt2 > 1){
		t = h->txt.head;
		// empty queue
		h->txt.head = NULL;
		h->txt.tail = &h->txt.head;

		tn = make_text_block(t->buf, cnt+16, t->nchars);
		h->txt.head = tn;
		h->txt.tail = &tn->nxt;

		cnt -= t->nchars;
		tx = t->nxt;
		free_text_block(t);
		t = tx;
		while(cnt > 0){
			bcopy(t->buf, &tn->buf[tn->nchars], t->nchars);
			tn->nchars += t->nchars;
			cnt -= t->nchars;
			tx = t->nxt;
			free_text_block(t);
			t = tx;
		}
	}
	return cnt;
}

// do entity escaping if needed.
//

struct text_block *entity_esc(struct text_block *t)
{	char *p, *p3;
	int cnt2;
	struct text_block *ttmp = NULL;

	// does t need  to be escaped?
	p = t->buf;
	for(cnt2=0; cnt2 < t->nchars; cnt2++){
		if( *p == '<' || *p == '"' || *p == '&'|| *p == '>' || *p == '\''){
			break;
		}
		p++;
	}
	if( cnt2 < t->nchars){
		ttmp = make_text_block("", 8192, 0);
		p = t->buf;
		ttmp->token = t->token;
		for(cnt2=0; cnt2 < t->nchars; cnt2++){
			p3 = NULL;
			switch(*p){
			case '<':
				p3 = "&lt;";
				break;
			case '>':
				p3 = "&gt;";
				break;
			case '&':
				p3 = "&amp;";
				break;
			case '"':
				p3 = "&quot;";
				break;
			case '\'':
				p3 = "&#39;";
				break;
			default:
				ttmp->buf[ttmp->nchars++] = *p;
				break;
			}
			if( p3 != NULL){
				while(*p3){
					ttmp->buf[ttmp->nchars++] = *p3++;
				}
			}
			p++;
		}
		t = ttmp;
	}
	return ttmp;
}

// this routine handles the quoting for javascript.
// if the string ends with an obj ref then it is removed as well.

struct text_block *script_esc(struct text_block *t)
{	char *p, *p3;
	int cnt2;
	struct text_block *ttmp = NULL;
	int savednchars = t->nchars;

	// does t need  to be escaped?
	p = t->buf;
	for(cnt2=0; cnt2 < t->nchars; cnt2++){
		if( *p == '<' || *p == '"' || *p == '&' || *p == '\'' || *p == ';'){
			break;
		}
		p++;
	}
	// look for obj ref at end.. (#NNN [FLAGS])
	p = &t->buf[t->nchars-1];
	// wont loop, will always break out..
	while( *p == ')' ){
		p--;
		while( p != t->buf && *p >= 'A' && *p <= 'Z'){
			p--;
		}
		if( *p != ' '){
			break;
		}
		p--;
		while( p != t->buf && *p >= '0' && *p <= '9'){
			p--;
		}
		if( *p == '#' && p[-1] == '('){
			// remove the obj ref..
			p--;
			while( &t->buf[t->nchars] != p){
				t->nchars--;
			}
			// flag conversion needed.
			cnt2 = t->nchars -1;
		}
		break;
	}

	if( cnt2 < t->nchars){
		ttmp = make_text_block("", 8192, 0);
		p = t->buf;
		ttmp->token = t->token;
		for(cnt2=0; cnt2 < t->nchars; cnt2++){
			p3 = NULL;
			switch(*p){
			case ';':
				// send up to the ;
				cnt2 = t->nchars; // make for loop exit.
				break;
			case '<':
				p3 = "&lt;";
				break;
			case '>':
				p3 = "&gt;";
				break;
			case '&':
				p3 = "&amp;";
				break;
			case '"':
				p3 = "\\\"";
				break;
			case '\'':
				p3 = "\\'";
				break;
			default:
				ttmp->buf[ttmp->nchars++] = *p;
				break;
			}
			if( p3 != NULL){
				while(*p3){
					ttmp->buf[ttmp->nchars++] = *p3++;
				}
			}
			p++;
		}
		t = ttmp;
	}
	return ttmp;
}

struct text_block *out_clickable(struct text_block *t, char *p, char *p2, char *p3)
{	struct text_block *cur = NULL;
	struct text_block *ttmp= NULL, *ttmp2 = NULL;
	int len, len2, len3, len4, len5;

	ttmp = script_esc(t);
	len = strlen(p);
	len2 = strlen(p2);
	len3 = strlen(p3);

	len4= t->nchars;
	if( ttmp != NULL){
		len4 = ttmp->nchars;
	}

	len5 = t->nchars;
	ttmp2 = entity_esc(t);
	if( ttmp2 != NULL){
		len5 = ttmp2->nchars;
	}

	cur = make_text_block(p, len+len2+len3+len4+len5, len);
			
	if( ttmp != NULL){
		strncpy(&cur->buf[cur->nchars], ttmp->buf, len4);
	}else {
		strncpy(&cur->buf[cur->nchars], t->buf, len4);
	}
	cur->nchars += len4;

	strcpy(&cur->buf[cur->nchars], p2);
	cur->nchars += len2;

	if( ttmp2 != NULL){
		strncpy(&cur->buf[cur->nchars], ttmp2->buf, len5);
	}else {
		strncpy(&cur->buf[cur->nchars], t->buf, len5);
	}
	cur->nchars += len5;

	strcpy(&cur->buf[cur->nchars], p3);
	cur->nchars += len3;

	if( ttmp != NULL){
		free_text_block(ttmp);
	}

	if( ttmp2 != NULL){
		free_text_block(ttmp2);
	}

	return cur;
}

struct text_block *out_span(struct text_block *t, char *p, char *p3)
{	struct text_block *cur = NULL;
	struct text_block *ttmp= NULL, *ttmp2 = NULL;
	int len, len2, len3, len4, len5;

	len = strlen(p);
	len3 = strlen(p3);
	len5 = t->nchars;
	ttmp2 = entity_esc(t);
	if( ttmp2 != NULL){
		len5 = ttmp2->nchars;
	}

	cur = make_text_block(p, len+len3+len5, len);
			
	if( ttmp2 != NULL){
		strncpy(&cur->buf[cur->nchars], ttmp2->buf, len5);
	}else {
		strncpy(&cur->buf[cur->nchars], t->buf, len5);
	}
	cur->nchars += len5;

	strcpy(&cur->buf[cur->nchars], p3);
	cur->nchars += len3;

	if( ttmp != NULL){
		free_text_block(ttmp);
	}

	if( ttmp2 != NULL){
		free_text_block(ttmp2);
	}

	return cur;
}


// convert the history info
int convert_history(struct http_rec *hr, struct descriptor_data *d, history *h)
{   struct text_block *cur, *t;
	struct text_block *tnext;
	int cnt, cnt2;
	char *p=NULL, *p2, *p3;
	int len=0, len2=0, len3=0;
	int divtype = hr->divtype, dt;
	char dbuf[128];

	tnext = h->txt.head;
	cnt = 0;
	while(tnext != NULL){
		t = tnext;
		tnext = t->nxt;
//		fprintf(logfile,"%d/%d/%d,", t->token, t->obj, t->nchars);
		if( divtype != TEXT_NORMAL){
			// do we need to end the current <div >
			dt = divtype;
			switch(divtype){
			case TEXT_CONTENTS:
				if( t->token == TEXT_CONTENTS || t->token == TEXT_GETOBJ || t->token == TEXT_EOL || t->token == TEXT_PADDING){
					break;
				}
				divtype = TEXT_NORMAL;
				break;
			case TEXT_OBVIOUS:
				if( t->token == TEXT_OBVIOUS || t->token == TEXT_EXIT|| t->token == TEXT_EOL || t->token == TEXT_PADDING){
					break;
				}
				divtype = TEXT_NORMAL;
				break;
			}

			if( divtype == TEXT_NORMAL){
				sprintf(dbuf, "</div><!-- dt=%d tok=%d -->\r\n", dt, t->token);
				p = dbuf;
				len = strlen(p);
				cur = make_text_block(p, len, len);
				*hr->output.tail = cur;
				hr->output.tail = &cur->nxt;
			}
		}
		switch(t->token){
		case TEXT_INPUT:
			cur = out_clickable(t, "<span class='hinput' OnClick='SendCmd(\"", "\")'>", "</span>");
			cnt += cur->nchars;
			break;

		case TEXT_GETOBJ:
			if(t->obj != NOTHING){
				cur = make_text_block(p, 8192, 0);
				sprintf(cur->buf, "<script> show_obj(\"%s\")</script>\n", Name0(t->obj));
				cur->nchars = strlen(cur->buf);
				*hr->output.tail = cur;
				hr->output.tail = &cur->nxt;
			}
			cur = out_clickable(t, "<span class='hget' OnClick='SendCmd(\"get ", "\")'>","</span>"); 
			cnt += cur->nchars;
			break;

		case TEXT_DROPOBJ:
			if(t->obj != NOTHING){
				do_include(d, hr, NULL, 0, t->obj, NULL);
#if 0
				cur = make_text_block(p, 8192, 0);
				sprintf(cur->buf, "<script> show_inv_obj(\"%s\")</script>\n", Name0(t->obj));
				cur->nchars = strlen(cur->buf);
				*hr->output.tail = cur;
				hr->output.tail = &cur->nxt;
#endif
			}
			cur = out_clickable(t, "<span class='hdrop' OnClick='SendCmd(\"drop ", "\")'>","</span>"); 
			cnt += cur->nchars;
			break;

			break;
		case TEXT_EXIT:
			cur = out_clickable(t, "<span class='hexit' OnClick='SendCmd(\"", "\")'>","</span>"); 
			cnt += cur->nchars;
			break;

		case TEXT_EOL:
			cur = out_span(t,"<span class='heol'>", "</span>\r\n");
			cnt += cur->nchars;
			break;

		case TEXT_EOLX:
			cur = out_span(t,"<span class='hbr'>", "</span>\r\n");
			cnt += cur->nchars;
			break;

		case TEXT_STARTCMD:
			cur = out_span(t,"<span class='hstart'>", "</span>\r\n");
			cnt += cur->nchars;
			break;

		case TEXT_ENDCMD:
			cur = out_span(t,"<span class='hend'>", "</span>\r\n");
			cnt += cur->nchars;
			break;

		case TEXT_CONTENTS:
			p = "<div class='hcontents'><span class='hprompt'";
			len = strlen(p);
			p2 = ">";
			len2 = strlen(p2);
			p3 = "</span>";
			len3 = strlen(p3);

			cur = make_text_block(p, len+len2+len3+t->nchars, len);
			
			strcpy(&cur->buf[cur->nchars], p2);
			cur->nchars += len2;

			strncpy(&cur->buf[cur->nchars], t->buf, t->nchars);
			cur->nchars += t->nchars;

			strcpy(&cur->buf[cur->nchars], p3);
			cur->nchars += len3;

			cnt += cur->nchars;
			divtype = TEXT_CONTENTS;
			break;

		case TEXT_OBVIOUS:
			p = "<div class='hobvious'><span class='hprompt'";
			len = strlen(p);
			p2 = ">";
			len2 = strlen(p2);
			p3 = "</span>";
			len3 = strlen(p3);

			cur = make_text_block(p, len+len2+len3+t->nchars, len);
			
			strcpy(&cur->buf[cur->nchars], p2);
			cur->nchars += len2;

			strncpy(&cur->buf[cur->nchars], t->buf, t->nchars);
			cur->nchars += t->nchars;

			strcpy(&cur->buf[cur->nchars], p3);
			cur->nchars += len3;

			cnt += cur->nchars;
			divtype = TEXT_OBVIOUS;
			break;
		case TEXT_PADDING:
			cur = out_span(t,"<span class='hpadding'>", "</span>\r\n");
			cnt += cur->nchars;
			break;

		default:
			cur = out_span(t,"<span class='htext'>", "</span>\r\n");
			cnt += cur->nchars;
			break;
		}
		if(cur != NULL){
			*hr->output.tail = cur;
			hr->output.tail = &cur->nxt;
		}
	}
#if 0
	if( divtype != TEXT_NORMAL){
		p = "</div> <!-- end of list -->\r\n";
		len = strlen(p);
		cur = make_text_block(p, len, len);
		*hr->output.tail = cur;
		hr->output.tail = &cur->nxt;
	}
#endif	
	hr->divtype = divtype;
	return 0;
}

int out_history(struct http_rec *hr, struct descriptor_data *d)
{   struct text_block *cur, *t;
	history *h=NULL;
	int cnt, cnt2;
	char *p;

	// push the history onto the output buffer
	h = d->history;
	if( h != NULL){
		convert_history(hr, d, h);
		h = h->prev;
		while(h != d->history){
			convert_history(hr, d, h);
			h = h->prev;
		}
	}

	return 0;
}
		
		
int do_http(fd_set *inset, fd_set *outset)
{   struct http_rec *hr = http_head, *hrn;
	int ch;
	char *p, *q, *pass;
	int cnt, cnt2;
    struct descriptor_data *d=NULL;
    dbref player = NOTHING, victim;
    struct text_block **qp, *cur;
	char *dbname = NULL;
	database *savdb = curdb;
	int idx;
	char cmd[64];
	history *h=NULL;
	char *msg;
	char dbdir[512];

    strcpy(msgbuf,"");
	strcpy(dbdir, "d/d");

	if( hr == NULL){
		return 0;
	}

	while(hr){
		if( hr->instate == -4){
			//fprintf(logfile,"HTTP -4\n");

			for (d = descriptor_list; d; d = d->next) {
				curdb = d->curdb;
				if( d->connected==2 && !bad_dbref(d->player, 0) &&
					strcasecmp(hr->user, Name(d->player))==0 &&
					(d->caps & CAP_HTML) &&
					!(Flags(d->player)&(ROBOT)) ){
					break;
				}
			}
			if( d == NULL){
				hr->instate == -1;
				continue;
			}

			out_history(hr, d);

			p = &hr->outbuf[hr->outcnt];
			hr->outpos = 0;
			hr->instate = -1;
			hr->fdc = NULL;
			detach_http(d, hr);
			continue;
		}
		// Headers output already...
		if( hr->instate == -3){
			fprintf(logfile,"HTTP: detached %d\n", hr->outcnt);
			hr->instate = -1;
		}
		if( hr->instate == -2){

				for (d = descriptor_list; d; d = d->next) {
					curdb = d->curdb;
					if( d->connected==2 && !bad_dbref(d->player, 0) &&
						strcasecmp(hr->user, Name(d->player))==0 &&
						(d->caps & CAP_HTML) &&
						!(Flags(d->player)&(ROBOT)) ){
						break;
					}
				}
				if( d == NULL){
					hr->instate == -1;
					continue;
				}


				if( d->input.head == NULL){
/* Last command we sent has been processed. */

					// Need to SHOW the current location of the player.
					// This command should generate ALL the output seen in the iframe.

					if( http_show(hr, d)){
						strncpy(&dbdir[3], curdb->name, 512 -2);
						http_expandfile(d, hr, NULL, 0, "prism.htm", dbdir);
					}

					p = &hr->outbuf[hr->outcnt];
					hr->outpos = 0;
					hr->instate = -1;
					hr->fdc = NULL;
					detach_http(d, hr);
				}
		}else if( hr->instate == -1){ /* doing output */
			if( hr->outcnt == 0){
				hr->outpos = 0;
				if( hr->fdc != NULL){
					// fprintf(logfile,"HTTP: read from file\n");
					hr->outcnt = fdc_read(hr->fdc, hr->outbuf, 8192);
					if( hr->outcnt <= 0){
						fdc_close(hr->fdc);
						hr->fdc = NULL;
						if( hr->inbuf[0] == 'h'){
							hr->inbuf[0] = 'a';
							hr->fdc = fdc_open(hr->inbuf, hr->curdir);
						}
						hr->outcnt = 0;
					}
    			}else if( hr->output.head != NULL){
					// copy up to 8192 into outbuf
					cur = hr->output.head;
					cnt = cur->nchars;
					if( cnt > 8192 ){
						cnt = 8192;
					}
					hr->outcnt = cnt;
					hr->outpos = 0;

					bcopy(cur->start, hr->outbuf, cnt);
					cur->start = &cur->start[cnt];
					cur->nchars -= cnt;

					if( cur->nchars == 0){

					    if (!cur -> nxt){
							hr->output.tail = &hr->output.head;
						}
						hr->output.head = cur -> nxt;
						free_text_block (cur);
					}
				}
				if( hr->outcnt == 0 && hr->fdc == NULL){
					//fprintf(logfile,"HTTP: close connection\n");
					close(hr->sock);
					hr = free_http(hr);
					if( hr == NULL)
						break;
					continue;
				}
			}
			if( FD_ISSET(hr->sock, outset)){
				cnt = hr->outcnt - hr->outpos;
				if( cnt > 0){
					cnt2 = write(hr->sock, &hr->outbuf[hr->outpos], cnt);
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
							close(hr->sock);
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
				if( FD_ISSET(hr->sock, inset)){
					hr->cnt = read(hr->sock, hr->outbuf, 8192);
					//fprintf(stderr, "Mud read(%d)\n", hr->cnt);

					if( hr->cnt <= 0){
						close(hr->sock);
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
					if( ch == ' '){
						hr->instate = 3;
					}else if( ch == '/' || ch == '\\'){
						hr->instate = 4;
					}else if( hr->incnt >= 12){
						hr->instate = 5;
					}else if( ch == '\r' ){
						hr->instate = 7;	/* wait for \n */
					}else if( ch == '\n' ){
						hr->instate = 8;
					}
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
						for(cnt = 0; cnt < 250 && *p; cnt++){
							p++;
						}
						*p++ = '/';
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
#if 0
					if( ch == '?' && hr->incnt == 10 ){
						hr->inbuf[hr->incnt+1] = '\0';
						if( strcasecmp(&hr->inbuf[1], "prism.php?")== 0){
							// skip over the prefix
							fprintf(logfile, "http: skip prefix\n");
							hr->incnt = 0;
							break;
						}
					}
#endif
					if( ch == '~' && hr->incnt == 1){
							hr->incnt = -1;
							hr->instate = 20;
							hr->command[0] = '\0';
							hr->authbuf[0] = '\0';
							hr->user[0] = '\0';
							break;
					}
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
					
	/* User browsing stuff */
				case 20:
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					hr->user[hr->incnt] = ch;

					if( hr->incnt == 62)
						hr->incnt--;
					if( ch == ' '){			/* http 1.x */
						hr->instate = 29;
						hr->user[hr->incnt] = '\0';
					}
					else if( ch == '/' || ch == '\\' || ch == '&'){
						hr->instate = 30;
						hr->user[hr->incnt] = '\0';
						hr->incnt = -1;
						break;
					}
					else if( ch == '\r'){
						hr->instate = 27;
						hr->user[hr->incnt] = '\0';
						break;
					}
					else if( ch == '\n'){
						hr->user[hr->incnt] = '\0';
						hr->instate = 28;		/* got 0.9 type */
						break;
					}
					break;              
				case 23:						/* end of headers?*/
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n' && hr->incnt == 0){
						hr->instate = 40;
						break;
					}
					hr->incnt = -1;
					hr->instate = 25;			/* go back and look for www-auth header */
					break;
				case 24:
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					hr->authbuf[hr->incnt] = ch;
					if( hr->incnt == 126)hr->incnt--;
					if( ch == '\r'){
						hr->authbuf[hr->incnt] = '\0';
						hr->incnt = -1;
						hr->instate = 23;
					}else if( ch == '\n'){
						hr->authbuf[hr->incnt] = '\0';
						hr->incnt = -1;
						hr->instate = 25;		/* go back and look for a new header */
					}
					break;
				case 25:
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					if( hr->incnt == 126)hr->incnt--;
					hr->inbuf[hr->incnt] = ch;
					
					if( ch == ':'){
						hr->inbuf[hr->incnt] = '\0';
						hr->incnt = -1;
						if( strncasecmp(hr->inbuf,"authorization", 13) ){
							hr->incnt = -1;
						}else{
							hr->instate = 24;		/* get auth data */
						}
					}else if(ch == '\r'){
						hr->instate = 23;
					}else if( ch == '\n'){
						if( hr->incnt == 0){
							hr->instate = 40;	/* assume done */
							break;
						}
						hr->incnt = -1;			/* fetch another */
					}
					break;
					
				case 26:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n'){
						hr->instate = 25;		/* start looking for auth stuff */
						hr->incnt = -1;
					}
					break;
					
				case 27:
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\n')hr->instate = 28;
					break;
					
				case 29:	/* look for \r\n */
					ch = hr->outbuf[hr->pos++]&0xff;
					if( ch == '\r')hr->instate = 26;
					break;
					
				case 28:	/* end of request */
/* won't get here  see below */
					break;
					
				case 30:
					ch = hr->outbuf[hr->pos++]&0xff;
					hr->incnt++;
					hr->command[hr->incnt] = ch;

					if( hr->incnt == 1020)
						hr->incnt--;
					if( ch == ' '){
						hr->command[hr->incnt] = '\0';
						hr->instate = 29;
					}else if( ch == '\r'){
						hr->command[hr->incnt] = '\0';
						hr->instate = 27;
					}else if( ch == '\n'){
						hr->instate = 28;
						hr->command[hr->incnt] = '\0';
					}
					break;					
				}
				if( hr->instate == 2 || hr->instate == -1){
					break;
				}
			}/* end while */
			if( hr->instate == 2){
				close(hr->sock);
				hr = free_http(hr);
				if( hr == NULL)
					break;
				continue;
			}
			if( hr->pos >= hr->cnt)
				hr->cnt = 0;
			if( hr->instate == 28){
/* process 0.9 style request */
				hr->instate = -1;
				strcpy(hr->outbuf, "<HTML><HEAD><TITLE>WEB Prism</TITLE></HEAD><BODY><H1>Sorry 0.9 requests not supported</H1></BODY>\r\n");
				hr->outpos = 0;
				hr->outcnt = strlen(hr->outbuf);
			}
			if( hr->instate == 40){
/* process 1.x style request */
				hr->instate = -1;

				process_http_request(hr);

// send return value.
				hr->outpos = 0;
				hr->outcnt = strlen(hr->outbuf);
				// fprintf(logfile,"HTTP: pos=%d cnt=%d\n", hr->outpos, hr->outcnt);
			}

		}
	
		hr = hr->next;
	}
	curdb = savdb;
	return 0;
}

int http_setfd(fd_set *inset,fd_set *outset)
{	struct http_rec *hr = http_head;
	int maxd = 0;

	if( hr == NULL){
		return 0;
	}
	while(hr){
		if( hr->instate == -2){
/* ignore this one */
		}else if( hr->instate == -1){	/* done input */
			if( maxd < hr->sock){
				maxd = hr->sock+1;
			}
			FD_SET(hr->sock, outset);
		}else{
			if( maxd < hr->sock){
				maxd = hr->sock+1;
			}
			FD_SET(hr->sock, inset);
		}
		hr = hr->next;
	}
	return maxd;
}

int new_http(SOCKET sock)
{	struct http_rec *hr = (struct http_rec *)malloc( sizeof(struct http_rec));
	SOCKET nc;
	struct sockaddr_in addr;
	int addrlen = sizeof(struct sockaddr_in);
	int val = 1;
	
	nc = accept(sock, (struct sockaddr *)&addr, &addrlen);
	if( nc == INVALID_SOCKET){
		return 1;
	}
	if( setsockopt(nc, IPPROTO_TCP, TCP_NODELAY, (const char *)&val, sizeof(val)) < 0){
		return 1;
	}

		
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

	hr->description.head = NULL;
	hr->description.tail = &hr->description.head;
	hr->output.head = NULL;
	hr->output.tail = &hr->output.head;
	
	hr->next_d = NULL;
	hr->divtype = TEXT_NORMAL;
	return 0;
}

