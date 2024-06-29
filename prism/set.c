#include "copyrite.h"

/* commands which set parameters */
#include <stdio.h>
#include <ctype.h>

#include "db.h"
#include "config.h"
#include "match.h"
#include "interfac.h"
#include "externs.h"

extern dbref euid;
extern int last_result;
extern dbref default_beaker;
extern dbref Check_defaults();

char *prt_money(val)
money val;
{	static char buf[20];
#ifdef MSDOS
	if( val == 1l){
#else
	if( val == 1){
#endif
		sprintf(buf, "a penny");
		return buf;
	}

#ifdef MSDOS
	sprintf(buf, "%ld pennies", val);
#else
	sprintf(buf, "%d pennies", val);
#endif
	return buf;
}


#define N_LOCKS 5

void need_lock( thing, n)
dbref thing;
int n;
{	struct locking *(*l)[], *l2;
	int nlock;

	if( has_lock(thing, n))return;

	if( !(l = Lock(thing))){

		l = (struct locking *(*)[])malloc( sizeof(struct locking*)* N_LOCKS);
		if(!l){
			panic("No memory in need_locking");
		}
		Lock(thing) = l;

		for(nlock = 0 ; nlock < N_LOCKS; nlock++){
			(*l)[nlock] = NULL;
		}
	}
	l2 = (struct locking *)malloc( sizeof( struct locking ) );
	if( l2){
		(*l)[n] = l2;	
		
		l2->key = TRUE_BOOLEXP;
	}
}

dbref find_exit(player, name)
dbref player;
char *name;
{
    init_match(player, name, TYPE_EXIT);
    match_exit();
    match_here();
    match_absolute();	/* Use controls( ) to see if reasonable */
    
    return match_result();
}
 


dbref match_controlled( player, name)
dbref player;
const char *name;
{
    dbref match;

    init_match(player, name, NOTYPE);
    match_everything();

    match = noisy_match_result();
    if(match != NOTHING && !controls(player, match)) {
	denied(player);
	return NOTHING;
    } else {
	if( match == NOTHING){
		notify(player, "Not found.");
	}
	return match;
    }
}

void do_name( player, name, newname)
dbref player;
const char *name;
char *newname;
{
    dbref thing;
    char *password;

    if((thing = match_controlled(player, name)) != NOTHING) {
	/* check for bad name */
	if(*newname == '\0') {
	    Notify(player, "Give it what new name?");
	    return;
	}

	/* check for renaming a player */
	if(Typeof(thing) == TYPE_PLAYER) {
	    /* split off password */
	    for(password = newname; !isspace(*password); password++);
	    /* eat whitespace */
	    if(*password) {
		*password++ = '\0'; /* terminate name */
		while(*password && isspace(*password)) password++;
	    }

	    /* check for null password */
	    if(!*password) {
		Notify(player,
		       "You must specify a password to change a player name.");
		Notify(player, "E.g.: name player = newname password");
		return;
	    } else if( chkpass(password, Ext(thing).password)) {
		Notify(player, "Incorrect password.");
		return;
	    } else {
		if( !add_name_ok(newname, thing)) {
			Notify(player, "You can't give a player that name.");
			return;
		}
		toad_player(Name(thing));	/*free up the name */
		add_name_ok(newname, thing);	/* just in case */
	    }
	    /* everything ok, notify */
	    fprintf(logfile, "NAME CHANGE: %s(%d) to %s\n",
		    Name(thing), thing, newname);
	    /* everything ok, change the name */
#ifndef VM_DESC
	    if(Name(thing) ) {
	        free( (FREE_TYPE) Name(thing));
	    }
#endif
	    NameW(thing, newname);
	    Notify(player, "Name set.");
	    return;

#ifdef Notdef
	} else {
	    if(!ok_name(newname)) {
		Notify(player, "That is not a reasonable name.");
		return;
	    }
#endif
	}

	if( !Builder(player)){
		fnotify(player,"Sorry builders or wizards only");
		return;
	}
	/* everything ok, change the name */
#ifndef VM_DESC
	if(Name(thing) ) {
	    free( (FREE_TYPE) Name(thing));
	}
#endif
	NameW(thing, newname);
	Notify(player, "Name set.");
    }
}

void do_describe( player, name, description)
dbref player;
const char *name, *description;
{
    dbref thing;

    if((thing = match_controlled(player, name)) != NOTHING &&
	Typeof(thing) != TYPE_EXIT) {
#ifdef VM_DESC
	DescW(thing, description);
#else
	if( Desc(thing) ) {
	    free( (FREE_TYPE)  Desc(thing) );
	}
	Desc(thing) = (char *)alloc_string(description);
#endif
	Notify(player, "Description set.");
    }
}

void do_lock( player, name2, keyname)
dbref player;
const char *name2, *keyname;
{
    dbref thing;
    BOOLEXP_PTR key;
    char *name = (char *)name2;

    switch(thing = find_exit(player, name) ) {
      case NOTHING:
	Notify(player, "I don't see what you want to lock!");
	return;
      case AMBIGUOUS:
	Notify(player, "I don't know which one you want to lock!");
	return;
      default:
	if(!controls(player, thing) || !Builder(player) ) {
	    Notify(player, "You can't lock that!");
	    return;
	}
	break;
    }

    if( Typeof(thing) != TYPE_EXIT){
	Notify(player, "You can only lock exit/commands!");
	return;
    }
    if( Contents(thing) == NOTHING){
	Notify(player, "Can't lock an unlinked exit!");
	return;
    }

    key = parse_boolexp(player, keyname);

    if(key == TRUE_BOOLEXP) {
	Notify(player, "I don't understand that key.");
    } else {
	free_boolexp( Key(thing));

 	Key(thing) = key;
	Notify(player, "Locked.");
    }
}

void do_unlock( player, name2)
dbref player;
const char *name2;
{
    dbref thing;
    char *name = (char *)name2;

    if((thing = find_exit(player, name)) != NOTHING) {

	if( Typeof(thing) != TYPE_EXIT){
		Notify(player, "You can only unlock exit/commands!");
		return;
    	}
	if( !Builder(player)){
		Notify(player,"Builders or wizards only!");
		return;
	}

	if( Key(thing) != TRUE_BOOLEXP){
		free_boolexp(Key(thing) );
	}
	Key(thing) = TRUE_BOOLEXP;
	Notify(player, "Unlocked.");
    }
}

void do_unlink( player, name)
dbref player;
const char *name;
{
    dbref exit;

    exit = find_exit(player, name);

    switch(exit) {
      case NOTHING:
	Notify(player, "Unlink what?");
	break;
      case AMBIGUOUS:
	Notify(player, "I don't know which one you mean!");
	break;
      default:
	if(!controls(player, exit) || !Builder(player)) {
	    denied(player);
	} else {
	    if( !is_god(player) && player != Owner(exit) ){
fnotify(player, "Sorry, only God can unlink exits he doesn't own!");
		return;
	    }
	    switch(Typeof(exit)) {
	      case TYPE_EXIT:
		Contents(exit) = NOTHING;
		Notify(player, "Unlinked.");
		free_boolexp( Key(exit));
		Key(exit) = TRUE_BOOLEXP;
		break;
	      default:
		Notify(player, "Only exit/commands are linked!");
		break;
	    }
	}
    }
}

void do_chown( player, name, newobj)
dbref player;
const char *name, *newobj;
{
    dbref thing;
    dbref owner;
    dbref i;

/* @chown cannot be EXEC'ed */

    if(euid != NOTHING || !ArchWizard(player)) {
	denied(player);
	return;
    } else {
	init_match(player, name, NOTYPE);
	match_everything();

	if((thing = noisy_match_result()) == NOTHING) {
	    return;
	} else if((owner = lookup_player(newobj)) == NOTHING) {
	    Notify(player, "I couldn't find that player.");
	} else if(!is_god(player) && Typeof(thing) == TYPE_PLAYER && !Robot(thing)) {
	    Notify(player, "Players always own themselves.");
	} else if( !is_god(player) && TrueWizard(owner) && player != owner){
	    Notify(player, "Sorry, you cannot chown that to them.");
	}else if ( !is_god(player) && TrueWizard(Owner(thing)) && Owner(thing) != player){
		Notify(player, "Sorry, that is owned by another WIZ!");
	} else {

/* This code allows GOD to change the ownership of all stuff owned by
 * one player to be owned by another
 */  
	    if( Typeof(thing) == TYPE_PLAYER){
fprintf(logfile,"Chown: %s(%d) to %s(%d)\n",Name(thing),thing, Name(owner),owner);
		for(i=0; i < db_top; i++){
			if( Owner(i) == thing && i != thing)
				Owner(i) = owner;
			if( Typeof(i) == TYPE_EXIT)
				do_chown_boolexp(Key(i), thing, owner);
		}
	    }else {
		Owner(thing) = owner;
	    }
	    Notify(player, "Owner changed.");
	}
    }
}

struct set_flags{
char *name;
object_flag_type flag;
int room_rank, thing_rank, exit_rank, player_rank;
char *comment;
} flag_table[] = 
{/*              Name           Value        Room rank Thing   Exit    Player*/
	{(char *)" dummy",	0,		5,	5,	5,	5, "Bit C Name .... Room.... Player . Thing .. Exit"},
	{(char *)" dummy",	0,		5,	5,	5,	5, "========================================================"},
	{(char *)"WIZARD",	WIZARD,		4,	1,	1,	4, " 04 W WIZARD .. ........ WIZARD . WORLD .. ........"},
	{(char *)"LINK_OK",	LINK_OK,	3,	1,	1,	3, " 05 L LINK_OK . ........ LISTEN . HEARTBEA LOCAL .."},
	{(char *)"DARK",	DARK,		1,	1,	1,	3, " 06 D DARK .... DARK ... DARK ... DARK ... OBVIOUS."},
	{(char *)"TEMPLE",	TEMPLE,		1,	1,	1,	1, " 07 T TEMPLE .. TEMPLE.. ........ ........ TIMESTAMP"},
	{(char *)"STICKY",	STICKY,		0,	0,	0,	0, " 08 S STICKY .. TITLE/CL SELFDRAW SELFDRAW ........"},
#ifdef RESTRICTED_BUILDING
	{(char *)"BUILDER",	BUILDER,	3,	3,	3,	3, " 09 B BUILDER . ........ BUILDER. ........ ........"},
#endif
	{(char *)"FEMALE",	FEMALE,		3,	3,	1,	3, " 10 F FEMALE .. ........ ........ ........ FAST...."},
#ifdef NO_QUIT
	{(char *)"NO_QUIT",	NO_QUIT,	1,	1,	3,	3, " 11 Q NO_QUIT . NO_QUIT. NO_QUIT. NO_QUIT. ........"},
	{(char *)"INHERIT", 	INHERIT,	1,	3,	1,	3, " 12 I INHERIT . CLASS... ........ INHERIT. INTERNAL"},
#endif
#ifdef WEAPON
	{(char *)"WEAPON",	WEAPON,		3,	3,	3,	3, " 13 A WEAPON .. ........ ........ ........ ........"},
#endif
#ifdef ROBOT
 	{(char *)"ROBOT",	ROBOT,		3,	1,	1,	3, " 14 O ROBOT ... ........ ROBOT .. ........ R-pref"},
#endif
#ifdef CONTAINER
	{(char *)"CONTAINER",	CONTAINER,	3,	1,	3,	3, " 15 H CONTAINER ........ ....... CONTAINE ........"},
#endif
	{(char *)" dummy",	0,		5,	5,	5,	5, " 16 Pseudo Flag used to check if object is logged in."},
	{(char *)" dummy",	0,		5,	5,	5,	5, "====================================================="},
	{(char *)"", 0, 0, 0,	0,	0}
};

void print_flags(player)
dbref player;
{	struct set_flags *t = flag_table;

	while( t->name && *(t->name)){
		notify(player, t->comment);
		t++;
	}
}



void do_set( player, name, flag)
dbref player;
const char *name, *flag;
{
    dbref thing;
    const char *p;
    object_flag_type f;
    struct set_flags *t = flag_table;
    int rr;

    /* find thing */
    if((thing = match_controlled(player, name)) == NOTHING) return;

    /* move p past NOT_TOKEN if present */
    for(p = flag; *p && (*p == NOT_TOKEN || isspace(*p)); p++);

    /* identify flag */
    if(*p == '\0') {
	Notify(player, "You must specify a flag to set.");
	return;
    }else {
	f = 0;
	for(t=flag_table; t->name[0]; t++){
		if( string_prefix(t->name, p)){
			break;
		}
	}
	f = t->flag;
	switch(Typeof(thing)){
	case TYPE_THING:
		rr = t->thing_rank;
		break;
	case TYPE_ROOM:
		rr = t->room_rank;
		break;
	case TYPE_EXIT:
		rr = t->exit_rank;
		break;
	default:
		rr = t->player_rank;
		break;
	}
    }

    if( !f){
	Notify(player, "I don't recognized that flag.");
	return;
    }

#ifdef Notdef
    if(f == CONTAINER){
	if(Typeof(thing) != TYPE_THING){
		Notify(player, "Can only set that on THINGS!");
		return;
	}
    }
    /* check for restricted flag */
    if( !ArchWizard(player)
       && (   (f == WIZARD && Typeof(thing) != TYPE_THING)
	   || (f == FEMALE && Typeof(thing) != TYPE_EXIT)
#ifdef RESTRICTED_BUILDING
	   || f == BUILDER
#endif /* RESTRICTED_BUILDING */
	   || f == TEMPLE
	   || (f == INHERIT && Typeof(thing)!= TYPE_EXIT)
	   || (f == WEAPON && Typeof(thing) != TYPE_THING)
	   || (f == ROBOT && Typeof(thing) != TYPE_EXIT)
	   || (f == LINK_OK && Typeof(thing) != TYPE_EXIT)
	   || (f == DARK && Typeof(thing) != TYPE_EXIT)
          ) ) {
	denied(player);
	return;
    }
/* God only flags */
    if( !is_god(player) &&
	( (f == WIZARD && Typeof(thing) != TYPE_THING) )
    ){
	notify(player,"Only REAL gods can change that flag");
	return;
    }

/* Check for Arch Wiz stuff */
    if( (euid != NOTHING || !ArchWizard(player)) &&
	(f & (BUILDER|ROBOT) ) ){
	notify(player,"Only ArchWizards can change that flag");
	return;
    }

#else
	if( rank_of(player) < rr){
		denied(player);
		return;
	}
	
#endif

    /* else everything is ok, do the set */
    if(*flag == NOT_TOKEN) {
	/* reset the flag */
	if( f == ROBOT && robot_running(player, thing)){
		notify(player,"Must stop robot first");
		return;
	}
	if( f == CONTAINER && Contents(thing) != NOTHING){
		notify(player,"It is not empty!");
		return;
	}
	Flags(thing) &= ~f;
	if( f == LINK_OK && Typeof(thing) == TYPE_THING){
		stop_heart(thing);
	}
	Notify(player, "Flag reset.");
    } else {
	/* set the flag */
	Flags(thing) |= f;
	if( f == LINK_OK && Typeof(thing) == TYPE_THING){
		start_heart(thing);
	}
	Notify(player, "Flag set.");
    }
}

void do_cost(player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
    dbref thing, auth;
    money cost;

    if((thing = match_controlled(player, arg1)) != NOTHING) {
	if(*arg2 >= '0' && *arg2 <= '9')
		cost = atomoney(arg2);
	else if(*arg2 == '-' && arg2[1] >= '0' && arg2[1] <='9')
		cost = -atomoney(&arg2[1]);
	else {
		Notify(player, "You must specify a new cost");
		return;
	}
    }else
	return;

    if( euid != NOTHING)
	auth = euid;
    else
	auth = player;

    switch(Typeof(thing)){
	case TYPE_ROOM:
		if( cost > 0 && cost < db_top && Typeof((dbref)cost) == TYPE_ROOM){
		    	if( !ArchWizard(player) && Owner(cost)!= player){
				Notify(player, "Sorry Arch wizards only");
				last_result = 0;
				return;
    			}
			Zone(thing) = cost;
			sprintf(msgbuf, "Room %s is now in zone %d\n", Name(thing), cost);
			fnotify(player, msgbuf);
		}else if( cost == 0 || cost == NOTHING){
			Zone(thing) = cost;
			sprintf(msgbuf, "Room %s is now in zone %d\n", Name(thing), cost);
			fnotify(player, msgbuf);
		}else{
			Notify(player, "Can only set zone to a zone room!");
			last_result = 0;
			return;
		}
		break;
	case TYPE_PLAYER:
	    	if( !ArchWizard(player) ){
			Notify(player, "Sorry Arch wizards only");
			last_result = 0;
			return;
    		}
		Pennies(thing) = cost;
		Level(thing) = -1;	/* Re-Calc level */
		player_level(thing);
		sprintf(msgbuf, "%s(%d) set %s(%d) to %s",Name(auth),auth,
			Name(thing), thing, prt_money( Pennies(thing)) );
    		fnotify(player, msgbuf);
		break;
	case TYPE_EXIT:
	case TYPE_THING:
		Notify(player, "That does not have a pennies field");
		return;
    }
	
}


void do_time(player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
    dbref thing;
    long cost;

    if(!ArchWizard(player)){
	WizOnly(player);
	return;
    }

    if((thing = match_controlled(player, arg1)) != NOTHING) {
	if(*arg2 >= '0' && *arg2 <= '9')
		cost = atol(arg2);
	else if(*arg2 == '-' && arg2[1] >= '0' && arg2[1] <='9')
		cost = -atol(&arg2[1]);
	else {
		Notify(player, "You must specify a new time");
		return;
	}
    }else{
	return;
    }
    if(cost && arg2 && *arg2)
	Age(thing) = Age(thing) + cost;
    else
	updateage(thing);
    Notify(player, "Time changed");
	
}

int def_class(room)
dbref room;
{	if( bad_dbref(room, 0))
		return 0;
	if( Typeof(room) != TYPE_ROOM)
		return 0;
	if( default_room_class == room)
		return 1;
	if( default_thing_class == room)
		return 1;
	if( default_player_class == room)
		return 1;
	return 0;
}

void do_class( player, name, room_name)
dbref player;
const char *name;
const char *room_name;
{
    dbref thing;
    dbref room;

	if( !Builder(player)){
		Notify(player,"Builders or wizards only!");
		return;
	}


    if( !room_name || !*room_name)
	room = NOTHING;
    else
	room = parse_linkable_room(player, room_name);

    init_match(player, name, NOTYPE);
    match_neighbor();
    match_possession();
    match_me();
    match_here();
    match_absolute();
    match_player();

    if((thing = noisy_match_result()) != NOTHING) {
	switch(Typeof(thing)) {
	  case TYPE_EXIT:
		notify(player, "Can't set class on an Exit");
		return;

	  case TYPE_THING:
	  case TYPE_PLAYER:
	  case TYPE_ROOM:
	    if( !controls(player, thing) || 
		(room > 0 && !def_class(room) && !controls(player, room)) ) {
		denied(player);
	    }else if( room > 0 && 
		(Typeof(room) != TYPE_ROOM || (Flags(room)&INHERIT)==0) ){
		notify(player,"Thats not a valid class!");
		return;
	    } else {
		Class(thing) = room;
		notify(player, "Class set.");
	    }
	    break;

	  default:
	    notify(player, "Internal error: weird object type.");
	    fprintf(logfile, "PANIC weird object: Typeof(%d) = %d\n",
		    thing, Typeof(thing));
	    break;
	}
    }
}

void do_default(player, arg1, arg2)
dbref player;
char *arg1, *arg2;
{	dbref room, *ptr;

	if( !arg1 || !*arg1){
		sprintf(msgbuf, "Default class: room  (%d) = %s", DEF_ROOM, unparse_object(player, default_room_class));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default class: thing (%d) = %s", DEF_THING, unparse_object(player, default_thing_class));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default class: player(%d) = %s", DEF_PLAYER, unparse_object(player, default_player_class));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default wizard        = %s", unparse_object(player, default_wizard));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default player start  = %s", unparse_object(player, default_player_start));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default 'Beaker'      = %s", unparse_object(player, default_beaker));
		notify(player, msgbuf);
		sprintf(msgbuf, "Default temple class  = %s", unparse_object(player, default_temple_class));
		notify(player, msgbuf);
		return;
	}
	if( !is_god(player)){
		notify(player, "Sorry, Gods only!");
		return;
	}

	if( !string_compare(arg1, "room")){
		ptr = &default_room_class;
	}else if( !string_compare(arg1, "thing")){
		ptr = &default_thing_class;
	}else if( !string_compare(arg1, "player")){
		ptr = &default_player_class;
	}else if( !string_compare(arg1, "start")){
		ptr = &default_player_start;
	}else if( !string_compare(arg1, "temple")){
		ptr = &default_temple_class;
	}else if( !string_compare(arg1, "beaker")){
		ptr= &default_beaker;
	        init_match(player, arg2, TYPE_PLAYER);
		match_neighbor();
		match_me();
		match_absolute();
		match_player();

		*ptr = noisy_match_result();
		if( *ptr != NOTHING &&
		    (Typeof(*ptr) != TYPE_PLAYER || (Flags(*ptr) & WIZARD) ) ){
			notify(player, "MUST NOT be a wizard!");
			return;
		}
		notify(player, "Set.");
		return;
	}else if( !string_compare(arg1, "wizard")){
		ptr = &default_wizard;
	        init_match(player, arg2, TYPE_PLAYER);
		match_neighbor();
		match_me();
		match_absolute();
		match_player();

		*ptr = noisy_match_result();
		if( *ptr != NOTHING &&
		    (Typeof(*ptr) != TYPE_PLAYER || !(Flags(*ptr) & WIZARD) ) ){
			notify(player, "Not a wizard!");
			return;
		}
		
		notify(player, "Set.");
		return;
	}else{
		notify(player,"@default room|thing|player|temple = class");
		notify(player,"or @default beaker|wizard = player");
		notify(player,"or @default start = room");
		last_result = 0;
		return;
	}

	room = parse_linkable_room(player, arg2);

	if( room == NOTHING){
		notify(player, "Could not find class.");
		last_result = 0;
		return;
	}
	*ptr = room;
	notify(player, "Set.");
}
