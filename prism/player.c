#include "copyrite.h"

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "externs.h"
#include "fight.h"

extern struct player_levels player_levels_table[];
extern dbref make_new_object();
extern int dbversion;

struct extension *need_ext( p)
dbref p;
{	struct extension *e;

	if( Typeof(p) != TYPE_PLAYER){
		fprintf(logfile,"Need_ext(%d) - not a player\n", p);
		return NULL;
	}

	if( dbversion){
fprintf(logfile,"Need_ext(%d) - dbversion %d\n", p, dbversion);
		return &Ext(p);
	}

	e = OldExt(p);

	if(e)return e;

	e = (struct extension *)malloc(Sizeof(struct extension));
	if(!e){
fprintf(logfile,"Out of memory in need_ext\n");
		panic("Out of memory");
	}
	OldExt(p)   = e;
	e->password = NULL;
	e->level    = 0;
	e->chp	    = 31000;
	e->level    = 0;
	e->robot    = NULL;
#ifdef THREAD
	e->exthread = NULL;
#endif
	return e;
}


int is_rubbish(thing)
dbref thing;
{
	char *p;

	p = (char *)Name(thing);

	if(p[0] == ' ' && p[1] == ' ' &&
	   p[2] == 'R' && p[3] == 'U' &&
	   p[4] == 'B' ){
		return 1;
	}
	return 0;
}
		

struct name_rec {
struct name_rec *l,*r;
char *name;
dbref player;
};

struct name_rec *names;

int astrcmp(a,b)
char *a, *b;
{	char ch,ch1;

	while(*a && *b){
		ch = *a;
		ch1= *b;
		if(ch >= 'A' && ch <= 'Z')ch += 0x20;
		if(ch1 >= 'A' && ch1 <= 'Z')ch1 += 0x20;
		if(ch < ch1)return -1;
		if(ch > ch1)return  1;
		a++;
		b++;
	}
	if(*a < *b)return -1;
	if(*a > *b)return 1;
	return 0;
}

struct name_rec *find_name(name, head)
char *name;
struct name_rec *head;
{
	while(head){
		switch(astrcmp(name, head->name)){
		case -1:
			head = head->l;
			break;
		case 1: head = head->r;
			break;
		default:
			return head;
		}
	}
	return NULL;
}

struct name_rec *add_name(name, head)
char *name;
struct name_rec **head;
{	struct name_rec *n;

	while(*head){
		switch(astrcmp(name, (*head)->name)){
		case -1:
			head = &((*head)->l);
			break;
		case 1:
			head = &((*head)->r);
			break;
		default:
			return *head;	/* found it already */
		}
	}
	n = (struct name_rec *)malloc(Sizeof(struct name_rec));
	if(!n)
		return NULL;
	n->l = n->r = NULL;
	n->name = (char *)alloc_string(name);
	n->player=0;
	*head = n;
	return n;	
}

int add_name_ok(name, player)
char *name;
dbref player;
{	struct name_rec *n;

	if(!ok_player_name(name))
		return 0;

	n = add_name(name, &names);
	if(!n){
fprintf(logfile,"Add_name(%s) failed!\n", name);
		return 0;
	}
	if( n->player <= 0 )	/* A new one */
		n->player = player;
	else if( n->player != player) /* already existed */
		return 0;
	return 1;
}
	

dbref lookup_player(name)
const char *name;
{
	struct name_rec *n = find_name(name, names);

	if(!n)return NOTHING;

	if(bad_dbref(n->player, 0))
		return NOTHING;
	return n->player;
}

/* This routine is used to remove a reference in the name tree 
 * Just clears the player field. This will allow the name to be re-used
 */

int toad_player(name)
char *name;
{
	struct name_rec *n = find_name(name, names);
	
	if( n ){
		n->player = 0; /* mark that its not in use */
		return 1;
	}
	return 0;
}

#ifndef MSDOS
#include <pwd.h>

/***********************************************************************
Copyright 1989 by Martyn Hampson, Imperial College Computer Centre.
************************************************************************/

char *encryptpw(passwd)
char *passwd ;
{

	long temp ;
	int i ;
	unsigned int c ;
	char salt[3] ;
	char *crypt() ;



	temp = 9 * getpid() ;
	salt[0] = temp & 077 ;
	salt[1] = (temp >> 6) & 077 ;
	for (i=0;i<2;i++) {
		c = salt[i] + '.' ;
		if ( c > '9') c += 7 ;
		if ( c > 'Z') c += 6 ;
		salt[i] = c ;
	}

	salt[2] = '\0' ;
	return (crypt(passwd,salt)) ;
}

char *mkpass(pass)
char *pass;
{

	long temp ;
	int i ;
	unsigned int c ;
	char salt[3] ;
	char *crypt() ;
	static char passbuf[20];


	temp =random();
	salt[0] = temp & 077 ;
	salt[1] = (temp >> 6) & 077 ;
	for (i=0;i<2;i++) {
		c = salt[i] + '.' ;
		if ( c > '9') c += 7 ;
		if ( c > 'Z') c += 6 ;
		salt[i] = c ;
	}

	salt[2] = '\0' ;
	passbuf[0] = '$';
	strcpy(&passbuf[1], crypt(pass, salt)) ;
	return passbuf;
}

int chkpass(pass, pass2)
char *pass, *pass2;
{
	char salt[3] ;
	char *crypt() ;
	char *encpw;

	if(*pass2 == '$' || *pass == '$'){
		salt[0] = pass2[1];
		salt[1] = pass2[2];
		salt[2] = '\0';
		encpw = crypt(pass, salt);
		return strcmp(encpw, &pass2[1]);
	}else
		return strcmp(pass, pass2);
}

#else
char *mkpass(pass)
char *pass;
{
	return pass;
}

chkpass( p1, p2)
char *p1, *p2;
{
	return strcmp(p1, p2);
}
#endif

/* Connect a PLAYER 
 * note d == NULL when called from start robot
 */

dbref connect_player( name, password, d)
const char *name, *password;
struct descriptor_data *d;
{	struct extension *e;
	dbref player;

	if(!d || d->connected == 0){
	    if((player = lookup_player(name)) == NOTHING) return NOTHING;
	}else {
		player = d->player;
	}

    if( Typeof(player) != TYPE_PLAYER){
fprintf(logfile,"connect_player: %s(%d) %x is not a player!\n", Name(player), player, Flags(player));
	return NOTHING;
    }

    e = &Ext(player);

    if( e->password && *e->password &&
	(!d || *password || d->connected) &&
	chkpass(password, e->password) )
		 return NOTHING;
    e->level = player_level(player);
    return player;
}

dbref create_player(name, password, d)
const char *name, *password;
struct descriptor_data *d;
{   struct extension *e;
    dbref player, player_start;

	player_start= PLAYER_START;

	if( default_player_start != NOTHING &&
	    Typeof(default_player_start) == TYPE_ROOM)
		player_start = default_player_start;

    if((!ok_player_name(name)&& d->connected == 0) || 
       (!ok_password(password) && !d->connected == 0)) return NOTHING;

    /* else he doesn't already exist, create him */
	if(d->connected == 0){

fprintf(logfile,"Start new player scan...\n");
	player = get_new_object( 1);

	change_object_type(player, TYPE_PLAYER);

	Pennies(player) = 0;

	    if(!add_name_ok(name, player)){
fprintf(logfile,"add_name_ok failed!\n");
		return NOTHING;
	    }

    /* initialize everything */
	NameW(player, name);
   	Location(player) = player_start;
   	Owner(player) = player;
   	Flags(player) = TYPE_PLAYER;

	if( default_player_class != NOTHING)
		Class(player) = default_player_class;

	e = &Ext(player);

	if( *password){
		e->password = (char *)alloc_string(mkpass(password));
		PUSH(player, Contents(player_start));
		}
    }else {
    	player = d->player;

	e = &Ext(player);
   	e->password = (char *)alloc_string(mkpass(password));
    
    /* link him to PLAYER_START */
    		PUSH(player, Contents(player_start));
    }

    return player;
}

void do_password( player, old, newobj)
dbref player;
const char *old, *newobj;
{	struct extension *e;

	if( Typeof(player) != TYPE_PLAYER)
		return;

	e = &Ext(player);

    if(  !e->password || chkpass(old, e->password)) {
	Notify(player, "Sorry");
    } else if(!ok_password(newobj)) {
	Notify(player, "Bad new password.");
    } else {
	
	free((void *) e->password);
fprintf(logfile,"PASSWORD: %s(%d) changed password to %s\n", Name(player),player, newobj);
	e->password = (char *)alloc_string(mkpass(newobj));
	Notify(player, "Password changed.");
    }
}


void check_name_tree()
{	dbref thing;
	struct name_rec *n;

	for(thing = 0; thing < db_top; thing++){
		if( Typeof(thing) == TYPE_PLAYER){
			n = find_name(Name(thing), names);
			if( n && n->player != thing){
fprintf(logfile,"Check_name_tree: %s(%d) != %s(%d)\n",Name(thing), thing,
				Name(n->player),n->player);
			}else if(!n){
fprintf(logfile,"Could not find %s(%d) in name tree\n", Name(thing),thing);
			}
		}
	}
}
 