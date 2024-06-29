#include "copyrite.h"

#include <stdio.h>

#ifndef MSDOS
#include <sys/types.h>
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
#include "externs.h"

static const char *flushed_message = "<Output Flushed>\r\n";

#ifndef MSC
#ifndef MSDOS
extern int	errno;
#endif
#endif

struct text_block *size64,*size128,*size256,*size512,*size1024;

void show_text_stats(player)
dbref player;
{	int n;
	struct text_block *t;
	int s64, s128, s256, s512, s1024;
	
	t=size64;
	for(n=0; t;t=t->nxt)n++;
	s64 = n;
	
	t=size128;
	for(n=0; t;t=t->nxt)n++;
	s128 = n;
	
	t=size256;
	for(n=0; t;t=t->nxt)n++;
	s256 = n;

	t=size512;
	for(n=0; t;t=t->nxt)n++;
	s512 = n;
	
	t=size1024;
	for(n=0; t;t=t->nxt)n++;
	s1024 = n;

	sprintf(msgbuf,"Size %4d %4d %4d %4d %4d", 64, 128, 256, 512, 1024);
	notify(player,msgbuf);
	sprintf(msgbuf,"Blks %4d %4d %4d %4d %4d", s64, s128, s256, s512, s1024);
	notify(player,msgbuf);

}


struct text_block *make_text_block(s,n)
char *s;
int n;
{
	struct text_block *p,**p1;
	int size;
	for(size=64; size <= n && size < 2048; size = size*2);

	p1 = NULL;
	if( n < 64 && size64){
		p1 = &size64;
	}else if(n < 128 && size128){
		p1 = &size128;
	}else if(n < 256 && size256){
		p1 = &size256;
	}else if(n < 512 && size512){
		p1 = &size512;
	}else if(n < 1024&& size1024){
		p1 = &size1024;
	}

	if(!p1){
		MALLOC(p, struct text_block, 1);
		if( size != 2048){
			MALLOC(p->buf, char, size);
		}else {
			MALLOC(p->buf, char, n);
		}
	}else {	/* unlink it */
		p = *p1;
		*p1 = p->nxt;
	}
	bcopy (s, p->buf, n);
	p->nchars = n;
	p->start = p->buf;
	p->nxt = 0;
	p->size= size;
	return p;
}

void free_text_block (t)
struct text_block *t;
{	struct text_block **p = NULL;

	if(t->size == 64){
		p = &size64;
	}else if( t->size == 128)
		p = &size128;
	else  if( t->size == 256)
		p = &size256;
	else  if( t->size == 512)
		p = &size512;
	else  if( t->size == 1024)
		p = &size1024;
	else {
		FREE (t->buf);
		FREE ((char *) t);
		return;
	}
	t->nxt = *p;
	*p = t;
}

void add_to_queue(q, b, n)
struct text_queue *q;
const char *b;
int n;
{
    struct text_block *p;

    if (n == 0) return;

    p = make_text_block ((char *)b, n);
    p->nxt = 0;
    *q->tail = p;
    q->tail = &p->nxt;
}

/* Replace text in a block */

void replace_text(q, t, b, o, l)
struct text_queue *q;
struct text_block *t;
const char *b;
int o,l;
{
	struct text_block *p, *p1;
	int n,cnt;
	char *b1;
	extern char *Lastcommand;

	if( !q || !t || !b){
fprintf(stderr,"Bad args in replace_text. Lastcommand = %s\n", Lastcommand);
		return;
	}

	for(b1 =(char *) b; *b1;b1++)
		if(*b1 == '$')
			*b1 = '#';	/* replace $ with # to stop loops.*/

	cnt = strlen( b);
	
	if( q == NULL || t == NULL)
		return;		/* Bad arguments */
	if( o+l > t->nchars)
		return;		/* Bad arguments */

	n = t->nchars - o - l;
	if( n > 0){
		p = make_text_block ( &t->start[o+l], n);
	}else p = NULL;

	p1= make_text_block ((char *) b, cnt);

	/* link in new blocks */
	if( p ){
		p->nxt = t->nxt;
		if( q->tail == &t->nxt)
			q->tail = &p->nxt;	/* t was last in list */

		p1->nxt = p;
	}else {
		p1->nxt = t->nxt;
		if( q->tail == &t->nxt)
			q->tail = &p1->nxt; /* t was last in list */
	}
	
	t->nxt = p1;

	t->nchars = o;			/* Adjust new length of t */

}


int flush_queue(q, n)
struct text_queue *q;
int n;
{
        struct text_block *p;
	int really_flushed = 0;
	
	n += strlen(flushed_message);

	while (n > 0 && (p = q->head)) {
	    n -= p->nchars;
	    really_flushed += p->nchars;
	    q->head = p->nxt;
	    free_text_block (p);
	}
	p = make_text_block((char *)flushed_message, strlen(flushed_message));
	p->nxt = q->head;
	q->head = p;
	if (!p->nxt)
	    q->tail = &p->nxt;
	really_flushed -= p->nchars;
	return really_flushed;
}

int queue_write( d, b, n)
struct descriptor_data *d;
const char *b;
int n;
{
    int space;

    space = MAX_OUTPUT - d->output_size - n;
    if (space < 0)
        d->output_size -= flush_queue(&d->output, -space);
    add_to_queue (&d->output, b, n);
    d->output_size += n;
    return n;
}

int queue_string( d, s)
struct descriptor_data *d;
const char *s;
{	int ret;
	struct text_block p, *p1;
	
    ret = queue_write (d, s, strlen (s));
	p1 = (struct text_block *) d->output.tail;
	return ret;
}

void freeqs(d)
struct descriptor_data *d;
{
    struct text_block *cur, *next;

    cur = d->output.head;
    while (cur) {
	next = cur -> nxt;
	free_text_block (cur);
	cur = next;
    }
    d->output.head = 0;
    d->output.tail = &d->output.head;

    cur = d->input.head;
    while (cur) {
	next = cur -> nxt;
	free_text_block (cur);
	cur = next;
    }
    d->input.head = 0;
    d->input.tail = &d->input.head;

    if (d->raw_input)
        FREE (d->raw_input);
    d->raw_input = 0;
    d->raw_input_at = 0;
}


/* Code to randomise messages */

char *ranmsg(table, cnt)
char *table[];
int cnt;
{
	return table[ random() % cnt];
}

/* Put a command back into the input stream */
/* Added by PJC 23/9/91 */

int queue_full(q)
struct text_queue *q;
{	int n = 0;
	struct text_block *t;

	t = q->head;
	while( n++ < 2 && t && t != *(q->tail) )t = t->nxt;
	if( n == 2)
		return 1;
	return 0;
}

void push_on_queue(q, b, n)
struct text_queue *q;
const char *b;
int n;
{
    struct text_block *p;

    if (n == 0) return;

    p = make_text_block ((char *)b, n);
    p->nxt = q->head;
    if( !q->head){
	q->tail = &(p->nxt);
    }
    q->head = p;
}

