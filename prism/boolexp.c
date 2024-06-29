/* Cause boolexp_table to be defined */

#define BTABLE 1

#define TBUFSIZ 1019

#include "copyrite.h"

#include <ctype.h>

#include "db.h"
#include "match.h"
#include "interfac.h"
#include "fight.h"

#include "boolexp.h"

#include "config.h"
#include "externs.h"

extern dbref euid;


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


int boolexp_depth;	/* if non zero then we are in an EXEC */
dbref euid = NOTHING;	/* effective UID */
int last_result;	/* used by EXEC to return a result */
static int db_flag = 0;
static dbref b_player;	/* The player the lock is being evaluated for */
static dbref b_here = NOTHING;	/* Where the player is when the lock is evaluated */
static char *failure;	/* Failure reason (NOTHING/AMBIGUOUS) */

extern dbref default_beaker;

extern int xargc;
extern char **xargv;

extern int bad_dbref( PROTO2(dbref, dbref));
struct bool_rec *last_bool_rec;

extern char *ctime();
extern char *name_anon();
static char *tempbuf;

extern dbref created_object;
extern long eval_bool_count;
extern char *prt_money(PROTO(money));
extern int depth;		/* Used by EXEC's and SEND's */

/* 14/2/93 */
dbref this_exit;		/* hold the number of the current xit. only
				   used in xpand...*/

extern char *Name0( PROTO(dbref));

/* Memory allocation on demand test routine */

int chk_tempbuf()
{
	if( !tempbuf){
		tempbuf = (char *)malloc(TBUFSIZ+5);
		if( !tempbuf){
			fprintf(logfile,"CHK_TEMPBUF: No memory available!\n");
			return 1;
		}
	}
	return 0;
}


/* Media stuff */
long UpdatePosition(pos, sub, add)
long pos, sub, add;
{	long x,y,z;
	long mx, my, mz;
	
	mx = 0x3ffl;
	my = mx << 10;
	mz = my << 10;

	x = (pos & mx) - (sub & mx) + (add & mx);
	y = (pos & my) - (sub & my) + (add & my);
	z = (pos & mz) - (sub & mz) + (add & mz);

	pos = (x & mx) | (y & my) | (z & mz);
	return pos;
}


int Within(pos, sub, add)
long pos, sub, add;
{	int x,i,j;
/* check x */
	x = pos & 0x3ff;
	i = sub & 0x3ff;
	j = add & 0x3ff;

	if( i < j){
		if(x < i || x > j){
			return 0;
		}
	}else
		if( x > i || x < j)return 0;
	pos = pos >> 10;
	sub = sub >> 10;
	add = add >> 10;
/* check y */	
	x = pos & 0x3ff;
	i = sub & 0x3ff;
	j = add & 0x3ff;

	if( i < j ){
		if(x < i || x > j){
			return 0;
		}
	}else
		if( x > i || x < j)return 0;
	pos = pos >> 10;
	sub = sub >> 10;
	add = add >> 10;
/* check z */
	x = pos & 0x3ff;
	i = sub & 0x3ff;
	j = add & 0x3ff;
	
	if( i < j ){
		if(x < i || x > j){
			return 0;
		}
	}else
		if( x > i || x < j)return 0;
	pos = pos >> 10;
	sub = sub >> 10;
	add = add >> 10;

	return 1;
}


/*
 *	This routine checks to see that the Basic Authorisation 
 *	checks are TRUE.
 */

int basic_auth_check(exit, victim, msg)
dbref exit, victim;
char *msg;
{	dbref auth_owner = NOTHING;
	dbref victim_loc = NOTHING;

	if( bad_dbref(exit, 0)){
fprintf(logfile, "basic_auth_check: bad exit number %d\n", exit);
		return 0;
	}
	if( bad_dbref(victim, 0)){
fprintf(logfile, "basic_auth_check: bad victim number %d\n", victim);
		return 0;
	}

	auth_owner = Owner(exit);
	if( Typeof(victim) != TYPE_ROOM){
		victim_loc = Location(victim);
	}else {
		victim_loc = victim;
	}

	if( Typeof(victim) == TYPE_PLAYER){
		if( is_god( victim)){
fprintf(logfile, "basic_auth_check: exit %d tried to %s on a God\n", exit, msg);
			return 0;		
		}
		if( TrueWizard(victim) && !TrueWizard(auth_owner)){
fprintf(logfile, "basic_auth_check: exit %d(%d) tried to %s on a wizard %s(%d)\n", exit, auth_owner, msg, Name(victim), victim);
			return 0;
		}

	}

	if( bad_dbref(victim_loc, 0)){
fprintf(logfile,"basic_auth_check: bad victim loc(%d)\n", victim_loc);
		return 0;		/* Can't find owner of here...*/
	}


	if( Owner(victim_loc) == auth_owner)
		return 1;		/* Ok */

	if( TrueWizard(auth_owner))
		return 1;
	return 0;	/* Not ok */
}

int logged_in_or_robot(player)
dbref player;
{
	if( Flags(player)&ROBOT)
		return 1;
	return logged_in(player);
}

void set_error(msg)
char *msg;
{
	failure = msg;
}

char **save_xargs(sargc, l)
int sargc;
struct dbref_chain **l;
{	char **sargv;
	int j;
	struct dbref_chain **l1 = l;

	if( l )
		*l = NULL;

	sargv = (char **)malloc( (sargc+2)*Sizeof(char *));
	if(!sargv)
		return NULL;
	for(j=0; j <= sargc; j++){
		sargv[j] = (char *)alloc_string(xargv[j]);
		if( l1 ){
			if( xargl(j)){
				*l1 = add_chain(NULL, xargl(j)->this);
			}else {
				*l1 = add_chain(NULL, 0);
				(*l1)->this = NOTHING;
			}
			l1 = &(*l1)->next;
		}
	}
	sargv[sargc+1] = NULL;
	return sargv;
}

void restore_xargs(sargc, sargv, sargl)
int sargc;
char **sargv;
struct dbref_chain *sargl;
{
	int i;

	if( sargc){
		setupxargs(sargc, sargv, sargl);

		for(i=0; i < sargc; i++){
			free_string( sargv[i]);
		}
		if( sargv){
			freemsg( (FREE_TYPE)  sargv, "restore_xargs");
			sargv = NULL;
		}
	}else {
fprintf(logfile,"restore: sargc == 0\n");
	}
}

/* This is a bit complicated due to the need to produce a 
 * consistant result. We try to make a list of possible 
 * candidates. If there are exact matches we keep them only,
 * if partial matches, then keep them.
 */

dbref find_arg( b, container)
int b;
dbref container;
{	char *p;
	struct dbref_chain *ch, **chl;

	if( b >= xargc)
		return NOTHING;
	if( b < 0)
		return NOTHING;

	ch = xargl(b);
	if( ch){
		return ch->this;	/* Any will Do */
	}

/* strip blanks */
	p = xargv[b];
	while(p && *p == ' ')p++;
	xargv[b] = p;

	while(*p)p++;
	while(p != xargv[b]){
		if( p[-1] == ' '){
			p--;
			*p = '\0';
		}else break;
	}
	p = xargv[b];

/* look it up */

	xargl(b) = make_chain( Contents(container));
	ch = xargl(b);
	if( !ch){
		xargl(b) = make_chain( Contents( loc_of(container)) );
	}else {
		while( ch->next )ch = ch->next;
		ch->next = make_chain( Contents( loc_of(container)) );
	}
/* Basic prune */

	for( chl = &(xargl(b)); ch = *chl;){
		if( Typeof(ch->this) == TYPE_EXIT){
			*chl = ch->next;

			ch->next = NULL;
			free_chain(ch);
		}else
			chl = &(ch->next);
	}
	for( ch = xargl(b); ch ; ch = ch->next){
		if( !string_compare(p, Name0(ch->this)) ) {
			break;		/* Found an exact match */
		}
	}
	if( ch ){/* Keep only exact matches */
		for( chl = &(xargl(b)); ch = *chl; ){
			if( string_compare( p, Name0(ch->this))){
				*chl = ch->next;
				ch->next = NULL;
				free_chain(ch);
			}else
				chl = &(ch->next);
		}
	}else {
/* No exact matches so Keep partial matches */
		for( chl = &(xargl(b)); ch = *chl; ){
			if( !string_match(  Name0(ch->this), p)){
				*chl = ch->next;
				ch->next = NULL;
				free_chain(ch);
			}else
				chl = &(ch->next);
		}
	}
	
	ch = xargl(b);
	if( ch )
		return ch->this;
	return NOTHING;
}

/* Given a list of candidates, do a bit more pruning. */

dbref prune_chain(n, obj, func)
int n;
dbref obj;
int func;
{	struct dbref_chain *ch, **chl;

	for( chl = &(xargl(n)); ch = *chl; ){
		if( ch->this == NOTHING)
			goto done;

		if( member(ch->this, Contents(obj)) ){
			if( func == BOOLEXP_NOT){	/* throw this ? */
				*chl = ch->next;
				ch->next = NULL;
				free_chain(ch);
			}else
				chl = &(ch->next);
		}else{
			if( func != BOOLEXP_NOT){
				*chl = ch->next;
				ch->next = NULL;
				free_chain(ch);
			}else
				chl = &(ch->next);
		}
	}
done:
	ch = xargl(n);
	if(ch )
		return ch->this;
	return NOTHING;
}

void print_args(player)
dbref player;
{
	notify(player,"Boolexp function argument specials");
	notify(player,"===============================================");
	notify(player,"SELF ....... The receiver of the message");
	notify(player,"SENDER ..... The sender of the message");
	notify(player,"LOC ........ Location of receiver");
	notify(player,"HERE ....... Room that the receiver is in");
	notify(player,"$0 ......... Receiver");
	notify(player,"$1 ......... What matched the 1st * or # pattern");
	notify(player,"$2 ......... What matched the 2nd * or # pattern");
	notify(player,"In string expansions");
	notify(player,"$[#]0, $[#]1, $[#]2, $[#]3 as above.");
	notify(player,"$[#]s ...... Sender");
	notify(player,"$[#]L ...... Name of HERE");
	notify(player,"$[#]l ...... Number of HERE");
	notify(player,"$c ......... Receivers Class");
	notify(player,"$d ......... Todays date");
	notify(player,"$t ......... Time of day");
	notify(player,"$x ......... The exit that matched");
	notify(player,"===============================================");
	notify(player,"[#] $#0 .... Print #num of $0 instead of string");
}

void apply_func(b, f)
BOOLEXP_PTR b;
void (*f)();
{
	while( b != TRUE_BOOLEXP){
		if( NT_T1(b) == NT_PTR)
			apply_func(vm_bool(b)->u1.p, f);
		(*f)(b);
		if( NT_T2(b) == NT_PTR){
			b = vm_bool(b)->u2.p;
		}else break;
	}
}

dbref decode_obj(t, n)
int t;
union new_boolobj *n;
{	dbref	res = NOTHING;
	int sargc;
	char **sargv;
	struct dbref_chain *sargl;


	switch( t){
	case NT_DBREF:
		res = n->d;
		break;
	case NT_ME:
		res = b_player;
		break;
	case NT_ARG:
		res = n->i;
		if( res <= 0){
			res = receiver;
		}else{
			res = find_arg( res, receiver);
		}
		break;

	case NT_HERE:
		res = b_here;
		break;
	case NT_LOC:
		res = Location(receiver);
		break;
	case NT_RECEIVER:
		res = receiver;
		break;
	case NT_SENDER:
		res = sender;
		break;
	case NT_INT:
		res = n->l;
		break;
	case NT_STR:
		created_object = NOTHING;
		if(chk_tempbuf())
			break;
		if( n->s){
			xpand(b_player, tempbuf, n->s, TBUFSIZ);
			sargv = save_xargs(sargc = xargc, &sargl);
			init_match(b_player, tempbuf, NOTYPE);
			match_neighbor();
			match_possession();
			match_me();
			match_here();
			match_absolute();
			if( ArchWizard(b_player)){
				match_player();
			}
			res = noisy_match_result();
			restore_xargs(sargc, sargv, sargl);
		}else 
			res = NOTHING;
		break;
	}
	if(bad_dbref(res, NOTHING)){
		res = NOTHING;
	}
	
	return res;
}

dbref OBJ1(b)
BOOLEXP_PTR b;
{
	return decode_obj(NT_T1(b), & (vm_bool(b)->u1));
}

dbref OBJ2(b)
BOOLEXP_PTR b;
{
	return decode_obj(NT_T2(b), & (vm_bool(b)->u2));
}

BOOLEXP_PTR PTR1(b)
BOOLEXP_PTR b;
{	BOOLEXP_PTR res;
	res = TRUE_BOOLEXP;

	if(NT_T1(b) == NT_PTR)
		res = (BOOLEXP_PTR )vm_bool(b)->u1.p;
	return res;
	
}

BOOLEXP_PTR PTR2(b)
BOOLEXP_PTR b;
{	BOOLEXP_PTR res;
	res = TRUE_BOOLEXP;

	if(NT_T2(b) == NT_PTR)
		res = (BOOLEXP_PTR )vm_bool(b)->u2.p;
	return res;
}

char *STR1(b)
BOOLEXP_PTR b;
{	char *res = (char *)"";
	struct dbref_chain *chain;
	dbref c;

	if( NT_T1(b) == NT_STR){
#ifdef VM_MSGS
		res = get_desc(vm_bool(b)->u1.l);
#else
		res = (char *)vm_bool(b)->u1.s;
#endif
	}else if( NT_T1(b) == NT_ARG){
		c = vm_bool(b)->u1.i;
		if( c <= 0){
			res = Name0(receiver);
		}else if( c < xargc){
			res = xargv[c];
			if( chain = xargl(c) ){
				if( !bad_dbref(chain->this, 0)){
					res = Name0(chain->this);
				}
			}
		}
	}
	if( !res) res = "";
	return res;
}

char *STR2(b)
BOOLEXP_PTR b;
{	char *res = (char *)"";
	struct dbref_chain *chain;
	dbref c;

	if( NT_T2(b) == NT_STR){
#ifdef VM_MSGS
		res = get_desc(vm_bool(b)->u2.l);
#else
		res = (char *)vm_bool(b)->u2.s;
#endif
	}else if(NT_T2(b) == NT_ARG){
		c = vm_bool(b)->u2.i;
		if( c <= 0){
			res = Name0(receiver);
		}else if( c < xargc){
			res = xargv[c];
			if( chain = xargl(c) ){
				if( !bad_dbref(chain->this ,0)){
					res = Name0(chain->this);
				}
			}
		}
	}
	if( !res) res = "";
	return res;
}

long VAL1(b)
BOOLEXP_PTR b;
{	long res;
	int t= NT_T1(b);

	if( t == NT_INT)
		res = vm_bool(b)->u1.l;
	else if( t == NT_STR || t == NT_ARG)
		res = atol(STR1(b));
	return res;
}

long VAL2(b)
BOOLEXP_PTR b;
{	long res;
	int t = NT_T2(b);

	if( t == NT_INT)
		res = vm_bool(b)->u2.l;
	else if( t == NT_STR || t == NT_ARG)
		res = atol(STR2(b));
	return res;
}

int ARG1(b)
BOOLEXP_PTR b;
{	int res = vm_bool(b)->u1.i;

	return res;
}

int ARG2(b)
BOOLEXP_PTR b;
{	int res = vm_bool(b)->u2.i;

	return res;
}

BOOLEXP_PTR * W_PTR1(b)
BOOLEXP_PTR b;
{
	BOOLEXP_PTR * res = &(vm_bool(b)->u1.p);
	vm_bool(b)->n_type |= NT_PTR;
	return res;
}

BOOLEXP_PTR * W_PTR2(b)
BOOLEXP_PTR b;
{
	BOOLEXP_PTR * res = &vm_bool(b)->u2.p;
	vm_bool(b)->n_type |= (NT_PTR << 4);
	return res;
}

long *W_VAL1(b)
BOOLEXP_PTR b;
{	long * res = &vm_bool(b)->u1.l;
	vm_bool(b)->n_type |= NT_INT;
	return res;
}

long *W_VAL2(b)
BOOLEXP_PTR b;
{	long * res = &vm_bool(b)->u2.l;
	vm_bool(b)->n_type |= NT_INT << 4;
	return res;
}

dbref *W_OBJ1(b)
BOOLEXP_PTR b;
{	dbref * res = &vm_bool(b)->u1.d;
	vm_bool(b)->n_type |= NT_DBREF;
	return res;
}

dbref *W_OBJ2(b)
BOOLEXP_PTR b;
{	dbref * res = &vm_bool(b)->u2.d;
	vm_bool(b)->n_type |= NT_DBREF << 4;
	return res;
}

void W_STR1(b, s)
BOOLEXP_PTR b;
char *s;
{	int otype = NT_T1(b);

	vm_bool(b)->n_type |= NT_STR;
#ifdef VM_MSGS
	vm_bool(b)->u1.l = alloc_desc_2(s);
#else
	if( otype == NT_STR){
		free_string(vm_bool(b)->u1.s);
	}
	vm_bool(b)->u1.s = s;
#endif
}


void W_STR2(b, s)
BOOLEXP_PTR b;
char *s;
{	int otype = NT_T2(b);

	vm_bool(b)->n_type |= NT_STR << 4;
#ifdef VM_MSGS
	vm_bool(b)->u2.l = alloc_desc_2(s);
#else
	if( otype == NT_STR){
		free_string(vm_bool(b)->u2.s);
	}
	vm_bool(b)->u2.s = s;
#endif
}

int *W_ARG1(b)
BOOLEXP_PTR b;
{	int * res = &vm_bool(b)->u1.i;
	vm_bool(b)->n_type |= NT_ARG;
	return res;
}


int *W_ARG2(b)
BOOLEXP_PTR b;
{	int * res = &vm_bool(b)->u2.i;
	vm_bool(b)->n_type |= NT_ARG  << 4;
	return res;
}

BOOLEXP_PTR *W_PTR(b, w)
BOOLEXP_PTR b;
int w;
{	if( w)
		return W_PTR2(b);
	return W_PTR1(b);
}

void W_STR(b, w, s)
BOOLEXP_PTR b;
int w;
char *s;
{	if( w)
		W_STR2(b, s);
	else
		W_STR1(b, s);
}


int *W_ARG(b, w)
BOOLEXP_PTR b;
int w;
{	if( w)
		return W_ARG2(b);
	return W_ARG1(b);
}

dbref *W_OBJ(b, w)
BOOLEXP_PTR b;
int w;
{	if( w)
		return W_OBJ2(b);
	return W_OBJ1(b);
}

long *W_VAL(b, w)
BOOLEXP_PTR b;
int w;
{	if( w)
		return W_VAL2(b);
	return W_VAL1(b);
}




struct locking *has_lock(thing, n)
dbref thing;
int n;
{	struct locking *(*l)[];

	l = Lock(thing);
	if( l){
		return (*l)[n];
	}
	return NULL;
}

BOOLEXP_PTR has_lock_key(thing, n)
dbref thing;
int n;
{	struct locking *(*l)[];

	l = Lock(thing);
	if( l && (*l)[n]){
		return (*l)[n]->key;
	}
	return TRUE_BOOLEXP;
}


int is_bool_func(s)
char *s;
{	struct bool_rec *t = booltable;

	last_bool_rec = NULL;	
	if( !s || !*s)return 0;	/* not found */

	while(*(t->name)){
		if( !strcmp(t->name, s)){
			last_bool_rec = t;
			return t->type;
		}
		t++;
	}
	return 0;
}

int boolexp_info(b, s, f)
BOOLEXP_PTR b;
int *f;
char **s;
{	struct bool_rec *t = booltable;

	if( !b || b == TRUE_BOOLEXP){
		*s = (char *)"*UNLOCKED*";
		*f = -1;
		return -1;
	}

	while( *(t->name) ){
		if(t->type == vm_bool(b)->n_func)
			break;
		t++;
	}
	*s = (char *)t->name;
	*f = t->fmt;
	return t->fmt;
}

/* see if Object is referenced in this Boolexp */

int search_boolexp( player, b, obj)
dbref player, obj;
BOOLEXP_PTR b;
{

    if(b == TRUE_BOOLEXP) {
	return 0;
    } else {
	switch(vm_bool(b)->n_func) {
	  case BOOLEXP_AND:
	  case BOOLEXP_OR:
		return search_boolexp(player, PTR1(b), obj) ||
			search_boolexp(player,PTR2(b), obj); 

	  case BOOLEXP_NOT:
	    return search_boolexp(player, PTR1(b), obj);

	  case BOOLEXP_CONST:
	    if( NT_T1(b) != NT_DBREF)
		return 0;
	    return OBJ1(b) == obj;

	  case BOOLEXP_GROUP:
		if( NT_T1(b) == NT_DBREF && OBJ1(b) == obj)
			return 1;
		return 0;	/* Doesnt contain obj */

	  case BOOLEXP_FUNC:
		return 0;

	  case BOOLEXP_DROP:
		return 0;
	  case BOOLEXP_TRUE:
	  case BOOLEXP_FALSE:
		return 0;

	  default: 
		if( NT_T1(b) == NT_DBREF && OBJ1(b) == obj) return 1;
		if( NT_T2(b) == NT_DBREF && OBJ2(b) == obj) return 1;
		return 0;

	}
    }
}

/* Expand the parameter markersin the source string (s) into the
 * destination string (d) generating at max n chars.
 */

void xpand(player, d, s, n)
dbref player;
char *d, *s;
int n;
{	char *p, ch;
	int c,i;
	char buf[30];
       	long tim ;
	struct dbref_chain *chain;
	int numeric = 0;		/* Use numeric form.. */

	while(n-- && s && *s){
		ch = *s;
		if( ch == '$'){
			numeric = 0;
			buf[0] = '\0';
			p = buf;
			if( s[1] == '#'){
				numeric=1;
				s++;
			}
			if( s[1] >= '0' && s[1] <= '9'){
				c = s[1] - '0';
				if( c <= 0){
					if( numeric){
						sprintf(buf,"#%d", receiver);
					}else{
						sprintf(buf, "%.28s", Name0(receiver));
					}
					p = buf;
				}else if( c < xargc){
					p = xargv[c];
					if( chain = xargl(c) ){
						if( !bad_dbref(chain->this ,0)){
							if(numeric){
								sprintf(buf, "#%d", chain->this);
								p = buf;
							}else{
								p = Name0(chain->this);
							}
						}
					}
				}
			}else if( (s[1] == '$'|| s[1] == 'p')){
				p = name_anon(player);
			}else if( (s[1] == 'l' || s[1] == 'L') ){
				if( b_here == NOTHING)
					b_here = Location(player);
				if( s[1] == 'l' || numeric)
					sprintf(buf,"#%d",b_here);
				else
					sprintf(buf, "%.28s", Name0( b_here) );
				p = buf;
/* $e used in debugging mostly */
			}else if( s[1] == 'e'){
				sprintf(buf,"(%d)", euid);
				p = buf;
			}else if( s[1] == 'f'){
				sprintf(buf,"%.28s", failure);
				p = buf;
			}else if( s[1] == 'R'){
				if(numeric){
					sprintf(buf, "#%d", receiver);
				}else{
					sprintf(buf, "%s", Name0(receiver));
				}
				p = buf;
			}else if( s[1] == 's'){
				if(numeric){
					sprintf(buf, "#%d", sender);
				}else{
					sprintf(buf, "%s", Name0(sender));
				}
				p = buf;
			}else if( s[1] == 'S'){
				sprintf(buf, "%s", name_anon(sender));
				p = buf;
			}else if(  s[1] == 'd'){
                            (void) time(&tim);
				p = ctime(&tim);
				i = 0;
				while(n && *p && i++ < 10){
					*d++ = *p++;
					n--;
				}
				*s++;
				*s++;
				continue;
			}else if( s[1] == 't'){
                            (void) time(&tim);
                            p = ctime(&tim);

                            p = &p[11];
				i = 0;
				while(n && *p && i++ < 8){
					*d++ = *p++;
					n--;
				}
				*s++;
				*s++;
				continue;
			}else if( s[1] == 'x'){ /* exit name */
				if( ArchWizard(player)){
					sprintf(buf,"#%d", this_exit);
					p = buf;
				}
			}else if( s[1] == 'c'){	/* Class ? */
				if( ArchWizard(player)){
					sprintf(buf,"#%d", Location(this_exit));
					p = buf;
				}
			}
			if( *p){
				while(n && *p){
					*d++ = *p++;
						n--;
					}
					s++;
					s++;
			}
		}else if( *s == '\\' && 
			  (s[1] == '"' || s[1] == '\\' || s[1] == '$')){
			s++;
		}
		*d++ = *s++;
	}
	*d = '\0';
}

money limit_rewards(player, reward)
dbref player;
money reward;
{	money pen, lowerpen;

	lowerpen = 0;

	if( Level(player) > 0){
		lowerpen = player_level_table[Level(player)-1].pennies;
	}
	pen = player_level_table[Level(player)].pennies;

	pen = pen - lowerpen;		/* make pen == range */

	if( reward > pen/ 5)
		reward = pen/5;
	return reward;
}

/* For player evaluate the boolexp b. thing_2 is the object the lock
 * is in
 */


int eval_bool( player, b, thing_2, parent_func)
dbref player, thing_2;
BOOLEXP_PTR b;
int parent_func;
{	int ret=0;
	char *p, *p1, *p2;
#ifdef BOOLEXP_AGE
	long tage, tage1;
#endif
	dbref container,object,thing,thng;
	int sargc;
	char **sargv;
	dbref victim, uid, auth;
	money cost;
	int cnt, t2;
	dbref saved_euid = euid, saved_rec, saved_sen;
	int qs = quiet, saved_last = last_result;
	BOOLEXP *bexp = vm_bool(b);
	struct dbref_chain *sargl;
	dbref exit_owner = NOTHING;

	if(bad_dbref(thing_2, 0)){
fprintf(logfile,"eval_bool: bad dbref for exit (%d)\n", thing_2);
		return 0;
	}
	exit_owner = Owner(thing_2);

	eval_bool_count++;

    if(b == TRUE_BOOLEXP) {
	ret = 1;
    } else {
	switch(vm_bool(b)->n_func) {
	  case BOOLEXP_FALSE:
		ret = 0;
		break;

	  case BOOLEXP_TRUE:
		ret =1;
		break;

	  case BOOLEXP_AND:
	    ret = (eval_bool(player, PTR1(b), thing_2, vm_bool(b)->n_func)
		    && eval_bool(player, PTR2(b), thing_2, vm_bool(b)->n_func) );
		break;
	  case BOOLEXP_OR:
	    ret = (eval_bool(player, PTR1(b), thing_2, vm_bool(b)->n_func)
		    || eval_bool(player, PTR2(b), thing_2, vm_bool(b)->n_func) );
		break;
	  case BOOLEXP_NOT:
	    ret = !eval_bool(player, PTR1(b), thing_2, vm_bool(b)->n_func);
		break;
	  case BOOLEXP_CONST:
		if( NT_T1(b) == NT_ARG){
			thng = find_arg(vm_bool(b)->u1.i, receiver);
			if( thng != NOTHING){
				thng = prune_chain(vm_bool(b)->u1.i, receiver, parent_func);
			}

			if( (thng != NOTHING) == (parent_func!=BOOLEXP_NOT) ){
				return 1;
			}else
				return 0; 
		}else if( NT_T1(b) == NT_INT){
			thng = VAL1(b);
		}else
			thng = OBJ1(b);

		if( thng < 0)
			return 0;	/* NOTHING or AMBIGUOUS */

		ret = (thng == sender
		    	|| (euid != NOTHING && thng == euid)
		    	|| member(thng, Contents(sender)) 
			 );
		break;

	  case BOOLEXP_GROUP:
		thing = OBJ1(b);
		if( bad_dbref(thing, 0))
			return 0;
		if( Typeof(thing) != TYPE_EXIT){
fprintf(logfile,"Found non exit %d as arg to group lock on %d\n", thing, thing_2);
			return 0;
		}
		
		if( depth++ > 30){
			sprintf(msgbuf, "DEPTH LOOP in @ for lock (#%d)", thing_2);
			fnotify(player,msgbuf);
			ret = 1;
			break;
		}
		ret = eval_bool(player, Key(thing) ,thing_2, parent_func);
		depth--;
		break;

	  case BOOLEXP_FUNC:
fprintf(logfile,"Found func on %s(%d)\n", Name(thing_2), thing_2);
		if( !Player(player)){
			ret = 1;
			break;
		}
		ret = 0;
		break;

	    case BOOLEXP_EXEC:
	    case BOOLEXP_EXECL:
		saved_rec = receiver;
		saved_sen = sender;

		if(depth++ > 30){
			sprintf(msgbuf,"DEPTH LOOP processing EXEC's in lock (#%d)", thing_2);
			fnotify(player, msgbuf);
			ret = 1;
			break;
		}

		if( Typeof(player) != TYPE_PLAYER){
			if( default_beaker > 0)
				player = default_beaker;
			else {
				player = 1;
			}
		}

		if( vm_bool(b)->n_func == BOOLEXP_EXEC)
			quiet = 1;
		else
			quiet = 0;

		if( NT_T2(b) != NT_UNKN)
			uid = OBJ2(b);
		else
			uid = 0;
		if(uid && exit_owner == uid && !is_god(uid) ){
			euid = uid;
		}else if( uid){
sprintf(msgbuf,"EXEC: %d not owner of %d who is %d",uid, thing_2, exit_owner);
fprintf(logfile,"%s\n",msgbuf);
			quiet = 0;
			notify(player,msgbuf);
			ret = 0; /* fail for now */
			break;
		}else if( TrueWizard(player) && !TrueWizard(exit_owner)){
			euid = exit_owner;
fprintf(logfile,"EXEC: euid set to %d on exit %d\n", euid, thing_2);
		}else {
			euid = NOTHING;
		}

		created_object = NOTHING;

		if(chk_tempbuf()){
			ret = 0;
			break;
		}
		if( !basic_auth_check(thing_2, player, "EXEC")){
			ret = 0;
			break;
		}

		xpand(player, tempbuf, STR1(b), TBUFSIZ);
		sargc = xargc;
		if(sargc){
			sargv = save_xargs(sargc, &sargl);
			if( !sargv){
				return 0; /* no memory */
			}
		}
		p = tempbuf;
		if( is_god(player)){
fprintf(logfile,"Tried to exec on God(%d) '%s' from %d\n", player, p, thing_2);
			last_result =0;
		}else {

 			last_result = 1;	/* most succeed */
			if(p && *p){
				process_command(player, p);
			}else
				last_result = 0;
		}

		restore_xargs(sargc, sargv, sargl);

		receiver = saved_rec;
		sender = saved_sen;

		if( last_result)
			ret = 1;
		else
			ret = 0;
		last_result = saved_last;
		depth--;
		break;

	  case BOOLEXP_AGE:
		switch(NT_T1(b)){
		case NT_INT:
			tage = vm_bool(b)->u1.l;
			tage = timenow - tage;
			break;
		default:
			thing = OBJ1(b);
			if(bad_dbref(thing, 0))
				return 0;
			tage = Age(thing);
			break;
		}
		switch(NT_T2(b)){
		case NT_INT:	/* Old databases have second arg as an int */
			thing = vm_bool(b)->u2.i;
			if(bad_dbref(thing, 0))
				return 0;
			if( !thing)
				thing = player;
			tage1 = Age(thing);
			break;
		default:
			thing = OBJ2(b);
			if(bad_dbref(thing, 0))
				return 0;
			if( !thing)
				thing = player;
			tage1 = Age(thing);
			break;
		}
		ret = tage <= tage1;
		break;

	  case BOOLEXP_LEVEL:
		if( NT_T2(b) != NT_UNKN){
			thing = OBJ2(b);
			if( bad_dbref(thing, 0)){
				ret = 0;
				break;
			}
		}else
			thing = player;
		if(!thing)
			thing = player;		/* Special case */

		switch(NT_T1(b)){
		case NT_INT:
			cnt = vm_bool(b)->u1.l;
			ret = cnt >= Level(thing);
			break;
		default:
			thng = OBJ1(b);
			if( bad_dbref(thng, 0)){
				ret = 0;
			}else if(Typeof(thng) != TYPE_PLAYER)
				ret = 0;
			else
				ret = Level(thng) > Level(thing);
		}
		break;

	  case BOOLEXP_HAS:

		if( NT_T1(b) == NT_INT){
			cnt = VAL1(b);
			container = OBJ2(b);
			if( container < 1)
				container = player;

			if( bad_dbref(container, 0))
				return 0;

			for(thing = Contents(container); thing != NOTHING; thing =Next(thing)){
				if( Typeof(thing) != TYPE_EXIT)
					cnt--;
			}
			if( cnt >= 0)
				return 1;
			return 0;
		}else 
 		container=  OBJ1(b);

		if( container < 1)
			container = b_here;

		if( bad_dbref(container, 0))
			return 0;
		if( NT_T2(b) == NT_ARG){
			object = find_arg(vm_bool(b)->u2.i, container);
			if( object != NOTHING)
				object = prune_chain(vm_bool(b)->u2.i, container, parent_func);
		}else
			object   =  OBJ2(b);
		if( object < 0)
			return 0;
		for(thing = Contents(container); thing != NOTHING; thing = Next(thing)){
			if( thing == object)
				return 1;
		}
		return 0;
/* The MOVE function */
	  case BOOLEXP_TEL:{
			dbref dest,victim;

			euid = exit_owner;

			victim = OBJ1(b);
			dest = OBJ2(b);
			if( bad_dbref(victim ,0) || bad_dbref(dest, 0)){
				ret = 0;
				break;
			}
			quiet = 1;
			if( !do_tel_sub(player, victim, dest, exit_owner )){
				ret= 0;
			}else
				ret= 1;
		}
		break;

	  case BOOLEXP_DROP:
fprintf(logfile,"Obj %s(%d) has a drop lock\n", Name(thing_2), thing_2);
		ret = 0;
		break;

	  case BOOLEXP_FLAGS:
		if( NT_T2(b) == NT_UNKN)
			victim = sender;
		else
			victim = OBJ2(b);
		if( victim == 0)
			victim = sender;

		if( bad_dbref(victim, 0))
			return 0;

		cnt = VAL1(b);
		if( cnt == 16)
			ret = logged_in( victim);
		else if( cnt == 17){
		}else
			ret = (0 != ((1 << cnt) & Flags(victim)) );
		break;
		
	  case BOOLEXP_PAY:
		if( NT_T2(b) != NT_UNKN)
			victim = OBJ2(b);
		else
			victim = player;

		if( bad_dbref(victim, 0) || Typeof(victim) != TYPE_PLAYER ||
			!logged_in_or_robot(victim)){
			ret= 0;
			break;
		}
		cost = VAL1(b);

		if( cost >= 0){
			ret = payfor(victim, cost);
		}else {
			sprintf(msgbuf,"PAY{..., %d} failed, use REWARD{} intead!", thing_2);
			fnotify(player, msgbuf);
			ret = 0;	/* Pay failed */
		}
		break;

	  case BOOLEXP_REWARD:
		if( NT_T2(b) != NT_UNKN)
			victim = OBJ2(b);
		else
			victim = player;

		if( bad_dbref(victim, 0) || Typeof(victim) != TYPE_PLAYER ||
				!logged_in_or_robot(victim)){
			ret= 0;
			break;
		}

		cost = VAL1(b);

		if( !TrueWizard(exit_owner) ){
sprintf(msgbuf,"Non wiz owner %d on REWARD lock %d", exit_owner, thing_2);
fnotify(exit_owner, msgbuf);
			ret = 0;
			break;
		}
		if( victim == Owner(receiver) && !TrueWizard(victim)){
sprintf(msgbuf,"REWARD: owner %d of %d tried to get reward on exit %d", victim, receiver, thing_2); 
fnotify(exit_owner, msgbuf);
			ret = 0;
			break;
		}

		if( cost < 0){
sprintf(msgbuf,"REWARD{..., %d} failed, use PAY{} intead!", thing_2);
fnotify(exit_owner, msgbuf);
			ret = 0;	/* Reward failed */
		}
		cost = limit_rewards(victim, cost);

fprintf(logfile,"REWARD: %s(%d) got %s from exit(%d) on object %d %s\n", Name(victim), victim, prt_money(cost), thing_2, receiver, Name(receiver));
		Pennies(victim) = Pennies(victim) + cost;	/* make payment */
		euid = NOTHING;
		player_level(victim);
		ret = 1;	/* ok */
		break;

	  case BOOLEXP_HEAL:
		if( NT_T2(b) == NT_UNKN)
			victim = sender;
		else
			victim = OBJ2(b);

		if( victim ==0)
			victim = sender;

		if( bad_dbref(victim, 0))
			return 0;
		if( !Player(victim) || !logged_in_or_robot(victim)){
			set_error("Not a player");
			return 0;		/* fails on non players */
		}
		cost = VAL1(b);
		if( !TrueWizard(exit_owner) ){
sprintf(msgbuf,"Non wiz owner %d on WOUND lock %d", exit_owner, thing_2);
fnotify(player, msgbuf);
			ret=0;
			break;
		}
		if( NT_T1(b) == NT_INT && cost < 0){
fprintf(logfile,"Translating HEAL on %d to WOUND\n", thing_2);
			vm_bool(b)->n_func = BOOLEXP_WOUND;
			vm_bool(b)->u1.l = -vm_bool(b)->u1.l;
		}

		ret= do_heal_sub(player, victim, cost);
		break;

	  case BOOLEXP_WOUND:
		if( NT_T2(b) == NT_UNKN)
			victim = sender;
		else
			victim = OBJ2(b);

		if( victim ==0)
			victim = sender;

		if( bad_dbref(victim, 0))
			return 0;
		if( !Player(victim) || !logged_in_or_robot(victim)){
			set_error("Not a player");
			return 0;		/* fails on non players */
		}
		cost = VAL1(b);
		if( cost > 0)
			cost = -cost;

		if( !TrueWizard(exit_owner) ){
sprintf(msgbuf,"Bad owner for WOUND{} lock. %d", thing_2);
fnotify(player, msgbuf);
			ret=0;
			break;
		}

		ret= do_heal_sub(player, victim, cost);
		break;

	      case BOOLEXP_BSUCC:
	      case BOOLEXP_BOSUCC:
	      case BOOLEXP_SUCC:
	      case BOOLEXP_FAIL:
	      case BOOLEXP_OSUCC:
	      case BOOLEXP_OFAIL:
		if( chk_tempbuf()){
			goto bad_succ;
		}
/* New SUCC/OSUCC */
		t2 = NT_T2(b);
		if( t2 != NT_UNKN){	/* Parameter 2 is the Message */
			p = STR2(b);
			victim = OBJ1(b);
			if( !bad_dbref(victim, 0) && !controls(exit_owner,loc_of(victim))){
sprintf(tempbuf,"SUCC/OSUCC in lock(#%d) %d not auth to SUCC to %d", thing_2, exit_owner, victim);
				fnotify(exit_owner, tempbuf);
				goto bad_succ;
			}
		}else {
			p = STR1(b);	/* Parameter 1 is the Message */
			victim = sender;
		}
		if( p && *p && !bad_dbref(victim, 0)){
			xpand(player, tempbuf, p, TBUFSIZ);
			if( vm_bool(b)->n_func == BOOLEXP_SUCC ||
			    vm_bool(b)->n_func == BOOLEXP_BSUCC||
			    vm_bool(b)->n_func == BOOLEXP_FAIL){
				if( vm_bool(b)->n_func != BOOLEXP_BSUCC){
					do_succ(victim, tempbuf, "");
				}
			}else {	thng = exit_owner;
				if( bad_dbref(thng, 0) || !Builder(thng)){
sprintf(tempbuf,"OSUCC in lock(#%d) not owned by a builder!", thing_2);
					fnotify(thng, tempbuf);
					goto bad_succ;
				}
				thing = loc_of(victim);
				if( !controls(thng, thing)){
sprintf(tempbuf,"OSUCC in lock (#%d) owned by %d, not auth to OSUCC in %d", thing_2, exit_owner, thing);
					fnotify(thng, tempbuf);
					goto bad_succ;
				}
				if( vm_bool(b)->n_func != BOOLEXP_BOSUCC){
					do_osucc(victim, tempbuf, "", thing);
				}
			}
		}
bad_succ:
		if( vm_bool(b)->n_func == BOOLEXP_SUCC ||
		    vm_bool(b)->n_func == BOOLEXP_BSUCC||
		    vm_bool(b)->n_func == BOOLEXP_BOSUCC||
		    vm_bool(b)->n_func == BOOLEXP_OSUCC)
			ret= 1;
		else
			ret= 0;
		break;

	      case BOOLEXP_PROMOTE:
		if( !TrueWizard( exit_owner ) ){
sprintf(msgbuf,"Non wiz owner %d on PROMOTE lock %d", exit_owner, thing_2);
fnotify(exit_owner, msgbuf);
			ret = 0;		/* Auth failed */
			break;
		}
		if( !logged_in_or_robot(player)){
			return 0;
		}
		player_level(player);

		if( Level(player) == VAL1(b)){
fprintf(logfile,"Promoting %s(%d) from %d using exit(%d)\n", Name(player), player, Level(player), thing_2);
			Level(player) = Level(player)+1;
			ret = 1;
		}else ret = 0;
		break;

	      case BOOLEXP_CLASS:
fprintf(logfile,"Found CLASS{} lock on %d\n", thing_2);
		ret = 0;
		break;

	    case BOOLEXP_PUSH:
		victim = sender;
		p = STR1(b);

		if( NT_T2(b) == NT_STR){
			victim = OBJ1(b);
			p = STR2(b);
		}
		if( bad_dbref(victim, 0)){	/* Could not find valid recipient */
			ret = 0;
			break;
		}

		if( !Player(victim)){
			ret = 1;
			break;
		}
		if( !basic_auth_check(thing_2, victim, "PUSH") ){
			return 0;
		}

		if( chk_tempbuf()){
			ret = 0;
			break;
		}
		xpand(victim, tempbuf, p, TBUFSIZ);
 		push_command(victim, tempbuf);
		ret = 1;
		break;

	  case	BOOLEXP_SEND:
	  case	BOOLEXP_SSEND:
		saved_rec = receiver;
		saved_sen = sender;
		created_object = NOTHING;

		if(chk_tempbuf()){
			return 0;
		}

		if( depth++ > 30){
			sprintf(msgbuf, "DEPTH LOOP in SEND for lock (#%d)", thing_2);
			fnotify(player,msgbuf);
			ret = 1;
			break;
		}
		if( NT_T1(b) != NT_STR && NT_T2(b) == NT_STR &&
			vm_bool(b)->n_func != BOOLEXP_SSEND){
			thing = OBJ1(b);
			if( bad_dbref(thing, 0) ){
fprintf(logfile,"SEND failed, bad dbref(%d)\n", thing);
					return 0;
			}
			xpand(thing, tempbuf, STR2(b), TBUFSIZ);
			victim = thing;
			auth = exit_owner;
			if( !ArchWizard(auth) && auth != Owner(victim) && auth != victim){
sprintf(msgbuf,"SEND: %d not auth to send %s to %d", auth, tempbuf, victim);
				fnotify(player, msgbuf);
				ret = 0;
				break;
			}
/* Don't change sender */
/* 19 Feb 93 put sender change back */
			sender=receiver;
		}else {
			if( NT_T2(b) == NT_UNKN){
				uid = 0;
			}else
				uid = OBJ2(b);
			if(uid && exit_owner == uid && !is_god(uid) ){
				euid = uid;
			}else if( uid){
sprintf(msgbuf,"SEND: %d not owner of %d who is %d",uid, thing_2, exit_owner);
				quiet = 0;
				fnotify(exit_owner,msgbuf);
				ret = 0; /* fail for now */
				break;
			}else if((Flags(player)&WIZARD) && !(Flags(exit_owner)&WIZARD) ){
				euid = exit_owner;
fprintf(logfile,"SEND: euid set to %d on exit %d\n", euid, thing_2);
			}else {
				euid = NOTHING;
			}
/* expand the string with current arguments */
			xpand(player, tempbuf, STR1(b), TBUFSIZ);
			victim = receiver;
			auth = euid;
		}
		if( !basic_auth_check(thing_2, victim, "SEND")){
			ret = 0;
			break;
		}
/* Common Code part */
		p = tempbuf;

		sargc = xargc;
		if(sargc){
			sargv = save_xargs(sargc, &sargl);
			if( !sargv){
				return 0; /* no memory */
			}
		}

		last_result = 1;	/* most succeed */
		
		if(p && *p){
			if( vm_bool(b)->n_func == BOOLEXP_SSEND)
				ssend_message(sender, victim, p, auth, Location(thing_2));
			else
				send_message(sender, victim, p, auth);
		}
		restore_xargs(sargc, sargv, sargl);

		receiver = saved_rec;
		sender = saved_sen;

		if( last_result)
			ret = 1;
		else
			ret = 0;
		last_result = saved_last;
		depth--;
		break;

	  case BOOLEXP_RND:
		thing = VAL1(b);
		victim= VAL2(b);
		ret = random();
		if( victim < 8192)
			ret = ret >> 4;
		ret = ret % victim;
		ret = (victim - thing) <= ret;
		break;

	  case BOOLEXP_SHOW:
		thing = OBJ1(b);
		ret = show(player, b_here,thing);
		show_flush(player, b_here, ret);
		ret = 1;
		break;

	  case BOOLEXP_HIDE:
		thing = OBJ1(b);
		ret = hide(player, b_here, thing);
		show_flush(player, b_here, ret);
		ret = 1;
		break;

	  case BOOLEXP_DEF:
		thing = OBJ1(b);
		if( NT_T2(b) == NT_STR){
			visualise(player, thing, STR2(b));
		}else{
			visualise(player, thing, NULL);
		}
		ret = 1;
		break;

	  case BOOLEXP_POS:
	  	{	long sub, add;
	  		long pos;
	  		
	  		sub = 0l;
	  		add = 0l;
	  		pos = Position(receiver);
	  		if( NT_T1(b) == NT_INT)
	  			sub = VAL1(b);
	  		else {
	  			thing = OBJ1(b);
	  			if( !bad_dbref(thing, 0))
	  				sub = Position(OBJ1(b));
	  		}
            
            if( NT_T2(b) == NT_INT)
            	add = VAL2(b);
            else {
	  			thing = OBJ2(b);
	  			if( !bad_dbref(thing, 0))
	  				add = Position(OBJ2(b));
	  		}
	  		Position(receiver) = UpdatePosition(pos, sub, add);
	  	}
		ret = 1;
		break;

	  case BOOLEXP_WITHIN:
	  	{	long sub, add;
	  		long pos;
	  		
	  		sub = 0l;
	  		add = 0l;
	  		pos = Position(receiver);
	  		if( NT_T1(b) == NT_INT)
	  			sub = VAL1(b);
	  		else {
	  			thing = OBJ1(b);
	  			if( !bad_dbref(thing, 0))
	  				sub = Position(OBJ1(b));
	  		}
            
            if( NT_T2(b) == NT_INT)
            	add = VAL2(b);
            else {
	  			thing = OBJ2(b);
	  			if( !bad_dbref(thing, 0))
	  				add = Position(OBJ2(b));
	  		}
	  		ret = Within(pos, sub, add);
	  	}
		break;

	  default:
fprintf(logfile,"Unknown BOOLEXP type %d(%d)\n",vm_bool(b)->n_func, OBJ1(b));
	    ret= 0;
	}
    }
	euid = saved_euid;
	quiet= qs;
    return ret;
}

int eval_boolexp(player, b,thing_2)
dbref player,thing_2;
BOOLEXP_PTR b;
{	int ret;
	dbref saved_player = b_player;
	dbref saved_here = b_here;
	char *saved_tb = tempbuf;
	
	failure = "";
	tempbuf = NULL;

	if(boolexp_depth > 20)
		return 0;	/* Fail */
	boolexp_depth++;

	b_player = player;
	b_here = loc_of(player);

	this_exit = thing_2;

	ret = eval_bool(player, b, thing_2, BOOLEXP_TRUE);

	if( tempbuf){
		free((FREE_TYPE) tempbuf);
		tempbuf = NULL;
	}

	b_player = saved_player;
	b_here = saved_here;
	boolexp_depth--;
	tempbuf = saved_tb;
	return ret;
}

void list_bool_funcs(player, arg1)
dbref player;
const char *arg1;
{	int n = atoi(arg1);
	int i, ncom;
	struct bool_rec *t = booltable;

	ncom = sizeof(booltable) / sizeof(struct bool_rec);
	ncom = ncom -1;

	if( n < 0 || 16*n >= ncom){
		notify(player, "That page does not exist!");
		return;
	}

	sprintf(msgbuf,"Boolexp function list Page %d.", n);
	notify(player, msgbuf);

	sprintf(msgbuf,"==============================");
	notify(player, msgbuf);

	for(i=0; i < 16 && i+16*n < ncom; i++){
		sprintf(msgbuf, "%s", t[i+16*n].help);
		notify(player, msgbuf);
	}
	
}

static dbref xfrom,xto;
void do_chown_bool_sub(b)
BOOLEXP_PTR b;
{
	if( NT_T1(b) == NT_DBREF && vm_bool(b)->u1.d == xfrom)
		vm_bool(b)->u1.d = xto;
	if( NT_T2(b) == NT_DBREF && vm_bool(b)->u2.d == xfrom)
		vm_bool(b)->u2.d = xto;
}

void do_chown_boolexp(b, from,to)
BOOLEXP_PTR b;
dbref from,to;
{	dbref savedfrom = xfrom;
	dbref savedto = xto;

	xto = to;
	xfrom = from;

	apply_func(b, do_chown_bool_sub);

	xfrom = savedfrom;
	xto = savedto;
}
