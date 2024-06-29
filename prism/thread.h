/* thread.h
 * By P.J.Churchyard
 * Copyright 1991 (c)
 * All rights reserved
 */

#ifndef _THREAD_

#define _THREAD_ 1

#ifndef DSTKSIZE
#define DSTKSIZE 100
#define RSTKSIZE 100
#endif

struct thread_rec {
int fbase;
int fstate;
DATUM *fdp;		/* Datastack pointer */
struct dict **fip;	/* Players IP pointer */
struct dict *fi_interp;	/* The interpreter they are using */

struct dict *fdictop;
struct dict **fdtop;
struct dict *fdhead;
struct dict **fdtemp;
struct dict **fcodebuf;
struct dict ***fipsp;	/* Players Stack pointer */

char *finptr;
FILE *finfile;
FILE *foutfile;
FILE *fstdin;
FILE *fstdout;
struct dict *foutfunc, *finfunc;
struct dict **fcurrent,**fcontext;

DATUM fdstack[DSTKSIZE];		/* data stack */
struct dict **ipstack[RSTKSIZE];	/* return stack */
char finbuf[256];
};

extern struct thread_rec *thread;

#define ip (thread->fip)
#define dp (thread->fdp)
#define ipsp (thread->fipsp)
#define datastack (thread->fdstack)
#define retstk (thread->ipstack)
#define dictop (thread->fdictop)
#define dtop (thread->fdtop)
#define dhead (thread->fdhead)
#define dtemp (thread->fdtemp)
#define state (thread->fstate)
#define codebuf (thread->fcodebuf)

#define inbuf (thread->finbuf)
#define inptr (thread->finptr)
#define infile (thread->finfile)
#define outfile (thread->foutfile)
#define Stdin (thread->fstdin)
#define Stdout (thread->fstdout)
#define outfunc (thread->foutfunc)
#define infunc (thread->finfunc)
#define base (thread->fbase)
#define i_interp (thread->fi_interp)
#define current (thread->fcurrent)
#define context (thread->fcontext)

#endif _THREAD_
