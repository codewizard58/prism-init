#include "copyrite.h"

#include <stdio.h>

#ifndef MSDOS
#include <sys/types.h>
#endif


#ifdef MSDOS
#include "defs.h"

#ifndef NONET
#include <sys/tk_types.h>

#include <tk_errno.h>
#endif

#ifndef FD_SET
#ifndef _WINDOWS
#include "fd_set.h"
#endif
#endif

#else
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>
#endif


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

/* #define OUTPUT_WRAP */
#define OUTPUT_WIDTH 76

#define INPUT_EDIT

#ifdef MSDOS
struct timezone {
char * zone;
};

#endif

extern long timenow;
extern int xargc;
extern char **xargv;
#ifndef MSC
#ifndef MSDOS
extern int      errno;
#endif
#endif

#include "descript.h"

extern struct descriptor_data *descriptor_list;

extern int sock;
extern int ndescriptors;

extern void freeqs( PROTO(struct descriptor_data *));
extern const char *addrout ( PROTO(long));
extern char *strsave ( PROTO(const char *));
extern int queue_string( PROTO2(struct descriptor_data *, const char *));
extern int queue_write( PROTO3(struct descriptor_data *, const char *, int));
int process_output( PROTO(struct descriptor_data *));
int process_input( PROTO(struct descriptor_data *));
#ifndef MSDOS
int bailout( PROTO3(int, int, struct sigcontext *));
#endif

extern int quiet;               /* turn off notifies */
extern int work;

#ifndef EAGAIN
#define EAGAIN 98
#endif
/* from text.c */

void replace_text( PROTO5(struct text_queue *, struct text_block *, const char *, int, int));
/* */


#ifdef OUTPUT_WRAP

static int token_length(d, cur, len)
struct descriptor_data *d;
struct text_block *cur;
int *len;
{    char *p = cur->start;
    struct text_queue *q = &d->output;
    int cnt,n;
    int states = 0;
    int nch;

    cnt = 0;
    n = cur->nchars;
    nch = n;

    while(n-- ){
	if(*p == '\n' || *p == '\015'){
	    *len = -(nch-n);
	    return nch-n;    /* output up till the newline */
	}
	if( states != 2 && *p != ' ' && *p != ' '){
	    if( states != 2 && *p == '\\' && n > 0){
			switch(p[1]){
			case 'n':
			case 'r':
				replace_text(q, cur, "\r\n", nch-n-1, 2);
				return nch-n-1;
			case 't':
				replace_text(q, cur, "\t", nch-n-1, 2);
				return nch-n-1;
			case '\\':
				replace_text(q, cur, "\\", nch-n-1, 2);
				return nch-n-1;
			default:
				break;
			}
	    }
	    p++;
	    states = 1;
	    if(OUTPUT_WIDTH < *len+nch -n){
		if(*len || cnt)
			return cnt;     /* current token overflows */
		else
			return nch-n;
	    }
	}else if( states != 1 && (*p == ' ' || *p == '\t')){
	    p++;
	    states = 2;    /* whitespace */
	    if(OUTPUT_WIDTH < *len + nch -n){
		if(*len || cnt)
			return cnt;     /* current token overflows */
		else
			return nch-n;
	    }
	}else {    /* end of token so remember length so far */
	    n++;        /* try again.*/
	    if(states == 2)
		cnt = nch-n;
	    states = 0;
	}
    }

/* end of block that we are interested in. */
	while( states == 1 && cur->nxt){
		cur = cur->nxt;
		n = cur->nchars;
		p = cur->start;

		while(n-- ){
			if(*p == '\n' || *p == '\015'){
				return nch;     /* output all prev block */
			}
			if( states != 2 && *p != ' ' && *p != ' '){
				p++;
				states = 1;
				if(OUTPUT_WIDTH < *len+cur->nchars -n){
				if(*len || cnt)
					return cnt;     /* current token overflows */
				else
					return cur->nchars-n;
				}
			}else {
				return nch; /* output all the block */
			}
		}
	}
	return nch;
}
#else
static int token_length(d, cur, len)
struct descriptor_data *d;
struct text_block *cur;
int *len;
{    char *p = cur->start;
    struct text_queue *q = &d->output;
    int cnt,n;
    int states = 0;
    int nch;

    cnt = 0;
    n = cur->nchars;
    nch = n;
    return nch;
}
#endif



int process_output( d)
struct descriptor_data *d;
{
    struct text_block **qp, *cur;
    int cnt,loopcnt;
    int retn, ilen = 0;

#ifdef MSDOS
    if( d->raw_input){
	mud_write(d->descriptor, "\r",1);
	ilen = strlen( d->raw_input);
    }
#endif

    loopcnt = 5;

    for (qp = &d->output.head; cur = *qp;) {
	work = 1;
	if( d->descriptor < 0 && Flags(d->player)&ROBOT){
		cnt = cur->nchars;
	}else {
		if(cur->nchars){
#ifdef NOTDEF
			cnt = token_length(d, cur, &(d->olen));         

			if( cnt == 0){  /* need to wrap */
				cnt = mud_write(d->descriptor, "\r\n",2);
				if(cnt <= 0){
					if (errno == EWOULDBLOCK){
						retn=1;
					}else if(errno == 0){
						retn = 1;
					}else
						retn = 0;
					goto done;
				}
				d->olen = 0;
				continue;               /* Try again */
			}
#else
			cnt = cur->nchars;
#endif
			cnt = mud_write (d->descriptor, cur -> start, cnt);
		} else {
			cnt = 0;
		}
	}

	if (cnt < 0) {
	    if (errno == EWOULDBLOCK)
		retn = 1;
	    else if( errno == 0){
		retn = 1;
	    }else
		retn = 0;
	    goto done;
	}
	d->olen += cnt;

	d->output_size -= cnt;
	if (cnt == cur -> nchars) {
	    if (!cur -> nxt)
			d->output.tail = qp;
	    *qp = cur -> nxt;
	    free_text_block (cur);
	    continue;           /* do not adv ptr */
	}
	cur -> nchars -= cnt;
	cur -> start += cnt;
	if( loopcnt-- == 0)
		break;
    }
    retn = 1;
done:

#ifdef MSDOS
    if( d->raw_input && ilen){
	if( d->olen )
		mud_write(d->descriptor, "\r\n",2);
	mud_write(d->descriptor, d->raw_input, ilen);
    }
#endif
    return retn;
}


void save_command (d,command)
struct descriptor_data *d;
const char *command;
{
/* 19-2-93 */
    if( (d->caps & CAP_BSX) == 0 && queue_full(&d->input)){
fprintf(logfile,"Throwing (%d) %s\n", d->player, command);
    }else{
	add_to_queue (&d->input, command, strlen(command)+1);
    }
}

void push_command (player, command)
dbref player;
const char *command;
{       int f;
	struct descriptor_data *d;


	if( bad_dbref(player, 0)){
		return;
	}
		f = Flags(player);

	if( (f & (TYPE_MASK|ROBOT))== TYPE_PLAYER){
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected>= 0 && d->connected <= 2 && 
				d->player == player) {

				if( !queue_full(&d->input))
					push_on_queue( &d->input, command, strlen(command)+1);
				else {
fprintf(logfile,"Queue full in push_command(%d, '%s')\n", player, command);
				}
			}
		}
	}else {
		push_command_robot(player, command);
	}
}

int process_input (d)
struct descriptor_data *d;
{
    char *buf = NULL;
    int got,left=0;
    char *p, *pend, *q, *qend;
    extern int bad_host();
    int skipnl = 0;             /* used by escaped EOL code */

    buf = (char *)malloc(1024);
    if(!buf){
ret0:
	if( buf)
		free(buf);
	return 0;
    }

    got = mud_read (d->descriptor, buf, 1022 );
    if (got <= 0){
	if( errno == EAGAIN)
		goto ret1;
	perror("read");
	goto ret0;
    }
    buf[got] = '\0';                    /* makes dbx easier */


    if (!d->raw_input) {
	MALLOC(d->raw_input,char,MAX_COMMAND_LEN);
	d->raw_input_at = d->raw_input;
    }

    p = d->raw_input_at;
    pend = d->raw_input + MAX_COMMAND_LEN - 1;
    for (q=buf, qend = buf + got; q < qend; q++) {
	work = 1;

#ifdef INPUT_EDIT
	if ((*q == '\377' || *q == '\177'  || *q == '\010') && p != d->raw_input){
		p--;
		*p = '\0';
		continue;
	}
	if (*q == '\022' && p > d->raw_input){  /* ^r Redraw */
		*p = '\0';
		if( 0 > mud_write(d->descriptor, "\r\n", 2) )
			goto ret0;
		if( 0 > mud_write(d->descriptor, d->raw_input, strlen(d->raw_input)) )
			goto ret0;
		continue;
	}
	if (*q == '\030'){      /* line kill ^x */
		p = d->raw_input;
		*p = '\0';
		if( 0 > mud_write(d->descriptor, "XXX\r\n", 2) )
			goto ret0;
		continue;
	}
#endif
/* At the end of the current line. */
	if (*q == '\n' ||  *q == '\r') {
		*p = '\0';
/* Remove consequtive EOL chars */
		while( q != qend-1 && (q[1] == '\n' || q[1] == '\r')){
			q++;
		}

		if(     d->descriptor >= MAX_USERS && !d->output.head)
			goto ret0;
		if (p > d->raw_input){
			if( p[-1] == '\\'){
				p--;
				*p= '\0';
			}else {
				save_command (d, d->raw_input);
				if( !d->output.head && bad_host(d->host ) )
					d->connected = 3;       /* shutdown */
				p = d->raw_input;
				*p = '\0';
				continue;
			}
		}else{
			p = d->raw_input;
			*p = '\0';
			continue;
		}
	}else if(p < pend && isascii (*q) && isprint (*q)) {
		*p++ = *q;
		*p = '\0';
		continue;
	}
    }
    if (p > d->raw_input) {
		d->raw_input_at = p;
    }else{
	FREE(d->raw_input);
	d->raw_input = 0;
	d->raw_input_at = 0;
    }
ret1:
    if( buf)
	free(buf);
    return 1;
}
