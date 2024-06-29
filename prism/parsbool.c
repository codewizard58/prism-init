/* Routines to parse boolexp's */

#include "copyrite.h"

#include <stdio.h>
#include <ctype.h>
#ifdef MSDOS
#include <stdlib.h>
#endif

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "boolexp.h"
#include "match.h"

extern dbref db_base;
extern char *db_unparse_boolexp();
extern int dbversion;		/* Data Base version number */

int bad_parse;			/* Error flag */
static char *p_boolexp;
char *p_buf;
static dbref parse_player;	/* Parsing authority */
static int abs_parse;		/* Flag to control parsing level */

int Nextc()
{	return (int)*p_boolexp++;
}

void UnNextc(c, f)
int c;
FILE *f;
{
	--p_boolexp;
	*p_boolexp = c;
}

int skipws()
{	int c;
	while(1){
		c = Nextc();
		if( c!= ' '&& c != '\011')
			break;
	}
	return c;
}

#ifdef VM_BOOL
int bool_top;
#endif

BOOLEXP_PTR new_boolexp()
{	BOOLEXP_PTR n;

#ifdef VM_BOOL
	n = bool_top++;
	vm_bool(bool_top);
#else
	n = (BOOLEXP_PTR ) malloc(Sizeof(BOOLEXP));
	if( !n){
		fprintf(logfile,"New_boolexp: No memory\n");
		exit(99);
	}
#endif
	vm_bool(n)->n_type = 0;
	vm_bool(n)->n_func = 0;
	return n;
}

void bool_free(b)
BOOLEXP_PTR b;
{

#ifndef VM_BOOL
	free( (FREE_TYPE) b);
#else
	fprintf(logfile,"Free_bool(%d)\n", b);
#endif
}


dbref match_bool_const(parse_player, buf, type)
dbref parse_player;
char *buf;
int type;
{	dbref thing;
	static char abuf[100];
	char *p;

	strncpy(abuf, buf, 99);
	abuf[99] = '\0';
	p = abuf;
	while(*p){
		switch(*p){
		case ';':
		case '(':
		case ')':
		case '{':
		case '}':
		case '=':
		case '\n':
			*p = '\0';
		break;
		}
		if( *p)p++;
		else break;
	}

	init_match(parse_player, abuf, TYPE_THING);
	match_neighbor();
	match_possession();
	match_me();
	match_absolute();
	match_player();
	thing = match_result();

	if(thing == NOTHING) {
		sprintf(abuf, "I don't see %.20s here.", buf);
		notify(parse_player, abuf);
		return NOTHING;
	} else if(thing == AMBIGUOUS) {
		sprintf(abuf, "I don't know which %.20s you mean!", buf);
		notify(parse_player, abuf);
		return NOTHING;
	}
	return thing;
}

static BOOLEXP_PTR negate_boolexp(b)
BOOLEXP_PTR b;
{
    BOOLEXP_PTR n;

    /* Obscure fact: !NOTHING == NOTHING in old-format databases! */
    if(b == TRUE_BOOLEXP) return TRUE_BOOLEXP;

    n = new_boolexp();
    vm_bool(n)->n_func = BOOLEXP_NOT;
    *W_PTR1(n) = b;

    return n;
}

char *getword(f)
FILE *f;
{	static char buf[128];
	char *p;
	int n=0;

	p = buf;
	while( (*p = Nextc()) >= 'A' && *p <= 'Z'){
		if(n++ < 120)
			p++;
	}
	UnNextc(*p, f);
	*p = '\0';
	return buf;
}

extern struct bool_rec *last_bool_rec;

long b_getlong()
{	long res = 0;
	int ch;
	
	while(1) {
		ch = Nextc();
		if( ch >= '0' && ch <= '9'){
			res = res*10 + (ch -'0');
		}else
			break;
	}
	UnNextc(ch, NULL);
	return res;
}

void b_getword(buf)
char *buf;
{	int cnt, c;
	char *p;

	cnt = 0;
	p = buf;
	*p = '\0';

	while(cnt < 99){
		c = Nextc();
		if( (c>= 'a' && c <= 'z') ||
		    (c>= 'A' && c <= 'Z') ||
		    (c>= '0' && c <= '9') ||
		     c == ' ' || c == '\011' || c == '_' || c == '*'){
			*p++ = c;
			*p = '\0';
			cnt++;
		}else break;
	}
	UnNextc(c, NULL);
	while( p != buf){
		p--;
		if(*p != ' ' && *p != '\011')
			break;
		else
			*p = '\0';
	}
}


/* get_const 
 * Get the basic type of object.
 * top only set for top level calls IE allow functions
 */

static void get_const(b, which, top)
BOOLEXP_PTR b;
int which, top;
{	int c, ok=1;
	long val;
	char  *p;
	dbref d;
	
	c = skipws();

	if( c == '#'){
		val= b_getlong();
		if( val > 0)
			val = val + db_base;
		*W_OBJ(b, which) = val;
		ok++;
	}else if( c == '-'){
		*W_VAL(b, which) = -b_getlong();
		ok++;
	}else if( c == '$'){
		*W_ARG(b, which) = b_getlong();
		ok++;
	}else if( c == '"'){
		p = msgbuf;
		while( (c = Nextc()) != EOF){
			if( c == '"' || c == '\n')
				break;
			if( c == '\\'){
				c = Nextc();
				if(c != '"'){
					UnNextc(c,NULL);
					c = '\\';
				}
			}
			*p++ = c;
		}
		if(c != '"')
			UnNextc(c, NULL);
		*p = '\0';
		W_STR(b, which, (char *)alloc_string(msgbuf) );
		ok++;
	}else if( !(c >= '0' && c <= '9')){	/* Not a number */
		UnNextc(c, NULL);
		b_getword( msgbuf);
		if( top && !msgbuf[0]){
/* Top level can't be null, but FUNC args can */
			ok = 0;
		}else if( !strcmp(msgbuf, "ME")){
			if( which)
				vm_bool(b)->n_type |= NT_ME << 4;
			else
				vm_bool(b)->n_type |= NT_ME;
			ok++;
		}else if( !strcmp(msgbuf, "HERE")){
			if( which)
				vm_bool(b)->n_type |= NT_HERE << 4;
			else
				vm_bool(b)->n_type |= NT_HERE;
			ok++;
		}else if( !strcmp(msgbuf, "LOC")){
			if( which)
				vm_bool(b)->n_type |= NT_LOC << 4;
			else
				vm_bool(b)->n_type |= NT_LOC;
			ok++;
		}else if( !strcmp(msgbuf, "SENDER")){
			if( which)
				vm_bool(b)->n_type |= NT_SENDER << 4;
			else
				vm_bool(b)->n_type |= NT_SENDER;
			ok++;
		}else if( !strcmp(msgbuf, "SELF")){
			if( which)
				vm_bool(b)->n_type |= NT_RECEIVER << 4;
			else
				vm_bool(b)->n_type |= NT_RECEIVER;
			ok++;
		}else if(top){	/* Could be a func */
			vm_bool(b)->n_func = is_bool_func(msgbuf);
			if( vm_bool(b)->n_func ){
				c = Nextc();
				if(c == '{'){
					c = skipws();
					if( c != '}'){
						UnNextc(c, NULL);
						get_const(b, 0, 0);
						c = Nextc();
					}
					if( c != '}'){
						if( c != ',' ){
fprintf(logfile,"\nBad parse 2: BOOLEXP_%d\n", vm_bool(b)->n_func);
							bad_parse= 95;
							return;
						}
						if( c == ',')
							c = skipws();	/* Throw the comma */
						UnNextc(c, NULL);
						get_const(b, 1, 0);
						c = Nextc();
					}
					if( c != '}'){
fprintf(logfile,"\nBad parse 3: BOOLEXP_%d, %c(%x)\n", vm_bool(b)->n_func, c, c);
						bad_parse = 98;
					}
					return ;
				}else {
fprintf(logfile,"\nBad parse 4: BOOLEXP_%d\n", vm_bool(b)->n_func);
					bad_parse = 97;
					return ;
				}
			}
		}
		if(!abs_parse && !ok){
			if( !msgbuf[0]){
				d = NOTHING;
			}else {
				d = match_bool_const(parse_player, msgbuf, TYPE_THING);
			}
			if( d == NOTHING){
				bad_parse = 90;
			}else {
				*W_OBJ(b, which) = d;
				ok++;
			} 
		}
		if( !ok) {
			fprintf(logfile,"\ngetconst: bad parse (%s)\n",msgbuf);
			bad_parse = 89;
		}
	}else {
		UnNextc(c,NULL);
		val = b_getlong();
		if( top && !dbversion){	/* Top level are dbrefs, Honest! */
			if( val > 0)
				val = val + db_base;
			*W_OBJ(b, which) = val;
		}else
			*W_VAL(b, which) = val;
		ok++;
	}
	if( !ok){
fprintf(logfile,"\nNot ok in getconst, bad_parse=%d\n", bad_parse);
	}
}

static BOOLEXP_PTR getboolexp1(f)
FILE *f;
{
    BOOLEXP_PTR b;
    BOOLEXP_PTR b1;
    BOOLEXP_PTR b2;
    int c;

    c = skipws();
    switch(c) {
      case '\n':
	UnNextc(c, f);
	bad_parse = 80;
	return TRUE_BOOLEXP;
	/* break; */
      case 0:
      case EOF:
		bad_parse = 91;
		return TRUE_BOOLEXP;

	case '!':
		b = new_boolexp();
		vm_bool(b)->n_func = BOOLEXP_NOT;
		*W_PTR1(b) = getboolexp1(f);
		if(bad_parse){
			return TRUE_BOOLEXP;
		}
		return b;

      case '(':
		b = NULL;
		b1 = getboolexp1(f);
		b2 = b1;
		if(bad_parse){
			return TRUE_BOOLEXP;
		}
	    	c = skipws();
		if( c == AND_TOKEN || c == OR_TOKEN){
			b1 = new_boolexp();
			b = b1;		/* B points at last &/| node */
			while(1){
				if( c == AND_TOKEN)
					vm_bool(b)->n_func = BOOLEXP_AND;
				else
					vm_bool(b)->n_func = BOOLEXP_OR;
				*W_PTR1(b) = b2;
				b2 = getboolexp1(f);
				if(bad_parse){
					return TRUE_BOOLEXP;
				}
				c = skipws();
				if( c == AND_TOKEN || c == OR_TOKEN){
					*W_PTR2(b) = new_boolexp();
					b = PTR2(b);
				}else {
					*W_PTR2(b)= b2;
					break;
				}
			}
		}
	    if(c != ')'){
fprintf(logfile,"\nExpected ) got %c(%2x)\n", c, c);
		bad_parse = 88;
		return TRUE_BOOLEXP;
	    }
	    return b1;
	/* break; */

      case GROUP_TOKEN:
	b = getboolexp1(f);
	if( b == TRUE_BOOLEXP || vm_bool(b)->n_func != BOOLEXP_CONST){
		fprintf(logfile,"\nbad group\n");
		bad_parse = 92;
		return TRUE_BOOLEXP;
	}else
		vm_bool(b)->n_func = BOOLEXP_GROUP;
	return b;

#ifdef Notdef
      case FUNC_TOKEN:
	b = getboolexp1(f);
	if( b == TRUE_BOOLEXP || vm_bool(b)->n_func != BOOLEXP_CONST){
		fprintf(logfile,"Getboolexp1: bad func\n");
		bad_parse = 93;
		return TRUE_BOOLEXP;
	}
	vm_bool(b)->n_func = BOOLEXP_FUNC;
	return b;
#endif

      default:
	UnNextc(c, f);
	b = new_boolexp();
	vm_bool(b)->n_func = BOOLEXP_CONST;

	get_const(b, 0, 1);
	if( NT_T1(b) == NT_INT && VAL1(b) == -1){
		free( (FREE_TYPE) b);
		b = TRUE_BOOLEXP;
	}
	if( bad_parse)
		return TRUE_BOOLEXP;
	return b;
    }

  error:
    bad_parse = 96;
    return TRUE_BOOLEXP;
}

/* Only called from the @lock command */
/* exp2 -> exp1 [ &/| exp2 ]         */

BOOLEXP_PTR getboolexp2(f)
FILE *f;
{	BOOLEXP_PTR b;
	BOOLEXP_PTR n;
	int c;

	c = skipws();
	if( c == '\n'){	/* Not locked */
		return TRUE_BOOLEXP;
	}
	UnNextc(c, f);

	b = getboolexp1(f);

	if( bad_parse)
		return TRUE_BOOLEXP;

	c = skipws();
	if( c == AND_TOKEN || c == OR_TOKEN){
		n = b;
		b = new_boolexp();
		vm_bool(b)->n_func = c == AND_TOKEN ? BOOLEXP_AND : BOOLEXP_OR;
		*W_PTR1(b) = n;
		*W_PTR2(b) = getboolexp2(f);
	}else {
		UnNextc(c, NULL);
	}
	return b;
}

BOOLEXP_PTR getboolexp(f)
FILE *f;
{
    BOOLEXP_PTR b;
    int c;

    if( !p_buf){
	p_buf = (char *)malloc(2050);
    }
    if( !p_buf)
	return TRUE_BOOLEXP;

    fgets(p_buf, 2048, f);
    p_boolexp = p_buf;
    abs_parse = 1;	/* Absolute parse */
    bad_parse = 0;
    parse_player = 1;	/* Wizard / God */

	c = skipws();
	if( c == '\n'){	/* Not locked */
		return TRUE_BOOLEXP;
	}
	UnNextc(c, f);
	b = getboolexp1(f);
	c = skipws();
	if( !( !c || c == '\n') ){
		bad_parse = 70;
	}
	if( bad_parse){
		fprintf(logfile, "\nParse error %d", bad_parse);
		return TRUE_BOOLEXP;
	}
    return b;
}


BOOLEXP_PTR parse_boolexp(player , str)
dbref player;
char *str;
{	dbref saved_player = parse_player;
	BOOLEXP_PTR b;
	char buf[100];
	int c;

	parse_player = player;
	p_boolexp = str;
	abs_parse = 0;		/* User friendly parse */
	bad_parse = 0;
	b = getboolexp2(NULL);
	c = skipws();
	if( !( !c || c == '\n') ){
		bad_parse = 70;
	}
	if( bad_parse){
		sprintf(buf, "Parse error %d", bad_parse);
		notify(player, buf);
		b = TRUE_BOOLEXP;
	}
	parse_player = saved_player;
	return b;
}

/* This routine finds whether the lock has suc and fail standard form
 *
 *               |
 *             /  \
 *            &  FAIL
 *           / \
 *        LOCK SUCC
 */

int succ_fail_form(b, s, f)
BOOLEXP_PTR b;
BOOLEXP_PTR *s;
BOOLEXP_PTR *f;
{	BOOLEXP_PTR b1;
	BOOLEXP_PTR s1;
	BOOLEXP_PTR f1;

	if( b == TRUE_BOOLEXP)
		return 0;
	if( vm_bool(b)->n_func != BOOLEXP_OR)
		return 0;
	b1 = PTR1(b);
	if( b1 == TRUE_BOOLEXP || vm_bool(b1)->n_func != BOOLEXP_AND)
		return 0;
	s1 = PTR2(b1);
	f1 = PTR2(b);
	if( s1 != TRUE_BOOLEXP && (vm_bool(s1) )->n_func == BOOLEXP_SUCC &&
	    f1 != TRUE_BOOLEXP && (vm_bool(f1) )->n_func == BOOLEXP_FAIL){
		if( s )
			*s = s1;
		if( f)
			*f = f1;
		return 1;
	}
	return 0;
}

BOOLEXP_PTR make_succ_fail_form(b, s, f)
BOOLEXP_PTR b;
BOOLEXP_PTR *s;
BOOLEXP_PTR *f;
{	BOOLEXP_PTR res;
	BOOLEXP_PTR a;

	res = new_boolexp();
	vm_bool(res)->n_func = BOOLEXP_OR;

	*f = new_boolexp();
	(vm_bool(*f) )->n_func = BOOLEXP_FAIL;

	*s = new_boolexp();
	(vm_bool(*s) )->n_func = BOOLEXP_SUCC;

	a = new_boolexp();
	vm_bool(a)->n_func = BOOLEXP_AND;

	*W_PTR1(res) = a;
	*W_PTR2(res) = *f;
	*W_PTR1(a)   = b;
	*W_PTR2(a)   = *s;

	return res;
}

void FREE_STR(b, which)
BOOLEXP_PTR b;
int which;
{	char *s = NULL;

	if(which){
		if( NT_T2(b) == NT_STR)
			s = STR2(b);
		vm_bool(b)->n_type = vm_bool(b)->n_type & 0xf;
	}else{
		if( NT_T1(b) == NT_STR)
			s = STR1(b);
		vm_bool(b)->n_type = vm_bool(b)->n_type & 0xf0;
	}
	if( s)
		free_string(s);
}


BOOLEXP_PTR  set_succ_fail(player, key, s, os, f, of)
dbref player;
BOOLEXP_PTR key;
char *s, *os, *f, *of;
{	BOOLEXP_PTR sk;
	BOOLEXP_PTR fk;

	if( !succ_fail_form( key, &sk, &fk) ){
		key = make_succ_fail_form( key, &sk, &fk);
	}

	if( s && *s){
		W_STR1(sk, (char *)alloc_string(s) );
	}
	if( os && *os){
		sprintf(msgbuf,"$s %s", os);
		W_STR2(sk,(char *) alloc_string(msgbuf) );
	}
	if( f && *f){
		W_STR1(fk, (char *) alloc_string(f) );
	}
	if( of && *of){
		sprintf(msgbuf,"$s %s", of);
		W_STR2(fk, (char *)alloc_string( msgbuf) );
	}
	return key;
}

BOOLEXP_PTR  reset_succ_fail(player, key, s, os, f, of)
dbref player;
BOOLEXP_PTR key;
int s, os, f, of;
{	BOOLEXP_PTR sk;
	BOOLEXP_PTR fk;

	if( !succ_fail_form( key, &sk, &fk) ){
		key = make_succ_fail_form( key, &sk, &fk);
	}

	if( s){
		FREE_STR(sk, 0);
	}
	if( os ){
		FREE_STR(sk, 1);
	}
	if( f ){
		FREE_STR(fk, 0);
	}
	if( of ){
		FREE_STR(fk, 1);
	}
	return key;
}

/* Was from set.c */

BOOLEXP_PTR make_true_lock()
{	BOOLEXP_PTR b = new_boolexp();
	vm_bool(b)->n_func = BOOLEXP_TRUE;
	return b;
}

void do_osucc_trans(b)
BOOLEXP_PTR b;
{	BOOLEXP_PTR b1;
	BOOLEXP_PTR b2;

	b1 = new_boolexp();
	b2 = new_boolexp();

	vm_bool(b1)->n_func = vm_bool(b)->n_func;
	vm_bool(b1)->n_type = (vm_bool(b)->n_type) & 0xf;
	vm_bool(b1)->u1.s = vm_bool(b)->u1.s;

	vm_bool(b2)->n_type = (vm_bool(b)->n_type>>4) & 0xf;
	vm_bool(b2)->u1.s = vm_bool(b)->u2.s;

	vm_bool(b)->n_type = (NT_PTR << 4) | NT_PTR;
	*W_PTR1(b) = b1;
	*W_PTR2(b) = b2;
	if( vm_bool(b)->n_func == BOOLEXP_SUCC){
		vm_bool(b)->n_func = BOOLEXP_AND;
		vm_bool(b2)->n_func = BOOLEXP_OSUCC;
	}else{
		vm_bool(b)->n_func = BOOLEXP_OR;
		vm_bool(b2)->n_func = BOOLEXP_OFAIL;
	}
}



/* Routines used by db.c to translate to new format */
/* These routines are working on old object records */
/* BEFORE they have been translated */

dbref add_exit_to( i, name)
dbref i;
char *name;
{
	dbref j;

	j = new_object();
	change_object_type(j, TYPE_EXIT);

	NameW(j, name);
	Flags(j)= TYPE_EXIT;
	if( default_wizard > 0){
		Owner(j)= default_wizard;
	}else
		Owner(j)= Owner(i);
	PUSH(j, Contents(i));
	Location(j) = i;
	Key(j) = TRUE_BOOLEXP;
	Contents(j) = 0;		/* It's a command */
	return j;
}

void translate_player_locks(i)
dbref i;
{	BOOLEXP_PTR k;
	dbref j;

	k = KeyN(i, 0);
	if( k != TRUE_BOOLEXP){
		j = add_exit_to(i, "rob #");
		Key(j) = k;

		if( Flags(i) & ANTILOCK){
			Flags(i) &= ~ANTILOCK;
			k = new_boolexp();
			vm_bool(k)->n_func = BOOLEXP_NOT;
			*W_PTR1(k) = Key(j);
			Key(j) = k;
		}
		(* Lock(i))[0]->key = TRUE_BOOLEXP;
	}

	k = KeyN(i, 2);
	if( k != TRUE_BOOLEXP){
		Flags(i) |= LINK_OK;
	}
}

void trans_player_home(i,j)
dbref i,j;
{	dbref l;
	BOOLEXP_PTR k;
	
	l = add_exit_to(i, ".home #");
	Flags(l) |= INHERIT;
	k = new_boolexp();
	Key(l) = k;
	vm_bool(k)->n_func = BOOLEXP_SEND;
	vm_bool(k)->n_type = NT_STR | (NT_DBREF << 4);
	sprintf(msgbuf, ".home $0 to [#%d]", j);
	vm_bool(k)->u1.s = alloc_string(msgbuf);
	vm_bool(k)->u2.d = default_wizard;
}


void strip_player(i)
dbref i;
{	dbref j;
	int depth = 0;
/* Remove unwanted exits */

	if( (Flags(i)&(WIZARD|ROBOT)) == 0){
		for(j =Contents(i); depth++ < 200 && !bad_dbref(j, 0); j=Next(j)){
			if( Typeof(j) != TYPE_EXIT)
				continue;
			if( !strcmp(".home #", Name(j)) ){
fprintf(logfile,"Stripping '%s' from %d\n", Name(j), i);
				Contents(i) = remove_first( Contents(i), j);
				make_rubbish(j);
				j = Contents(i);
				if( j == NOTHING)
					break;
				continue;
			}
			if(bad_dbref(j, 0)){
				Next(j) = NOTHING;
			}
		}
	}
}

void translate_thing_locks(i)
dbref i;
{	BOOLEXP_PTR k;
	BOOLEXP_PTR k1;
	dbref j;
	money cost;

	if( is_rubbish(i)){
		return;
	}

	if( Flags(i) & STICKY){
		fprintf(logfile, "Thing %d has STICKY bit\n", i);
	}

	k = KeyN(i, 0);
	if( k != TRUE_BOOLEXP){
		j = add_exit_to(i, "get #");
		Key(j) = k;

		if( Flags(i) & ANTILOCK){
fprintf(logfile,"Thing %d has ANTILOCK flag\n", i);
			Flags(i) &= ~ANTILOCK;
			k = new_boolexp();
			vm_bool(k)->n_func = BOOLEXP_NOT;
			if( succ_fail_form(Key(j), NULL, NULL)){
				k1 = PTR1(Key(j));
				*W_PTR1(k) = PTR1(k1);
				*W_PTR1(k1)= k;
			}else{ /* Simple case */
				*W_PTR1(k) = Key(j);
				Key(j) = k;
			}
		}

		k = new_boolexp();
		vm_bool(k)->n_func = BOOLEXP_AND;
		*W_PTR1(k) = Key(j);
		*W_PTR2(k)= new_boolexp();
		Key(j) = k;
		k = PTR2(k);
		vm_bool(k)->n_func = BOOLEXP_SSEND;
		W_STR1(k, alloc_string( "get $0"));

		(* Lock(i))[0]->key = TRUE_BOOLEXP;
		Owner(j) = Owner(i);
		
	}

	k = KeyN(i, 1);
	if( k != TRUE_BOOLEXP){
fprintf(logfile,"Thing %s has a drop lock\n", Name(i));
		j = add_exit_to(i, "drop #");
		Key(j) = k;
		(* Lock(i))[1]->key = TRUE_BOOLEXP;
		Owner(j) = Owner(i);
	}

	k = KeyN(i, 4);
	if( k != TRUE_BOOLEXP){
		Flags(i) |= LINK_OK;
		j = add_exit_to(i, ".heartbeat #;.heartbeat");
		Key(j) = k;
		Flags(j) |= INHERIT;
		(* Lock(i))[4]->key = TRUE_BOOLEXP;
	}

	if( Flags(i) & WEAPON){
		cost = OldPennies(i);
		if( cost < 0){
			j = add_exit_to(i, ".defend from *");
			k = new_boolexp();
			Key(j) = k;
			vm_bool(k)->n_func = BOOLEXP_HEAL;
			vm_bool(k)->n_type = NT_INT | (NT_SENDER << 4);
			vm_bool(k)->u1.l = -cost;
		}else{
			j = add_exit_to(i, ".damage * with #");
			k = new_boolexp();
			Key(j) = k;
			vm_bool(k)->n_func = BOOLEXP_HEAL;
			vm_bool(k)->n_type = NT_INT | (NT_SENDER << 4);
			vm_bool(k)->u1.l = -cost;
		}
		Flags(j) |= INHERIT;

	}else if( OldPennies(i) ){
		j = add_exit_to(i, ".reward #");
		Flags(j) |= INHERIT;
		k = new_boolexp();
		Key(j) = k;
		vm_bool(k)->n_func = BOOLEXP_REWARD;
		vm_bool(k)->n_type = NT_INT | (NT_SENDER << 4);
		vm_bool(k)->u1.l = OldPennies(i);
	}
}

void trans_thing_home(i, j)
dbref i, j;
{	BOOLEXP_PTR k;
	dbref l;

	if( Flags(i) & STICKY){
		l = add_exit_to(i, "drop #");
		k = new_boolexp();
		Key(l) = k;
		vm_bool(k)->n_func = BOOLEXP_SEND;
		vm_bool(k)->n_type = NT_STR| (NT_DBREF << 4);
		sprintf(msgbuf,".drop # to [#%d]", j);
		vm_bool(k)->u1.s = alloc_string(msgbuf);
		vm_bool(k)->u2.d = default_wizard;
	}

	l = add_exit_to(i, ".reset #");
	Flags(l) |= INHERIT;
	k = new_boolexp();
	Key(l) = k;
	vm_bool(k)->n_func = BOOLEXP_TEL;
	vm_bool(k)->n_type = NT_RECEIVER | (NT_DBREF << 4);
	vm_bool(k)->u2.d = j;
}

void translate_exit_locks(i)
dbref i;
{	BOOLEXP_PTR k;
	BOOLEXP_PTR k1;
	struct locking *l;

	if( Flags(i) & ANTILOCK){
		Flags(i) &= ~ANTILOCK;
		k = new_boolexp();
		vm_bool(k)->n_func = BOOLEXP_NOT;
		if( succ_fail_form(KeyN(i,0), NULL, NULL)){
			k1 = PTR1(KeyN(i,0));
			*W_PTR1(k) = PTR1(k1);
			*W_PTR1(k1)= k;
		}else{ /* Simple case */
			*W_PTR1(k) = KeyN(i,0);
			l = has_lock(i, 0);
			l->key = k;
		}
	}

}


void translate_room(i)
dbref i;
{	BOOLEXP_PTR k;
	dbref j;

	if( Flags(i) & INHERIT)
		return;

	k = KeyN(i, 0);
	if( k != TRUE_BOOLEXP){
		j = add_exit_to(i, "[*] * has arrived");
		Key(j) = k;

		if( Flags(i) & ANTILOCK){
			Flags(i) &= ~ANTILOCK;
			k = new_boolexp();
			vm_bool(k)->n_func = BOOLEXP_NOT;
			*W_PTR1(k) = Key(j);
			Key(j) = k;
		}

		(* Lock(i))[0]->key = TRUE_BOOLEXP;
		Flags(j) |= INHERIT;
	}

}

/* Translate Class stuff */
/* Note done after re_copy */
/* so working on NEW type objects */
int is_def_class(i)
dbref i;
{
	if( i == NOTHING)
		return 0;
	if( i == default_thing_class ||
	    i == default_player_class ||
	    i == default_room_class ||
	    i == DEF_PLAYER ||
	    i == DEF_THING ||
	    i == DEF_ROOM
	){
		return 1;
	}
	return 0;
}

void translate_player_class(i)
dbref i;
{	dbref j,l;
	int depth = 0;

	if( Class(i) == 0 || Class(i) == NOTHING){
		if( default_player_class != NOTHING)
			Class(i) = DEF_PLAYER;
	}else{
		j = Class(i);
		while( depth++ < 20){
			if( is_def_class(j))
				break;
			l = Class(j);
			if( is_def_class(l))
				break;
			if( l <= 0 ){
				Class(j) = DEF_PLAYER;
				break;
			}
			if( l >= db_top || Typeof(l) != TYPE_ROOM ||
				(Flags(l) & INHERIT)!= INHERIT)
				break;
			j = l;
		}
	}
}


void translate_room_class(i)
dbref i;
{	dbref j, l, p;
	int depth;

	if( (Flags(i) & TEMPLE) && default_temple_class != NOTHING){
		Class(i) = default_temple_class;
		return;
	}

	p = Zone(i);

	if( (Class(i) == 0 || Class(i) == NOTHING) ){
		if( default_room_class != NOTHING)
			Class(i) = DEF_ROOM;
	}else{
		j = Class(i);
		if (is_def_class(j)){
			return;
		}
		if( j <= 0 || j >= db_top){
			j = Zone(i);
			Class(i) = j;
		}
		if( !bad_dbref(j, 0)){
			depth = 0;
			while( depth++ < 10){
				if( is_def_class(j) )
					break;
				l = Class(j);
				if( is_def_class(l) )
					break;
				if( l <= 0){	/* Check for Zone chain */
					l = Zone(j);
					if( l <= 0 || l >= db_top || Typeof(l) != TYPE_ROOM || (Flags(l)&INHERIT)!= INHERIT){
						l = -1;
					}else {
						Class(j) = l;
						break;
					}
				}
				if( l <= 0 ){
					Class(j) = DEF_ROOM;
					break;
				}
				if( l >= db_top || Typeof(l) != TYPE_ROOM ||
					(Flags(l) & INHERIT)!= INHERIT)
					break;
				j = l;
			}
		}
	}
}

void translate_thing_class(i)
dbref i;
{	dbref j,l;
	int depth;

	if( Class(i) == 0 || Class(i) == NOTHING){
		if( default_thing_class != NOTHING)
			Class(i) = DEF_THING;
	}else{
		j = Class(i);
		depth = 0;
		while( depth++ < 15){
			if( is_def_class(j) )
				break;
			l = Class(j);
			if( is_def_class(l))
				break;
			if( l <= 0 ){
				Class(j) = DEF_THING;
				break;
			}
			if( l >= db_top || Typeof(l) != TYPE_ROOM ||
				(Flags(l) & INHERIT)!= INHERIT)
				break;
			j = l;
		}
	}
}
