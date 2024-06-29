#include "copyrite.h"

#include <stdio.h>
#include <ctype.h>

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "boolexp.h"
#include "match.h"

#ifdef MSC
#include <process.h>
#endif
#include "bsx.h"

#define AUTO_TOAD_DAYS  60      /* How many days after which players are Auto toaded */

#define DB_MSGLEN (MAX_COMMAND_LEN)

#ifdef EXTRACT
extern int extracting;
extern dbref *extract_list;
extern int extract_top;
#endif

#ifdef MSDOS
extern long mem_malloced;

#ifndef freemsg
#define freemsg(x, m) dosfreemsg(x, m)
extern void dosfreemsg(char *, char *);
#endif

#endif

#ifndef freemsg
#define freemsg(x, m) free(x)
#endif

#ifdef VM_DB
#define DbWrite(x, y) *vm_dbw(x) = y
#else
#define DbWrite(x, y) Db(x) = y
#endif



extern char *db_unparse_boolexp();

#ifndef VM_DB
struct object **db = NULL;
#endif

dbref db_top = 0;
int db_base = 0;
int nstrings;
long strsize;
extern void need_ext();
extern void need_lock( PROTO2(dbref, int));
extern int reset_money;
extern int no_robots;
extern void free_locks();

#ifdef VM_DESC
extern FILE *descf;
extern char *desc_name;
#endif

/* New variables */
int dbversion, newdbversion;
dbref default_room_class = NOTHING;
dbref default_thing_class= NOTHING;
dbref default_player_class=NOTHING;
dbref default_player_start=NOTHING;
dbref default_wizard     = NOTHING;
dbref default_beaker     = NOTHING;
dbref default_temple_class= NOTHING;    /* Only used by translate */
int merging;
dbref loc_room,loc_player,loc_thing;

#ifdef MSDOS
extern long ageof();
#endif

struct exit_rec{        /* this is only used to translate TINYMUD databases to
			 * PRISM ones */
struct exit_rec *next;
dbref room;
dbref exit;
};

struct exit_rec *exit_list;

void Putc(ch, f)
char ch;
FILE *f;
{       int n = ch;
	putc(n,f);
}

#ifndef DB_INITIAL_SIZE
#ifdef MSDOS
#define DB_INITIAL_SIZE 2000l
#else
#define DB_INITIAL_SIZE 10000
#endif
#endif /* DB_INITIAL_SIZE */

#ifdef DB_DOUBLING

dbref db_size = DB_INITIAL_SIZE;
#endif /* DB_DOUBLING */

#ifndef _WINDOWS
void abort_msg(msg)
char *msg;
{       fprintf(logfile,"\nAbort -%s", msg);
	fprintf(logfile,"  db_top=%d\n",db_top);
	fflush(logfile);
	abort();
}
#endif

static char *oldreqbuf;

char *requote(s)
char *s;
{       char *p;
	int n =0;

	if(! oldreqbuf ){
		oldreqbuf = (char *)malloc(2050);
		if(!oldreqbuf){
fprintf(logfile,"No memory in requote\n");
			return s;
		}
	}
	p = oldreqbuf;
	if( s){
		while(*p = *s && n++ < 2048){
			if(*s == '"')
				*p++ = '\\';
			*p++ = *s++;
		}
	}
	*p = '\0';
	return oldreqbuf;
}

const char *alloc_string(strng)
const char *strng;
{
    char *s;
    int n;

    /* NULL, "" -> NULL */
    if(strng == NULL || *strng == '\0') return NULL;

    n = strlen(strng);

    nstrings++;
    strsize += n;

    if((s = (char *) malloc(n+1)) == 0) {
		abort_msg("No mem in Alloc_string\n");
    }
    strncpy(s, strng,n);
    s[n] = '\0';
    return s;
}

void free_string(str)
char *str;
{       int n;

	if( !str) return;
	n = strlen(str);
	nstrings--;
	strsize -= n;
	freemsg( (FREE_TYPE) str, "free_string");
}

#ifdef MSDOS
#ifdef TEST_MALLOC
#define realloc(x, y) dosrealloc(x, y)
char *dosrealloc(char *, int);
#endif
#endif


#ifndef VM_DB
#ifdef DB_DOUBLING

static void db_grow( newtop)
dbref newtop;
{
    struct object **newdb;

    if(newtop > db_top) {
	db_top = newtop;
	if(!db) {
	    /* make the initial one */
	    db_size = DB_INITIAL_SIZE;
	    if((db = (struct object **)
		malloc(db_size * Sizeof(struct object *))) == 0) {
			exit(98);
	    }
	}
	
	/* maybe grow it */
	if(db_top > db_size) {
	    /* make sure it's big enough */
	    while(db_top > db_size) db_size *= 2;
	    if((newdb = (struct object **)
		realloc( (FREE_TYPE)  db,
			db_size * Sizeof(struct object *))) == 0) {
		abort_msg("DB_Grow\n");
	    } 
	    db = newdb;
	}
/* Allocate some storage */
	Db(db_top-1) = (struct object *)malloc(Sizeof( struct object));
	if(! Db(db_top-1)){
		abort_msg("DB_Grow4\n");
	}
    }
}

#else /* DB_DOUBLING */
dbref db_size;

static void db_grow( newtop)
dbref newtop;
{
    struct object **newdb;

    if(newtop > db_top) {
	db_top = newtop;
	if(db && db_top >= db_size) {
	    if((newdb = (struct object **)
		realloc( (FREE_TYPE)  db,
			db_top * Sizeof(struct object*))) == 0) {
		abort_msg("DB_Grow2\n");
	    } 
	    db = newdb;
	} else if(!db){
	    /* make the initial one */
	    db_size = DB_INITIAL_SIZE;
	    if((db = (struct object **)
		malloc(DB_INITIAL_SIZE * Sizeof(struct object *))) == 0) {
		abort_msg("DB_Grow3\n");
	    }
	}
	Db(db_top-1) = (struct object *)malloc(Sizeof( struct object));
	if(! Db(db_top-1)){
		abort_msg("DB_Grow4\n");
	}
    }
}
#endif /* DB_DOUBLING */

#endif  /* VM_DB */

dbref new_object()
{
    dbref newobj;
    struct object *o;

    newobj = db_top;
    db_grow(db_top + 1);

    /* clear it out */
    o = Db(newobj);
    o->ver = VERD;
    o->name = NULL;
    o->description = NULL;
    o->location = NOTHING;
    o->contents = NOTHING;
    o->next = NOTHING;
    o->lock  = NULL;
    o->owner = NOTHING;
    o->pennies = 0;
    /* flags you must initialize yourself */
    o->ext = NULL;
    o->age = 0l;
#ifdef Notdef
    o->commands = NOTHING;
#endif
    o->class = NOTHING;

    return newobj;
}

/* This routine is used by make_rubbish */

void change_object_type(i, typ)
dbref i;
int typ;
{
	register int cnt = sizeof( struct small_object);
	register char *p, *p1;
	struct object *o;
	struct player_object *po;
	struct room_object *ro;
	struct thing_object *to;
	struct exit_object *eo;

	o = Db(i);
	p = (char *)o;

	switch( typ ){
	case TYPE_PLAYER:
		po = (struct player_object *)malloc( sizeof( struct player_object) );
		p1 = (char *)po;
		while(cnt--)
			*p1++ = *p++;
		cnt = sizeof( struct extension );
		while(cnt--)
			*p1++ = 0;

		DbFree( i);
		DbWrite(i, (struct object *)po);
		Db(i)->ver = VERP;
#ifdef VM_DB
		free((FREE_TYPE) po);
#endif
		break;

	case TYPE_ROOM:
		ro = (struct room_object *)malloc( sizeof( struct room_object) );
		p1 = (char *)ro;
		while(cnt--)
			*p1++ = *p++;

		ro->zone = 0;

		DbFree( i);
		DbWrite(i, (struct object *)ro); 
		Db(i)->ver = VERR;
#ifdef VM_DB
		free((FREE_TYPE) ro);
#endif
		break;

	case TYPE_EXIT:
		eo = (struct exit_object *)malloc( sizeof( struct exit_object) );
		p1 = (char *)eo;
		while(cnt--)
			*p1++ = *p++;

		DbFree( i);
		DbWrite(i,(struct object *)eo);

		Db(i)->ver = VERE;
		Key(i) = TRUE_BOOLEXP;
#ifdef VM_DB
		free((FREE_TYPE) eo);
#endif
		break;

	case TYPE_THING:
		to = (struct thing_object *)malloc( sizeof( struct thing_object) );
		p1 = (char *)to;
		while(cnt--)
			*p1++ = *p++;

		DbFree( i);
		DbWrite(i, (struct object *)to);
		Db(i)->ver = VERT;
#ifdef VM_DB
		free((FREE_TYPE) to);
#endif
		break;
	}

}

void re_copy_object(i)
dbref i;
{       register int cnt = sizeof( struct small_object);
	register char *p, *p1;
	struct object *o;
	struct player_object *po;
	struct room_object *ro;
	struct thing_object *to;
	struct exit_object *eo;

	o = Db(i);
	p = (char *)o;

	if( o->ver){
fprintf(logfile,"Object %d is not version 0 but %d\n", i, o->ver);
		return;
	}

	switch( Typeof(i) ){
	case TYPE_PLAYER:
		po = (struct player_object *)malloc( sizeof( struct player_object) );
		p1 = (char *)po;
		while(cnt--)
			*p1++ = *p++;
		p = (char *) Db(i)->ext;
		p1= (char *) &po->extp;
		cnt = sizeof( struct extension );
		while(cnt--)
			*p1++ = *p++;
		po->extp.pennys = Db(i)->pennies;

		free_locks(Db(i)->lock);
		Db(i)->lock = NULL;

		if( Db(i)->ext)
			free( (FREE_TYPE) Db(i)->ext);
		DbFree( i );
		DbWrite(i, (struct object *)po);
		Db(i)->ver = VERP;
#ifdef VM_DB
		free((FREE_TYPE) po);
#endif
		break;

	case TYPE_ROOM:
		ro = (struct room_object *)malloc( sizeof( struct room_object) );
		p1 = (char *)ro;
		while(cnt--)
			*p1++ = *p++;

		ro->zone = Db(i)->pennies;

		free_locks(Db(i)->lock);
		Db(i)->lock = NULL;
		if( Db(i)->ext)
			free( (FREE_TYPE) Db(i)->ext);
		DbFree(i );
		DbWrite(i, (struct object *)ro);
		Db(i)->ver = VERR;
#ifdef VM_DB
		free((FREE_TYPE) ro);
#endif
		break;

	case TYPE_EXIT:
		eo = (struct exit_object *)malloc( sizeof( struct exit_object) );
		p1 = (char *)eo;
		while(cnt--)
			*p1++ = *p++;

		need_lock(i, 0);
		eo->key = KeyN(i, 0);

		(* Db(i)->lock)[0] = NULL;
		free_locks(Db(i)->lock);
		Db(i)->lock = NULL;

		if( Db(i)->ext)
			free(  (FREE_TYPE) Db(i)->ext);
		DbFree( i );
		DbWrite(i, (struct object *)eo);
		Db(i)->ver = VERE;
#ifdef VM_DB
		free((FREE_TYPE) eo);
#endif
		break;

	case TYPE_THING:
		to = (struct thing_object *)malloc( sizeof( struct thing_object) );
		p1 = (char *)to;
		while(cnt--)
			*p1++ = *p++;

		free_locks(Db(i)->lock);
		Db(i)->lock = NULL;

		if( Db(i)->ext)
			free(  (FREE_TYPE) Db(i)->ext);
		DbFree( i);
		DbWrite(i, (struct object *)to);
		Db(i)->ver = VERT;
#ifdef VM_DB
		free((FREE_TYPE) to);
#endif
		break;
	}
}

void putref(f, ref, term)
FILE *f;
dbref ref;
char *term;
{
#ifdef EXTRACT
    if( ref > 0 && ref < extract_top && extracting){
	if( !extract_list[ref])
		ref = -5;
	else
		ref = extract_list[ref];
    }
#endif
    fprintf(f, "%d%s", ref, term);
}

void putint(f, ref, term)
FILE *f;
int ref;
char *term;
{
	fprintf(f, "%d%s", ref, term);
}

static void putstring(f, s)
FILE *f;
const char *s;
{   int n= 1023;
    while(s && *s && n--) {
	Putc(*s++, f);
    } 
    Putc('\n', f);
}
	
int db_write_object(f, i)
FILE *f;
dbref i;
{
    struct object *o;
	struct robot *r;
    char *nam;

#ifdef MSDOS
#ifndef _WINDOWS
fprintf(logfile,"%d\r", i);
#endif
#endif

    o = Db(i);
    nam = Name(i);
    if( !nam || !*nam){
fprintf(logfile,"db_write: object %d has a NULL name!\n", i);
	    putstring(f, "Broken name");                
    }else {
	    putstring(f, Name(i));
    }
    if( Typeof(i) != TYPE_EXIT){
	    putstring(f, Desc(i) );
    }else {
	fprintf(f,"\n");
    }
    putref(f, o->location,";");
    putref(f, o->contents,";");
    putref(f, -1, ";");
    putref(f, o->next,"\n");

    if( Typeof(i) == TYPE_EXIT){
	if(Key(i) ){
		putboolexp(f, Key(i), i);
	}else
		Putc('\n', f);
    }else{
	Putc('\n', f);
    }
    putref(f, o->owner,";");

    if( Typeof(i) == TYPE_ROOM)
	putref(f, Zone(i),";");
    else if( Typeof(i) == TYPE_PLAYER){
#ifdef MSDOS
	fprintf(f, "%ld;", Pennies(i));
#else
	putint(f, Pennies(i),";");
#endif
    }else{
	putint(f, 0, ";");
    }

    putint(f, o->flags, "\n");

    if( Typeof(i) == TYPE_PLAYER ){
	putstring(f, Ext(i).password);
    }else {
	Putc('\n', f);
    }

/* was commands */
	putref(f, NOTHING,";");

    if( o->class != NOTHING && o->class){
	if( o->class > 0 && o->class == default_room_class)
		putref(f, DEF_ROOM, "\n");
	else if( o->class > 0 && o->class == default_player_class)
		putref(f, DEF_PLAYER, "\n");
	else if( o->class > 0 && o->class == default_thing_class)
		putref(f, DEF_THING, "\n");
	else {
		putref(f, o->class,"\n");
	}
    }else
	putref(f, NOTHING,"\n");


    fprintf(f, "!%ld\n", o->age);

    if( Typeof(i)== TYPE_PLAYER && (r= Ext(i).robot) && (o->flags & ROBOT)){

	fprintf(f, "!%s %s %d %d %d %d %d %d %d %d \"%s\" %d\n",
		Name(i),r->pass,r->speed,r->fight_p,0,0,
		0, r->move_p, r->get_p, r->drop_p, 
		r->comment, 0);
    }

    if( Typeof(i) == TYPE_PLAYER){
#ifdef MSDOS
		fprintf(f, "!!%d\n%ld\n%d\n%d\n", 0, Pennies(i), Level(i), Chp(i));
#else
		fprintf(f, "!!%d\n%d\n%d\n%d\n", 0, Pennies(i), Level(i), Chp(i) );
#endif
   }

#ifdef MSDOS
	fprintf(f, "!>%ld\n", Position(i));
#else
	fprintf(f, "!>%d\n", Position(i));
#endif

    return 0;
}

#ifdef EXTRACT
dbref ex_remap(x)
dbref x;
{       if(extracting){
		if( x > 0 && x < extract_top){
			return extract_list[x];
		}
		return NOTHING;
	}
	return x;
}
#else
#define ex_remap(x) x
#endif

dbref db_write(f)
FILE *f;
{
    dbref i;

	fprintf(f, "#v1\n");
	fprintf(f, "#d r %d\n", ex_remap(default_room_class));
	fprintf(f, "#d t %d\n", ex_remap(default_thing_class));
	fprintf(f, "#d p %d\n", ex_remap(default_player_class));
	fprintf(f, "#d s %d\n", ex_remap(default_player_start));
	fprintf(f, "#d w %d\n", ex_remap(default_wizard));
	fprintf(f, "#d b %d\n", ex_remap(default_beaker));
	fprintf(f, "#d e %d\n", ex_remap(default_temple_class));

    for(i = 0; i < db_top; i++) {
#ifdef EXTRACT
	if( extracting && i){
		if( extract_list[i] == NOTHING || i >= extract_top)
			continue;
		fprintf(f,"#%d\n", extract_list[i]);
	}else
#endif
	{
		fprintf(f, "#%d\n", i);
	}
	db_write_object(f, i);
    }
    fputs("***END OF DUMP***\n", f);
    fflush(f);

#ifdef EXTRACT
    extracting = 0;
#endif
    return(db_top);
}

dbref parse_dbref(s)
const char *s;
{
    const char *p;
    long x;
    dbref d;

    x = atol(s);
    if(x > 0) {
	d = x;
	return d;
    } else if(x == 0) {
	/* check for 0 */
	for(p = s; *p; p++) {
	    if(*p == '0') return 0;
	    if(!isspace(*p)) break;
	}
    }else if(x == DEF_PLAYER || x == DEF_ROOM || x == DEF_THING){
	d = x;
	return d;
    }

    /* else x < 0 or s != 0 */
    return NOTHING;
}

/* Allow ; to terminate the dbref value */
static char *ptr;
	    
dbref getref(f)
FILE *f;
{       char *p, *p1;

	if( ptr == NULL){
		fgets(msgbuf, 2048, f);
		ptr = msgbuf;
	}
/* skip white space */
	p = ptr;
	while( *p == ' ' || *p == '\011')p++;
/* Look for ; seperator */
	p1 = p;
	while( *p1 && *p1 != ';')p1++;
	if( *p1 == ';'){        /* found it */
		*p1++ = '\0';
		ptr = p1;
	}else{
		ptr = NULL;     /* Not found, so need a new buffer next time*/
	}

	return((dbref )atoi(p));
}

long getlref(f)
FILE *f;
{       char *p, *p1;

	if( ptr == NULL){
		fgets(msgbuf, 2048, f);
		ptr = msgbuf;
	}
/* skip white space */
	p = ptr;
	while( *p == ' ' || *p == '\011')p++;
/* Look for ; seperator */
	p1 = p;
	while( *p1 && *p1 != ';')p1++;
	if( *p1 == ';'){        /* found it */
		*p1++ = '\0';
		ptr = p1;
	}else{
		ptr = NULL;     /* Not found, so need a new buffer next time*/
	}
    return(atol(p));
}

static const char *getstring(f)
FILE *f;
{
    char *p;

    fgets(msgbuf, 4000, f);
    for(p = msgbuf; *p; p++) {
	if(*p == '\n') {
	    *p = '\0';
	    break;
	}
    }

    if(!*msgbuf) {
	return(NULL);
    } 
    p = alloc_string(msgbuf);
    return(p);
}

void free_boolexp(b)
BOOLEXP_PTR b;
{
    if(b != TRUE_BOOLEXP) {
	switch( NT_T1(b) ){
	case NT_PTR:
		free_boolexp(PTR1(b));
		break;
	case NT_STR:
		free_string(vm_bool(b)->u1.s);
		break;
	}
	switch( NT_T2(b) ){
	case NT_PTR:
		free_boolexp(PTR2(b));
		break;
	case NT_STR:
		free_string(vm_bool(b)->u2.s);
		break;
	}
	bool_free(b);
    }
}

void free_locks(l2)
struct locking *(*l2)[];
{       int n;
	struct locking *l;
	if(!l2)return;

	for(n = 0 ; n < 5; n++,l++){
		l = (*l2)[n];
		if( l){
			if( l->key)free_boolexp( l->key);
			l->key = NULL;
			freemsg(  (FREE_TYPE)  l,"Free locks-1");
			(*l2)[n] = NULL;
		}
	}
	freemsg(  (FREE_TYPE) l2, "Free locks-2");
}

void clean_object(o)
dbref o;
{
	if( bad_dbref(o,0)){
		return;
	}
	if( Db(o)->ver == VERD){
		free_locks(Db(o)->lock);
		Db(o)->lock = NULL;
	}else if( Typeof(o)== TYPE_EXIT){
		if( Key(o) != TRUE_BOOLEXP)
			free_boolexp(Key(o));
		Key(o) = TRUE_BOOLEXP;
	}else if( Typeof(o) == TYPE_PLAYER){
		if( Ext(o).robot){
			free((FREE_TYPE) Ext(o).robot);
			Ext(o).robot = NULL;
		}
	}
#ifndef VM_DESC
	free_string(Db(o)->description);
#endif
	Db(o)->description = NULL;
	Db(o)->owner = 0;
}



void free_obj(o)
struct object *o;
{
	struct locking *(*l)[];
	struct extension *e;

	if(!o)return;
#ifndef VM_DESC
	if(o->name) 
		free_string( (FREE_TYPE)  o->name);
	if(o->description) 
		free( (FREE_TYPE)  o->description);
#endif
	o->name = NULL;
	o->description = NULL;
	if(o->ver == 0){
		l = o->lock;
		o->lock = NULL;
		free_locks(l);
	}else {
		if( o->ver == 4){
			if( ((struct exit_object *)o)->key != TRUE_BOOLEXP){
				free_boolexp( ((struct exit_object *)o)->key);
			}
			((struct exit_object *)o)->key = TRUE_BOOLEXP;
		}
	}

	if(o->ver == 0){
		e = o->ext;
		o->ext = NULL;
		if( e){
			if(e->password) 
				free( (FREE_TYPE)  e->password);
			if(e->robot)
				free(  (FREE_TYPE) e->robot);
			free( (FREE_TYPE) e);
		}
#ifdef Notdef
		o->commands = NOTHING;
#endif
	}else if( (o->flags & TYPE_MASK) == TYPE_PLAYER){
		e = &( (struct player_object *)o)->extp;
		if(e->password) 
			free( (FREE_TYPE)  e->password);
		if(e->robot)
			free(  (FREE_TYPE) e->robot);
	}

	o->owner = NOTHING;
	o->location = 0;
	o->contents = NOTHING;
	o->next = 0;
	o->flags = 0;
	o->age = 0l;
}

void db_free()
{
#ifndef VM_DB
    dbref i;

    if(db) {
	for(i = 0; i < db_top; i++) {
		free_obj( Db(i) );
		DbFree( i);
	}
	free( (FREE_TYPE)  db);
	db = 0;
    }
#endif
    db_top = 0;
}

extern BOOLEXP_PTR set_succ_fail();

void parse_lock(f,i,nlocks)
FILE *f;
dbref i;
int nlocks;
{       char ch;
	struct locking *l;
	char *fail_message, *succ_message, *ofail, *osuccess;
	BOOLEXP_PTR b;

	ch = getc(f);
	while(ch == ' ')
		ch = getc(f);
	ungetc(ch, f);
	need_lock(i, nlocks);
	l = has_lock(i, nlocks);
	l->key = getboolexp(f);

	if( newdbversion == 0){
		fail_message = (char *) getstring(f);
		succ_message = (char *) getstring(f);
		ofail = (char *) getstring(f);
		osuccess = (char *) getstring(f);
#ifdef VM_DESC
		free_string(fail_message);
		free_string(succ_message);
		free_string(ofail);
		free_string(osuccess);
#else
		if( (fail_message && *fail_message) ||
		    (ofail && *ofail) ||
		    (succ_message && *succ_message) ||
		    (osuccess && *osuccess) ){
			if( l->key == TRUE_BOOLEXP){
				l->key = new_boolexp();
				b = l->key;
				vm_bool(b)->n_func = BOOLEXP_TRUE;
			}
			l-> key = set_succ_fail(1, l->key, succ_message, osuccess,
						 fail_message, ofail);
		}
#endif
	}
}


void parse_nlocks(f, i)
FILE *f;
dbref i;
{       char ch;
	int nlocks = 0;

	ch = getc(f);
	if(ch >= '0' && ch <= '9')
		nlocks = ch-'0';
	else{
fprintf(logfile,"db: bad parse in get extra locks %c\n",ch);
		return;
	}
	parse_lock(f, i, nlocks);
}

int parse_pz(f, d)
FILE *f;
dbref d;
{       char ch;
	dbref world;
	money pennies;
	int lev, chp;
	int lev1, chp1;
	struct object *o;

	o =Db(d);

	lev1 = -1;
	chp1 = 31000;

	ch = getc(f);

	while( ch == '!'){

		world = getref(f);
		pennies = getlref(f);

		ch = getc(f);
		ungetc(ch,f);

		chp = 31000;
		lev = -1;

		if(ch != '!' && ch != '#'){
			lev = getref(f);
			ch = getc(f);
			ungetc(ch,f);
			if(ch != '!' && ch != '#'){
				chp = getref(f);
			}
		}
		if( world == 0){
			if( o->ext){
				lev1 = lev;
				chp1 = chp;
			}
		}
		ch = getc(f);
		if( ch != '!'){
			break;
		}
		ch = getc(f);
		if( ch != '!'){
			ungetc(ch ,f);
			ch = '!';
			break;
		}
		
	}
	if( reset_money){
		lev1 = 0;
		chp1 = 20;
	}
	o->ext->level = lev1;
	o->ext->chp = chp1;
	return ch;
}

#define GETAREF(x) o->x = getref(f);\
	    if( o->x >= 0)\
		o->x += k;

#define GETAREF2(x) o->x = getref(f);\
	    if( o->x > 0)\
		o->x += k;


/* NOTE. db_read_one
 * This is not as simple as you might think. It assumes a
 * pre-PRISM database format first. If there is a version
 * specifier then it does what is needed.
 * 
 * After the initial # can be a v followed by version number.
 * version 1 databases do not have SUCC,FAIL,OSUCC or OFAIL fields.
 *
 * After the database has been read it is then post converted
 * to the new types if required.
*/

dbref db_read_one(f, i, k)
FILE *f;
dbref i;
int k;
{
    struct object *o;
    const char *end;
    char ch;
    int j,nlocks;
    struct locking *l;
    struct extension *e;
    char *robp;
    dbref exits = NOTHING;
    struct exit_rec *el;
    char *p;

    db_base = k;

	switch( ch = getc(f)) {
	  case '#':
	    while( i == 0){     /* Only first time around */
		ch = getc(f);           /* Look for version string */
		if( ch == 'v'){
			newdbversion = getref(f);
		}else if( ch == 'd'){   /* data line */
			while( ch = getc(f))
				if( ch != ' ')
					break;
			switch(ch){
			case 'b':
				if( !merging){
					default_beaker = getref(f);
					if( default_beaker > 0)
						default_beaker += k;
				}
				break;
			case 'e':
				if( !merging){
					default_temple_class = getref(f);
					if( default_temple_class > 0)
						default_temple_class += k;
				}
				break;
			case 'r':
				loc_room = getref(f);
				if( loc_room > 0)
					loc_room += k;
				if( !merging){
					default_room_class = loc_room;
				}
				break;
			case 'p':
				loc_player = getref(f);
				if( loc_player > 0)
					loc_player += k;
				if( !merging){
					default_player_class = loc_player;
				}
				break;
			case 't':
				loc_thing = getref(f);
				if( loc_thing > 0)
					loc_thing += k;
				if( !merging){
					default_thing_class = loc_thing;
				}
				break;
			case 's':
				if( !merging){
					default_player_start = getref(f);
					if( default_player_start > 0)
						default_player_start += k;
				}
				break;
			case 'w':
				if( !merging){
					default_wizard = getref(f);
					if( default_wizard > 0)
						default_wizard += k;
				}
			}
		}else if( ch == '#'){   /* comment */
		}else {
			ungetc(ch, f);
			break;
		}
/* Skip till start of next bit. */
		while(1){
			ch = getc(f);
			if(ch == EOF || ch == '\n' || ch == '#')
				break;
		}
		if( ch == '\n')
			ch = getc(f);
		if( ch != '#'){
fprintf(logfile,"Error processing database header, expecting '#' got '%c' (%2x)\n", ch, ch);
			break;
		}
	    }

	    /* another entry, yawn */
	    if(i != (j = getref(f))) {
		/* we blew it */
fprintf(logfile, "db_read_one: getref failed, expecting %d got %d\n", i, j);
		return -1;
	    }
#ifdef MSDOS
#ifndef _WINDOWS
fprintf(logfile,"%d \r", i+k);
#endif
#endif
	    /* make space */
	    if( db_top < i+k){
fprintf(logfile,"db_read_one:dbtop < i+k\n");
		    db_grow(i+1+k);
	    }else {
		if( i+k == db_top && i+k != new_object())
			fprintf(logfile,"db_read_one:i+k != new_object()\n");
	    }
	    
	    /* read it in */
	    o = Db(i+k);

	    o->name = getstring(f);
	    o->description = (char *) getstring(f);

	    GETAREF2(location)
	    GETAREF2(contents)  /* Don't translate 0's */
	    
	    exits = getref(f);
	    if(exits >= 0)
		exits += k;

	    if( exits >= 0){
		el = (struct exit_rec *)malloc(sizeof(struct exit_rec));
		if( el){
			el->next = exit_list;
			el->room = i+k;
			el->exit = exits;
			exit_list = el;
		}
	    }

	    GETAREF2(next)
	    o->lock = NULL;
	    need_lock((dbref)(i+k), 0);
	    parse_lock(f, i+k, 0);

	    GETAREF(owner);
#ifdef MSDOS
	    o->pennies = getlref(f);
#else
	    o->pennies = getref(f);
#endif
	    o->flags = getref(f);
if(ptr){
fprintf(logfile,"db: ptr != NULL :%s\n", ptr);
}

	    if( reset_money && (o->flags & TYPE_MASK)==TYPE_PLAYER &&
		!(o->flags&ROBOT)){
		o->pennies = 0;
	    }
	    if( (o->flags & TYPE_MASK) == TYPE_ROOM && o->pennies){
		o->pennies += k;
	    }

	    o->ext = NULL;
	    if( Typeof(i+k) == TYPE_PLAYER){
		    need_ext((dbref)(i+k));
		    e = o->ext;
		    e->password = (char *)getstring(f);
		    if( !e->password)
			e->password = (char *)alloc_string(" .......");
		    e->robstr = NULL;
	    }else {
		if( p = getstring(f) ){
fprintf(logfile,"Password on non player %s(%d)\n(%s)\n", Name(i+k), i+k, p);
		}
	    }
#ifdef Notdef
	    o->commands = NOTHING;
#endif

	    ch = getc(f);
	    if( ch != '!' && ch != '#' && ch != '*'){
		ungetc(ch, f);
		getref(f);
#ifdef Notdef
		GETAREF(commands);
#endif
		if( ptr){
			ch = *ptr++;
		}else {
			ch = getc(f);
		}
	    }

	    o->class = NOTHING;

	    if( ch != '!' && ch != '#' && ch != '*'){
		if(ptr){
			ptr--;
		}else {
			ungetc(ch, f);
		}
		GETAREF(class);
if(ptr){
fprintf(logfile,"db: ptr != NULL after class :%s\n", ptr);
}
		if( loc_player > 0 && o->class == loc_player){
			o->class = DEF_PLAYER;
		}else if( loc_room > 0 && o->class == loc_room){
			o->class = DEF_ROOM;
		}else if( loc_thing > 0 && o->class == loc_thing){
			o->class = DEF_THING;
		}
		ch = getc(f);
	    }

	    if( ch == '!'){
		o->age = getlref(f);
	    }else
		ungetc(ch, f);


/* Look for ROBOT info */
	    ch = getc(f);
	    if( ch == '!'){
		ch = getc(f);
		if(ch != '/' && ch != '!' && ch != '>' ){
			ungetc(ch,f);
			robp = (char *)getstring(f);
			if(robp){
				Db(i+k)->ext->robstr = robp;
			}
			ch = getc(f);
		}else {
			ungetc(ch,f);
			ch = '!';
		}
	    }else{
		if( Typeof(i+k) == TYPE_PLAYER && (Flags(i+k)&ROBOT) ){
			Flags(i+k) &= ~ROBOT;
fprintf(logfile,"Cleared Robot flag on player %s(%d)\n", Name(i+k), i+k);
		}
	    }

/* 3-Feb-93 Position stuff */
	o->position = 0;

 /* After ROBOT info */
	while( ch == '!'){
		ch = getc(f);           /* look at next char */

		if(ch == '/'){
			parse_nlocks(f, i+k);
			ch = getc(f);           /* get first char of next line */
		}else if( ch == '!'){   /* Penny zone info */
			ungetc(ch, f);
			ch = parse_pz(f, i+k);
		}
/* 3-Feb-93 Position stuff */
		else{
			if( ch != '>'){
				ungetc(ch, f);
			}
			o->position = getlref(f);
			ch = getc(f);           /* get first char of next line */
		}
	}
	ungetc(ch,f);

/* see if l is needed */
	    for(nlocks=0; nlocks < 5; nlocks++){
		    l = has_lock((dbref)(i+k), nlocks);
		    if( l && (l->key) ){
			break;
		    }
	    }
	    if(nlocks == 5){ /* not being used at all */
		free_locks( o->lock);
		o->lock = NULL;
	    }

	    if( dbversion){
		if( o->flags & ANTILOCK){
fprintf(stderr,"Antilock on %s\n", o->name);
			o->flags |= CONTAINER;
			o->flags &= ~ANTILOCK;
		}
	    }
	    return i;

	  case '*':
	    end = getstring(f);
	    if(end && end[0] != '*' || end[1] != '*' || end[2] != 'E' ||
		end[3] != 'N' || end[4] != 'D') {
		free( (FREE_TYPE)  end);
fprintf(logfile, "**END OF DUMP*** Expected! Obj = %d\n", i);
		break;
	    } else {
		if(end)
			free( (FREE_TYPE)  end);
		else {
fprintf(logfile, "**END OF DUMP*** Not found! Obj = %d\n", i);
		}
		break;
	    }
	  default:
fprintf(logfile,"Error parsing '%c' %02x Obj = %d\n", ch, ch, i);
		i = 0;
		while( i++ < 100 && (ch = getc(f)) != EOF)
			fprintf(logfile,"%c", ch);
		return -2;
	}
    return -1;
}

/* Translate_player_locks 
 * Is in parsbool.c
 */

void db_post_load(start, finish)
dbref start,finish;
{       dbref i, j, l;
	long tim;
	char nambuf[100];
	struct exit_rec *el, *el2;
	char *n;                        /* Used to check that Name returns non NULL*/
fprintf(logfile,"Start post load\n");

/* Start the translate pass */
	if( dbversion == 0){
		for(el = exit_list; el ; el = el2){
			i = el->exit;
			j = el->room;
			l = Contents(j);
			if( Typeof(j) != TYPE_ROOM){
				if( Typeof(j) == TYPE_THING){
					trans_thing_home(j, i);
				}else{
					trans_player_home(j, i);
				}
			}else if(Typeof(i) != TYPE_EXIT){
fprintf(logfile,"Exit field in room %d points to non exit! (%d)\n", j, i);
			}else{
				Contents(j) = i;        /* Put first exit on contents list */
				while(i != NOTHING){
if( Typeof(i) != TYPE_EXIT){
fprintf(stderr,"Found non exit %d in exits list from room %d\n", i, j);
break;
}
					Contents(i) = Location(i);
					Location(i) = j;

					if( Next(i) != NOTHING){
						i = Next(i);
					}else {
						Next(i) = l;    /* Tag contents on end */
						i = NOTHING;
						l = NOTHING;
					}
				}
			}
			if( l != NOTHING){
				Contents(j) = l;
			}
			el2 = el->next;
			free( (FREE_TYPE) el);
		}
		exit_list = NULL;
fprintf(logfile,"Start translate pass - exits\n");
		for(i=start; i < finish; i++){
			if( Typeof(i) == TYPE_EXIT){
				translate_exit_locks(i);
			}
		}

fprintf(logfile,"Start translate rest.\n");

		for(i = start; i < finish; i++){
#ifdef MSDOS
#ifndef _WINDOWS
fprintf(logfile,"%d    \r", i);
#endif
#endif
			switch(Typeof(i)){
			case TYPE_PLAYER:
			case TYPE_THING:
#ifdef Notdef
				for(j = Commands(i); j != NOTHING; j=l){
					l = Next(j);
					PUSH(j, Db(i)->contents);
				}
				Commands(i) = NOTHING;
#endif
				if( Typeof(i)== TYPE_PLAYER)
					translate_player_locks(i);
				else
					translate_thing_locks(i);
				break;

			case TYPE_ROOM:
				translate_room(i);
				break;
			}
		}
	}

fprintf(logfile,"Re-copy\n");
	for(i=start ; i < finish; i++){
		re_copy_object(i);
	}

	if( dbversion == 0){
		dbversion = 1;
fprintf(logfile,"Translate classes\n");
		for(i=start ; i < finish; i++){
			switch(Typeof(i)){
			case TYPE_ROOM:
				if( !(Flags(i) & INHERIT) )
					translate_room_class(i); /* Only non class rooms */
				break;
			case TYPE_THING:
				translate_thing_class(i);
				break;
			case TYPE_PLAYER:
				translate_player_class(i);
				break;
			default:
				break;
			}
		}
	}
	dbversion = 1;

	for(i=start; i < finish; i++){
		if( Typeof(i) == TYPE_THING && is_rubbish(i)){
			if( Contents(i) != NOTHING)
fprintf(logfile,"Rubbish(%d) has contents (%d)\n", i, Contents(i));
		}
		n = Name(i);
		if( n == NULL){
sprintf(msgbuf,"NULL name in object %d\n", i);
			abort_msg(msgbuf);
			NameW(i, "Broken...");
		}
	}

fprintf(logfile,"Put player names in the tree\n");

	for(i=start ; i < finish; i++){
	    if(Typeof(i)== TYPE_PLAYER){
		if(!add_name_ok(Name(i), i) ){
			char new_name[80];
			sprintf(new_name,"X_%s", Name(i));
			NameW(i, new_name);
			if( !add_name_ok(new_name, i)){
fprintf(logfile,"Duplicate PLAYER name %s(%d)\n", Name(i), i);
				Flags(i) = TYPE_THING;
			}
#ifdef Notdef
		}else if( !ageof(i) || ageof(i) > (3600l*24*7*10)){
fprintf(logfile,"%s is more than 10 weeks old (%ld)\n", Name(i), ageof(i));
		}else if( !ageof(i) || ageof(i) > (3600l*24*7*5)){
fprintf(logfile,"%s is more than 5 weeks old (%ld)\n", Name(i), ageof(i));
#endif
		}
	    }
	}

fprintf(logfile,"Look for Autotoads.\n");
	for(i=start; i < finish; i++){
		if( Typeof(i) == TYPE_PLAYER){
			strip_player(i);
#ifdef AUTO_TOAD
			if( i > 4 && i != default_beaker){
				tim = ageof(i);
				if( tim > AUTO_TOAD_DAYS*24*3600l){
					sprintf(nambuf,"*%.80s",Name(i));
					if( !(Flags(i)&(WIZARD|ROBOT)) ){
						do_toad(1, nambuf);
						if( !find_refs(1, i)){
							make_rubbish(i);
							fprintf(logfile,"Recycled\n");
						}
					}
				}
			}
#endif
		}       /* if player */
	}

fprintf(logfile,"Look for robots to start\n");
	for(i=start; i < finish; i++){
		if( Typeof(i) == TYPE_PLAYER && (Flags(i) & ROBOT) ){
			if( Ext(i).robstr)
				setup_bot(i, Ext(i).robstr);
			else
				Flags(i) &= ~ROBOT;
		}
	}

fprintf(logfile,"End of post_load\n");

}

dbref db_read(f)
FILE *f;
{
    dbref i;
    dbref j;

	dbversion = 0;


    db_free();
    loc_room = loc_player = loc_thing = NOTHING;
    for(i = 0;; i++) {
	j = i;
	if( (i = db_read_one(f, i, 0)) < 0)
		break;
    }

	if( i == -2){
		abort_msg("DB read parse error");
	}else {
#ifdef VM_DESC
	fclose(descf);
#ifdef MSDOS
	descf = fopen(desc_name, "rb+");
#else
	descf = fopen(desc_name, "r+");
#endif
	if(!descf){
fprintf(logfile,"Could not re-open %s\n", desc_name);
		abort();
	}else
fprintf(logfile,"Re-opened %s\n",desc_name);
#endif

	dbversion = newdbversion;

    db_post_load(0, j);         /* Do the post db_load stuff */
fprintf(logfile,"Default classes: %d %d %d\n", default_room_class, default_thing_class, default_player_class);
    return j;
    }
    return -1;
}
		
void do_merge(player, arg1, arg2)
dbref player;
char *arg1,*arg2;
{       FILE *f;
	dbref i,j;

	newdbversion = 0;
	dbversion = 0;

	if(!is_god(player)){
		notify(player,"You cannot @merge!");
		return;
	}
	f = fopen( arg1, "r");
	if(!f){
		notify(player, "Could not open the file!");
		return;
	}
	j = db_top;
	merging = 1;
	loc_room = loc_player = loc_thing = NOTHING;
	for(i = 0;; i++) {
		if( (i = db_read_one(f, i, j)) == -1)
			break;
	}
	fclose(f);
	merging = 0;
	sprintf(msgbuf,"New db_top is %d", db_top);
	notify(player, msgbuf);
	
#ifdef VM_DESC
	fclose(descf);
#ifdef MSDOS
	descf = fopen(desc_name, "rb+");
#else
	descf = fopen(desc_name, "r+");
#endif
	if(!descf){
fprintf(logfile,"Could not re-open %s\n", desc_name);
		abort();
	}else
fprintf(logfile,"Re-opened %s\n",desc_name);
#endif
	vm_reopen();

	dbversion = newdbversion;

	db_post_load(j, db_top);
	db_base = 0;
	
}

void version(obj, ver, msg)
dbref obj;
int ver;
char *msg;
{
	if( bad_dbref(obj, 0)){
fprintf(logfile,"Bad object ref (%d) for %s\n", obj, msg);
		return;
	}
	if( Db(obj)->ver != ver){
		fprintf(logfile,"Bad version on (%d) for %s. Is %d not %d\n", obj, msg, Db(obj)->ver, ver);
	}
}

#ifdef MSDOS
#ifndef _WINDOWS
void do_edit(player, obj)
dbref player;
char *obj;
{       FILE *editf;
	dbref thing;
	int error;
	struct object *db2;

	newdbversion = 1;

	if( !ArchWizard(player)){
		denied();
		return;
	}
	init_match(player, obj, NOTYPE);
	match_everything();
	thing = noisy_match_result();

	if( bad_dbref(thing, 0)){
		notify(player, "Can't edit that");
		return;
	}

	editf = fopen("EDIT.TMP", "w");
	if( !editf){
		notify(player, "Could not open EDIT.TMP");
		return;
	}
	fprintf(editf, "#%d\n", db_top);
	db_write_object(editf, thing);
	fclose(editf);
#ifndef MSC
	error = forklp("ws", "ws", "edit.tmp", NULL);
	if( !error)
		error = wait();
#else
	error = spawnlp(P_WAIT, "command", "command", "/c", "ws", "edit.tmp", NULL);
#endif
	if( error)
		perror("@edit");
	editf = fopen("EDIT.TMP", "r");
	if( !editf){
		notify(player, "Could not open EDIT.TMP for reading");
		return;
	}
fprintf(logfile,"Db_top = %d\n", db_top);
	if( 0 <= db_read_one(editf, db_top, 0)){
fprintf(logfile,"After db_read_one = %d\n", db_top);
		re_copy_object(db_top-1);
		db2 = Db(db_top-1);
		DbWrite(db_top-1, Db(thing));
		DbWrite(thing, db2);
		notify(player, "Edited.");
	}else
		notify(player, "Failed.");
	fclose(editf);

	return;
}
#endif
#endif


#ifdef TEST_MALLOC
#ifdef MSDOS
long mem_malloced = 0l;
long mem_malloccnt = 0l;
#else
int mem_malloced = 0l;
int mem_malloccnt = 0l;
#endif

#undef malloc
#undef realloc
#undef free
#ifndef MSDOS
extern char *malloc(int );
extern char *realloc(FREE_TYPE, int);
extern void free( FREE_TYPE);
#endif

struct mem_header {
int len, check;
char data;
};


char *dosmalloc( siz)
int siz;
{       char *p;
	struct mem_header *p1;

	p1 = (struct mem_header *)malloc( siz+Sizeof(struct mem_header));
	if( !p1)
		return NULL;
	p1->len = siz;
	p1->check = -siz;
	p = &(p1->data);
	mem_malloced = mem_malloced+siz;
	mem_malloccnt++;
	return p;
}

void dosfreemsg(p, msg)
char *p;
char *msg;
{       int cnt;
	struct mem_header *p1;

	if( !p){
fprintf(logfile,"\nDosfree(NULL)\n");
		return;
	}
	p1 = (struct mem_header *) &(p[- (2* Sizeof(int) )]);

	if( p1->len + p1->check != 0 || !p1->len ){
fprintf(logfile,"Dosfree(%lx,%s) - Not allocated!\n", p, msg);
#ifndef MSDOS
		abort();
#endif
		return;
	}
	cnt = p1->len;

	p1->len = 0xaa55;
	p1->check = 0;
	free(p1);
	mem_malloced = mem_malloced-cnt;
	mem_malloccnt--;
}

void dosfree( x)
char *x;
{
	dosfreemsg((char *)x, "Unknown");
}

char *dosrealloc(x, cnt)
char *x;
int cnt;
{       unsigned siz;
	char *p;
	struct mem_header *p1;

	if( !x){
fprintf(logfile,"\nDosrealloc(NULL)\n");
		return NULL;
	}
	p1 = (struct mem_header *)& (x[-2 * Sizeof(int)]);

	if( p1->len + p1->check != 0 || !p1->len ){
fprintf(logfile,"Dosrealloc(%lx,%d) - Not allocated!\n", x, cnt);
#ifndef MSDOS
		abort();
#endif
		return NULL;
	}
	siz = p1->len;
	p1->len = 0xaa55;
	p1->check = 0;
	mem_malloced = mem_malloced - siz;

	p1 = (struct mem_header *)realloc(p1, cnt+Sizeof(struct mem_header ));
	if( !p1)
		return NULL;

	p1->len = cnt;
	p1->check = -cnt;
	p = &(p1->data);
	mem_malloced = mem_malloced+cnt;
	mem_malloccnt++;
	return p;
}

#endif
