#include "copyrite.h"

/* Wizard-only commands */
/* And Arch Wizards */

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "match.h"
#include "externs.h"

#ifndef NO_SUMMON
#define NO_SUMMON FEMALE
#endif

#ifndef BEAKER
#define BEAKER 2
#endif

#ifndef POOL_SIZE
#define POOL_SIZE 1000
#endif

extern int nstrings;
extern long strsize;
extern long send_msg_count;
extern long match_counts;
extern int last_result;
extern char *actime();

long cmp_char_count;
long cmp_exit_count;
long cmp_star_count;
long eval_bool_count;


#ifdef VM_DESC
#ifdef MSDOS
extern long desc_got;
#else
extern int desc_got;
#endif
#endif

#ifdef TEST_MALLOC
#ifdef MSDOS
extern long mem_malloced;
extern long mem_malloccnt;
#else
extern int mem_malloced;
extern int mem_malloccnt;
#endif
#endif

extern long ageof();


int bad_dbref(i, ok)
dbref i, ok;
{	
	if( i != ok && (i < 0 || i >= db_top))
		return 1;
	return 0;
}

/* fnotify
 *  send message to player and log it 
 */

void fnotify(player, buf)
dbref player;
char *buf;
{
fprintf(logfile,"%s\n", buf);
	notify(player, buf);
}

/* 
 * Make a pool of chain objects to save memory 
 */

struct dbref_chain *chain_pool;

struct dbref_chain *alloc_chain()
{	struct dbref_chain *c;
	int i;

	if( !chain_pool){		/* pool is empty */
		chain_pool = (struct dbref_chain *)malloc( Sizeof(struct dbref_chain)*POOL_SIZE);
		if( !chain_pool){
			abort_msg("No mem in alloc_chain\n");
		}
fprintf(logfile,"Add %d to chain pool\n", POOL_SIZE);
		c = chain_pool;
		chain_pool = NULL;
		for(i=0; i < POOL_SIZE;i++){
			c->next = chain_pool;
			chain_pool = c;
			c++;
		}
	}

	c = chain_pool;
	chain_pool = c->next;
	c->next = NULL;
	c->this = NOTHING;
	return c;
}


struct dbref_chain *add_chain( c, i)
struct dbref_chain *c;
dbref i;
{	struct dbref_chain *t1;

	t1 = alloc_chain();
	if( t1){
		t1->next = c;
		t1->this = i;
	}
	return t1;
}

struct dbref_chain *make_chain( i)
dbref i;
{	struct dbref_chain *t, *t1;
	dbref j = i;
	int depth = 0;

	for(t = NULL; i != NOTHING && depth++ < 2000; i=Next(i)){
		if( is_rubbish(i))
			continue;	/* Never count RUBBISH items */
		t = add_chain( t, i);
		if(!t){
fprintf(logfile,"No memory in make_chain(%d)\n", j);
			return NULL;
		}
	}
	return t;
}

void free_chain(t)
struct dbref_chain *t;
{	struct dbref_chain *t1;

	while(t ){
		t1 = t->next;
		t ->next = chain_pool;
		chain_pool = t;
/*		freemsg( (FREE_TYPE) t, "free_chain"); */
		t = t1;
	}
}

int not_in_chain(chain, i)
struct dbref_chain *chain;
dbref i;
{
	while(chain){
		if( chain->this == i)
			return 0;
		chain = chain->next;
	}
	return 1;
}

/* Table of known GODS */

dbref known_gods [] = 
{
	  1,
	  0
};

int is_god(player)
dbref player;
{	dbref *t = known_gods;

	if(euid != NOTHING)return 0;

	while(*t && *t != player)t++;
	return (int)*t;
}

int rank_of( p)
dbref p;
{	dbref zone;
	dbref sp = p;

	if( bad_dbref(p, 0) )
		return 0;

	if(euid != NOTHING)
		p = euid;

	if(!Player(p))return 0;		/* Have to be a player */

	if( is_god(p))return 4;

	if( Flags(p) & WIZARD)return 3; /* Arch Wizard */

	if( Flags(p) & BUILDER)return 1;

	return 0;
}

int ArchWizard(p)
dbref p;
{	
	if( bad_dbref(p, 0))
		return 0;
	if(!Player(p))return 0;

	if( is_god(p))
		return 4;

	if( euid != NOTHING ){
		if( (Flags(euid) & WIZARD) ) /* Pseudo Arch */
			return 3;
	}else if( Flags(p) & WIZARD){
		return 3; /* Arch Wizard */
	}
	return 0;
}

int do_tel_sub(player,victim,destination, auth)
dbref player, victim, destination, auth;
{	int mflags = 0;
	int depth = 0;
	dbref loc;

	if( Typeof(player) != TYPE_PLAYER){
		player = default_beaker;
	}
	if( player < 1){
		return 0;	/* Failed! */
	}

	if(Typeof(victim) == TYPE_ROOM){
failed:
		sprintf(msgbuf,"%d tried to MOVE %d to %d\n", player, victim, destination);
		fnotify(player, msgbuf);
		return 0;
	}

	if( Typeof(victim) == TYPE_EXIT && Contents(victim) != NOTHING){
		sprintf(msgbuf,"%d tried to MOVE a linked exit %d to %d\n",player, victim, destination);
		fnotify(player, msgbuf);
		return 0;
	}

	/* check victim, destination types, teleport if ok */
	while( !bad_dbref(destination, 0) && !can_contain(destination) && depth++ < 10 && destination != victim){
		destination = Location(destination);
	}

/* Players can only teleport to rooms. */
	if(Typeof(victim)== TYPE_PLAYER){
		depth = 0;
		while(!bad_dbref(destination, 0) && 
			Typeof(destination)!= TYPE_ROOM && depth++ < 10)
			destination = Location(destination);
	}

	loc = Location(victim);

	if( depth >= 10){
fprintf(stderr,"Found depth loop in do_tel_sub(%d, %d, %d, %d)\n", player, victim, destination, auth);
		destination = 0;
	}
	if( destination == victim){
sprintf(msgbuf, "Can't move %d into itself", victim);
fnotify(player, msgbuf);
		return 0;
	}
/* check that a player can only be teleported to a room */
	if(bad_dbref(destination, 0) || bad_dbref(loc,0)
	   || Typeof(destination) == TYPE_EXIT
	   || !can_contain(destination)
	   || (Typeof(victim)== TYPE_PLAYER && Typeof(destination) != TYPE_ROOM)
	) {
		goto failed;
	} else {
/* Done basic validations */
		switch(Typeof(victim)){
		case TYPE_PLAYER:
			if( ArchWizard(player))
				break;
			if( auth == Owner(loc) &&
			    auth == Owner(destination))
				break;
			denied(player);
			return 0;
/* you can move anything you own to/or from anywhere you own */
		case TYPE_THING:
			if( is_rubbish(victim)){
fnotify(player,"Cant @tel/MOVE Rubbish!");
				return 0;
			}
			if( ArchWizard(player))
				break;
			if( auth == Owner(victim) 
                         && (auth == Owner(destination) || auth == Owner(loc)))
				break;
			denied(player);
			return 0;

		case TYPE_EXIT:
			if( ArchWizard(player))
				break;
			if( auth == Owner(victim) && auth == Owner(destination) )
				break;
		/* Fall through */
		case TYPE_ROOM:
			denied(player);
			return 0;
		}
mflags |= media_remove(victim, loc);
show_flush(victim, loc, mflags);
mflags = 0;
    		moveto(victim, destination);
mflags |= media_add(victim, destination);
show_flush(victim, destination, mflags);
	}
	return 1;
}

void do_teleport( player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
    dbref victim;
    dbref destination;
    const char *to;

    /* get victim, destination */
    if(*arg2 == '\0') {
	victim = player;
	to = arg1;
    } else {
	init_match(player, arg1, NOTYPE);
	match_neighbor();
	match_possession();
	match_me();
	match_absolute();
	match_player();

	if((victim = noisy_match_result()) == NOTHING) {
		notify(player,"Cant find victim");
	    return;
	}
	to = arg2;
    }

    /* get destination */
    init_match(player, to, TYPE_ROOM);
    match_here();
    match_absolute();
	match_neighbor();
	match_me();
	match_player();

    switch(destination = match_result()) {
      case NOTHING:
	Notify(player, "Send it where?");
	break;
      case AMBIGUOUS:
	Notify(player, "I don't know which destination you mean!");
	break;
      default:
	if( !do_tel_sub(player, victim, destination, player)){
		Notify(player, "Teleport failed!");
		last_result = 0;
	}else{
   		Notify(player, "Teleported.");
		last_result = 1;
	}
    }
}


extern int maxticks;

void do_stats( player, name)
dbref player;
const char *name;
{
    int rooms;
    int exits;
    int things;
    int players;
    int total;
    dbref i;
    int owner = NOTHING;
    int	locks;
    long storage;
    int rubbish;

    if(!ArchWizard(player)) {
	sprintf(msgbuf, "The universe contains %d objects.", db_top);
	Notify(player, msgbuf);
	return;
    } else {
	owner = lookup_player(name);
	rubbish = locks = total = rooms = exits = things = players = 0;
	storage = 0l;
	for(i = 0; i < db_top; i++) {
	    storage = storage+sizeof(struct object *)+sizeof(struct object);
	    if(owner == NOTHING || owner == Owner(i)) {
		total++;
		switch(Typeof(i)) {
		  case TYPE_ROOM:
		    rooms++;
		    break;
		  case TYPE_EXIT:
			if( Key(i)){
				locks++;
			}
		    exits++;
		    break;
		  case TYPE_THING:
		    if( is_rubbish(i))
			rubbish++;
		    else
			things++;
		    break;
		  case TYPE_PLAYER:
		    players++;
		    break;
		  default:
fprintf(logfile,"Bad object type %d for object %d\n", Typeof(i), i);
		    abort();
		    break;
		}
	    }
	}
	sprintf(msgbuf,
		"%d objects = %d rooms, %d exits, %d things, %d players, %d locks.",
		total, rooms, exits, things, players, locks);
	Notify(player, msgbuf);
	sprintf(msgbuf, "%d recycled objects", rubbish);
	Notify(player, msgbuf);
	sprintf(msgbuf, "%d strings, %ld bytes.", nstrings, strsize);
	Notify(player, msgbuf);
#ifdef VM_DESC
#ifdef MSDOS
	sprintf(msgbuf, "get_desc has read %ld bytes in.", desc_got);
#else
	sprintf(msgbuf, "get_desc has read %d bytes in.", desc_got);
#endif
	notify(player,msgbuf);
#endif
	show_text_stats(player);

	sprintf(msgbuf,"Maxticks = %d",maxticks);
	notify(player, msgbuf);

	sprintf(msgbuf, "Boolexp_count = %ld", eval_bool_count);
	notify(player, msgbuf);

	notify(player,"Match statistics");
	sprintf(msgbuf,"cmp_char_counts = %ld",cmp_char_count);
	notify(player, msgbuf);

	sprintf(msgbuf,"cmp_star_counts = %ld", cmp_star_count);
	notify(player, msgbuf);

	sprintf(msgbuf,"cmp_exit_counts = %ld", cmp_exit_count);
	notify(player, msgbuf);

	sprintf(msgbuf,"send_msg count = %ld", send_msg_count);
	notify(player, msgbuf);
	sprintf(msgbuf,"match count = %ld", match_counts);
	notify(player, msgbuf);
#ifdef TEST_MALLOC
	sprintf(msgbuf,"Memory used -----");
	notify(player, msgbuf);
#ifdef MSDOS
	sprintf(msgbuf,"%ld blocks allocated", mem_malloccnt);
	notify(player, msgbuf);
	sprintf(msgbuf,"Total Malloced memory = %ld", mem_malloced);
	notify(player, msgbuf);
#else
	sprintf(msgbuf,"%d blocks allocated", mem_malloccnt);
	notify(player, msgbuf);
	sprintf(msgbuf,"Total Malloced memory = %d", mem_malloced);
	notify(player, msgbuf);
#endif
#endif
    }
}
		
void do_toad( player, name)
dbref player;
const char *name;
{
    dbref victim, thing;
    char nambuf[128];
    struct dbref_chain *con, *con1;

    if( !ArchWizard(player) ){
	WizOnly(player);
	return;
    }

    init_match(player, name, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    match_me();
    if((victim = noisy_match_result()) == NOTHING) return;

    euid = NOTHING;
   
    if( ArchWizard(victim)) {
	notify(player, "You cannot toad a wizard!");
	return;
    }

    if(Typeof(victim) != TYPE_PLAYER || (Flags(victim) & ROBOT)) {
	Notify(player, "You can only turn players into toads!");
    } else {
	/* we're ok */
	con1 = con = NULL;
	if( Contents(victim) != NOTHING){
		con1 = con = make_chain(Contents(victim));
		while(con){
			thing = remove_first(Contents(victim), con->this);
			Contents(victim) = thing;
			Location(con->this) = NOTHING;

			if(Typeof(con->this) == TYPE_EXIT){
				Contents(con->this) = NOTHING;	/* Unlink */
				if( Key(con->this)){
					free_boolexp(Key(con->this));
					Key(con->this) = TRUE_BOOLEXP;
				}
				moveto(con->this, player);
				Owner(con->this)= player;
			}else{
				moveto(con->this, Location(player));
			}
			con = con->next;
		}
	}
	free_chain(con1);
	/* do it */
	/* Notify people */
	sprintf(nambuf,"%.80s", Name(victim));
	Notify(victim, "You have been turned into a toad.");
/*	notify3(player, "You turned ", nambuf, " into a toad."); */
	notify(player,"Toaded.");
fprintf(logfile,"%s turned %s into a toad!\n", Name(player), nambuf);

	clean_object(victim);

	for(thing = 3; thing < db_top;thing++){
		if( Owner(thing) == victim){
			Owner(thing) = 2;	/* give all to beaker */
			if(default_beaker != NOTHING)
				Owner(thing) = default_beaker;
		}
	}


	if(toad_player(nambuf ) )
		notify(player,"Name freed OK!");
	else{
		notify(player,"Couldnt find name");
		return;
	}

	change_object_type(victim, TYPE_THING);
	Flags(victim) = TYPE_THING;

	/* reset name */
	sprintf(msgbuf, "a slimy toad named %s", nambuf);
	NameW(victim, msgbuf);
	Class(victim) = default_thing_class;
    }	/* else */
}

void do_newpassword( player, name, password)
dbref player;
const char *name, *password;
{
    dbref victim;
	char *mkpass();

    if( !TrueWizard(player)) {
	Notify(player, "Your delusions of grandeur have been duly noted.");
	return;
    } else if((victim = lookup_player(name)) < 1) {
	Notify(player, "No such player.");
    } else if(*password != '\0' && !ok_password(password)) {
	/* Wiz can set null passwords, but not bad passwords */
	Notify(player, "Bad password");
    } else if( is_god(victim)|| 
    	(TrueWizard(victim) && !is_god(player))){
			Notify(player, "Cant do that!");
    } else {
	/* it's ok, do it */
	if(Ext(victim).password) 
		free( (FREE_TYPE) Ext(victim).password);
fprintf(logfile,"NEWPASSWORD: %s(%d) changed %s(%d) password to %s\n", Name(player), player,
		Name(victim), victim, password);
	Ext(victim).password = (char *)alloc_string(mkpass(password));
	Notify(player, "Password changed.");
	sprintf(msgbuf, "Your password has been changed by %s.", Name(player) );
	Notify(victim, msgbuf);
    }
}


char * Name_of(player)
dbref player;
{	if( player >= 0 && player < db_top &&
	    Typeof(player) == TYPE_PLAYER)
		return (char *)Name(player);
	return (char *)"God";
}


void at_zone(player, arg1, arg2)
dbref player;
char *arg1, *arg2;
{	dbref zonehome, thing, owner;
	int cost, depth=0;
	int zone_here=zoneof(player);

	if( !arg1 || !*arg1){
	    if( zone_here >= 0 && zone_here < db_top){
		sprintf(msgbuf, "You are in zone %s owned by %s",
			zone_name(player), Name_of(Owner(zone_here)) );
		notify(player, msgbuf);
		thing = zoneof((dbref)zone_here);
		while( depth++ < 20 && thing >= 0 && thing < db_top &&
			 Typeof(thing)==TYPE_ROOM && (Flags(thing)&INHERIT) ){
			sprintf(msgbuf,"which is in %s owned by %s",
				Name(thing), Name_of(Owner(thing)));
			notify(player, msgbuf);
			thing = zoneof(thing);
		}
		return;
	    }
	    notify(player,"You are out of this world!");
	    return;
	}
	if(!is_god(player)){
		notify(player,"Just use @zone");
		return;
	}

	if(arg1 && arg2 && *arg1 && *arg2){
		init_match(player,arg1,TYPE_ROOM);
		match_absolute();
		zonehome=match_result();

		if(zonehome != NOTHING && Typeof(zonehome)== TYPE_ROOM &&
			(Flags(zonehome)&INHERIT) ){
			owner = lookup_player(arg2);
			if(owner == NOTHING)
				cost = atoi(arg2);
			else
				cost = NOTHING;
			for(thing=4; thing < db_top;thing++){
				if(Typeof(thing) == TYPE_ROOM &&
					(Zone(thing) == cost || Owner(thing) == owner)
					&& !(Flags(thing)&INHERIT)){
					Zone(thing) = zonehome;
					if(cost != NOTHING)
fprintf(logfile,"%s changes zone of %s(%d) from %d to %s\n", Name(player),
			Name(thing), thing, cost, Name(zonehome));
					else
fprintf(logfile,"%s changes zone of %s(%d) owned by %s to %s\n", Name(player),
			Name(thing), thing, Name(owner), Name(zonehome));
				}
			}
		}
	}else {
		notify(player,"Expected 2 arguments");
		return;
	}
	notify(player,"Done.");
}

/* Top 10 list */

struct ten {
dbref	who;
money   pennies;
int	lev;
};

struct ten week0[10] = 
{
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0},
	{ -1, 0, 0}
};


void do_ten(player)
dbref player;
{	int i;
	
	notify(player,"Current top ten players");
	notify(player,"-----------------------");

	for(i=0; i < 10; i++){
		if( bad_dbref(week0[i].who, 0))
			break;
		sprintf(msgbuf,"%2d. %2d %s %s", i+1, week0[i].lev, player_title(week0[i].who),Name(week0[i].who));
		notify(player, msgbuf);
	}
	notify(player,"-----------------------");
}

init_top()
{	int i;

	for(i=0; i < 10; i++)
		week0[i].who = NOTHING;
}

check_top(player)
dbref player;
{	int i,j;
	money penny;
	int levl;

	if( ageof(player) > 3600l*24*7l)
		return 0;
	penny = Pennies(player);
	levl = Level(player);

	for(i=0; i< 10; i++){
		if( week0[i].lev < levl || 
			(week0[i].lev == levl && week0[i].pennies < penny)){
			for(j=9; j > i; j--){
				week0[j] = week0[j-1];
			}
			week0[i].who = NOTHING;
		}
		if( week0[i].who == NOTHING){
			week0[i].who = player;
			week0[i].pennies = penny;
			week0[i].lev = levl;
			return 1;
		}
	}
	return 0;
}


/* The NOQUIT scan is really the @reset routine. 
 * It checks various things for consistency then does
 * the NOQUIT and .reset stuff.
 */

void do_noquit_scan()
{
	dbref e,p;	/* used to check */
	dbref thing, temp;
	int qs,cnt;
	char *buf = NULL;
	dbref auth = 1;
	struct dbref_chain *t = NULL, *t1;

	if( default_wizard > 1)
		auth = default_wizard;

	buf = (char *)malloc( 2050);
	if( !buf ){
fprintf(logfile,"No mem in do_noquit_scan\n");
		return;
	}
fprintf(logfile,"Start @reset %s\n", actime(&timenow));

fprintf(logfile,"Start scan up to %d with auth=%d\n", db_top, auth);
	for(thing = 0; thing < db_top; thing++){
		if( is_rubbish(thing) ){
			p = Location(thing);
			if(bad_dbref(p, 0))
				continue;
fprintf(logfile,"Removing rubbish %d from %d\n",thing, p);
			temp = remove_first(Contents(p), thing);
			Contents(p) = temp;
			Location(thing) = NOTHING;
			Next(thing) = NOTHING;
			continue;
		}
		if( bad_dbref(Owner(thing), 0)){
			if( default_beaker > 0)
				Owner(thing) = default_beaker;
			else
				Owner(thing) = BEAKER;
		}
		if( bad_dbref(Next(thing), NOTHING) )
			Next(thing) = NOTHING;
		if( bad_dbref(Contents(thing) , NOTHING))
			Contents(thing) = NOTHING;
		if( bad_dbref(Location(thing), NOTHING))
			Location(thing) = 0;
	}

/* check objects that might not be in any room */
fprintf(logfile,"Put objects where they think they are\n");
	for(thing=0; thing < db_top; thing++){
		if( Typeof(thing) != TYPE_EXIT){
			cnt = 0;
			for(temp = Contents(thing); !bad_dbref(temp, 0) && cnt++ < 1000; temp = Next(temp)){
				if( Typeof(temp) == TYPE_EXIT){
					if(Location(temp)!= NOTHING && 
					   Location(temp)!= thing){
fprintf(logfile,"Exit %d not in %d\n", temp, thing);
					}
					Location(temp) = thing;
				}
			}
		}
	}
	for(thing=0; thing < db_top; thing++){
		if(Typeof(thing) != TYPE_ROOM){
			if( is_rubbish(thing))
				continue;	/* Ignore rubbish items */
			p = Location(thing);
			if( !bad_dbref(p, 0)){
				temp = remove_first( Contents(p), thing);
				Contents(p) = temp;
				Next(thing) = NOTHING;
				Location(thing) = NOTHING;
				if( Typeof(p) == TYPE_EXIT){
fprintf(logfile,"Bad location for %d in %d\n", thing, p);
					p = 0;
				}else if( Typeof(p) != TYPE_ROOM && Typeof(thing) == TYPE_PLAYER){
fprintf(logfile,"Bad location for player %d is %d\n", thing, p);
					p = 0;
				}
			}else{
fprintf(logfile,"Bad dbref in location for %d\n", thing);
				p = 0;
			}
			if( thing == p){
fprintf(logfile,"%d was in itself!\n", thing);
				p = 0;
			}
			moveto(thing, p);
		}else if( Typeof(thing) != TYPE_ROOM ){
fprintf(logfile,"No quit: Bad object flags %x for %d\n", Flags(thing), thing);
		}
	}

fprintf(logfile,"Scan Contents\n");
	for(thing = 0; thing < db_top; thing++){
		if( is_rubbish(thing) ){
			Next(thing) = NOTHING;
			Location(thing) = NOTHING;
			Contents(thing) = NOTHING;
			continue;
		}
		if( Typeof(thing) == TYPE_EXIT)
			continue;
		t = NULL;
		for( e = Contents(thing); e != NOTHING; e = Next(e)){
			if( is_rubbish(thing))
				break;
			if( e != thing && not_in_chain(t, e)){
				t = add_chain(t, e);
			}else
				break;
		}
		t1 = t;
		Contents(thing) = NOTHING;
		while(t1){
			if(Location(t1->this) == thing){
				Next(t1->this) = Contents(thing);
				Contents(thing) = t1->this;
			}else {
fprintf(logfile,"%d not in %d\n", t1->this, thing);
			}
			t1 = t1->next;
		}
		free_chain(t);
		t = NULL;
	}


fprintf(logfile,"Start to NOQUIT rooms and players\n");
	for(thing = 0; thing < db_top; thing++){
		if( Flags(thing) & NO_QUIT){
			switch(Typeof(thing)){
			case TYPE_THING:
				break;
			case TYPE_ROOM:
				if( Contents(thing) == NOTHING)
					break;
				t1 = t = make_chain(Contents(thing));
				
				for(;t1 ; t1= t1->next ){
					e = t1->this;
					if(Typeof(e) == TYPE_PLAYER){
qs = quiet;
quiet = 0;
						notify(e, "The clock chimes and the world shakes!");
						notify(e, "The clock chimes and the world shakes!");
						notify(e, "The clock chimes and the world shakes!");
quiet = qs;
#ifdef Notdef	/* .home should do a drop all anyway */
						send_message(e, e, "drop all", auth);
#endif
						sprintf(buf, ".home %s", Name(e));
						simple_send_message(e, e, buf, auth);
					}
				}
				free_chain(t);
				break;
			case TYPE_PLAYER:
/* send robots home */
				if( Flags(thing) &ROBOT){
					sprintf(buf, ".home %s", Name(thing));
					simple_send_message(thing, thing, buf, auth);
				}
				break;
			}
		}
	}

fprintf(logfile,"No quit things\n");
	for(thing = 0; thing < db_top; thing++){
		if( Flags(thing) & NO_QUIT){
			switch(Typeof(thing)){
			case TYPE_THING:
				if(!is_rubbish(thing)){
					sprintf(buf,  ".home %s", Name(thing));
					simple_send_message(1, thing, buf, auth);
				}
				break;
			case TYPE_ROOM:
				break;
			}
		}
	}

fprintf(logfile,"Start reset scan\n");
	for(thing = 0; thing < db_top; thing++){
		switch(Typeof(thing)){
		case TYPE_THING:
			if(!is_rubbish(thing)){
				sprintf(buf,  ".reset %s", Name(thing));
/* Send .reset to things with their owners auth */
				simple_send_message(thing, thing, buf, Owner(thing));
			}
			break;
		case TYPE_ROOM:
			if( !(Flags(thing)&INHERIT) ){
				sprintf(buf, ".reset %s", Name(thing));
				simple_send_message(thing, thing, buf, auth);
			}
			break;
		case TYPE_PLAYER:
			sprintf(buf,".reset %s", Name(thing));
			simple_send_message(thing, thing, buf, auth);
			break;
		}
	}

	init_top();
	for(thing=0; thing < db_top; thing++){
		if( Typeof(thing) == TYPE_PLAYER){
			if( Flags(thing) & WIZARD){
fprintf(logfile,"Wizard %s(%d)\n", Name(thing), thing);
			}else {
				if( !(Flags(thing)&ROBOT)){
					check_top(thing);
				}
			}
		}
	}

	time(&timenow);
#ifdef MSDOS
	timenow = timenow -0x83abcd48l;
#endif
fprintf(logfile,"Finished @reset %s\n", actime(&timenow));

	check_name_tree();

	if(buf)
		free( (FREE_TYPE) buf);
}

int debug;

void do_debug(player,arg1, arg2)
dbref player;
char *arg1,*arg2;
{	
	if( !is_god(player) || euid != NOTHING){
		notify(player, "Ignored");
		return;
	}
	if( !arg1 || !*arg1)
		debug = 0;
	else
		debug = atoi(arg1);
}

void do_reset(player)
dbref player;
{
	if( !ArchWizard(player)){
		return;
	}
	do_noquit_scan();
}
