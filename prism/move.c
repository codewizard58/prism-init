#include "copyrite.h"

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "match.h"
#include "externs.h"
#include "fight.h"

extern int no_true;	/* from match.c */
extern int last_result;
extern char *name_anon();

dbref last_matched_exit;	/* remember result of last can_move */

#ifdef DEBUG
extern int debug;
#endif

int dropping;
int has_drop;

#ifndef NO_SUMMON
#define NO_SUMMON FEMALE
#endif

extern void updateage();

int carrying(player,thing)
dbref player,thing;
{	dbref i;

	if(thing == NOTHING)
		return 0;
	for(i= Contents(player); i != NOTHING; i= Next(i) ){
		if(i == thing)
			return 1;
		if( Typeof(i) == TYPE_EXIT)
			continue;
		if( (Flags(i) & CONTAINER) && carrying(i, thing))
			return 1;
	}
	return 0;
}

int move_player(player, from, to, auth)
dbref player,from,to;
dbref auth;
{
	if( to < 0 || to >= db_top){
		fprintf(logfile,"move_player: bad to %d\n", to);
		to = 0;
	}
	enter_room(player, to); 
	return 1;
}

void moveto( what, where)
dbref what, where;
{
    dbref loc;

    /* remove what from old loc */
    if((loc = Location(what) ) != NOTHING) {
	Contents(loc) = remove_first( Contents(loc), what);
    }

    /* test for special cases */
    switch(where) {
      case NOTHING:
	Location(what) = NOTHING;
	return;			/* NOTHING doesn't have contents */
    }

    /* now put what in where */
    if(Typeof(what)!= TYPE_EXIT && Position(what) ){
    }
    PUSH(what, Contents(where) );

    Location(what) = where;
}

void enter_room( player, loc)
dbref player,loc;
{	int mflags = 0;
    dbref old;
    char buf[BUFFER_LEN];

	if( loc == NOTHING){
fprintf(logfile,"Tried to move to location -1 (%s)\n", Name(player));
		return;
	}

    /* get old location */
    old = Location(player);

    /* check for self-loop */
    if(loc != old) {

	if(old != NOTHING) {

		if( !quiet){
			sprintf(buf, "[%s] %s has left", Name(player), name_anon(player));
			send_message(player, old, buf, wizauth());
			mflags |= media_remove(player, old);
		}
	    show_flush(player,old, mflags);
    	mflags = 0;
    	
		Contents(old) = remove_first(Contents(old), player);
		Location(player) = NOTHING;
		moveto(player, loc);

		if( !quiet){
			mflags |= media_add(player, loc);
			send_message(player, loc, "look here", player);
			sprintf(buf, "[%s] %s has arrived", Name(player), name_anon(player));
			send_message(player, loc, buf, wizauth());
		}
		updateage( loc);
	}
    }

/* Do Heal */
/* 12-AUG-92 make healing a bit slower */

    if( Typeof(player) == TYPE_PLAYER && random()%10 == 0){
	if( Ext(player).chp < Mhp(player))
		Ext(player).chp++;
    }

    /* check for pennies */
    if(!controls(player, loc)
       && Pennies(player) <= MAX_PENNIES
       && random() % PENNY_RATE == 0) {
	notify(player, "You found a penny!");
	Pennies(player)++;
    }

    if( random() % 12 == 0){		/* Check health for sickness */
	if( health(player) == 0){
		sprintf(buf, ".sick %s", Name(player), name_anon(player));
		send_message(player, player, buf, wizauth());
	}
    }
    show_flush(player,loc, mflags);
}
	    
int can_move( player, direction)
dbref player;
const char *direction;
{
	init_match_check_keys(player, direction, TYPE_EXIT);

/* 12-AUG-92 make @xxx inbuilts only again */
	if( direction && *direction == '@'){
		restore_match_vars();
		return 0;
	}

	match_exit();
	return some_matched();
}

char *select_str(a, b , c)
int a;
char *b, *c;
{
	if( a)return b;
	return c;
}

void do_move_sub(player, exit, direction)
dbref player, exit;
char *direction;
{	dbref loc, dest;

	loc = Location(player);

	switch(exit) {
	  case NOTHING:
	    {
	        notify(player, "You can't go that way.");
		last_result = 0;	/* It failed */
	    }
	    break;
	  case AMBIGUOUS:
	    notify(player, "I don't know which way you mean!");
	    last_result = 0;
	    break;
	  default:
	    /* we got one */
	    /* check to see if we got through */

	    loc = Location(player);
	    dest = Contents(exit);

	    if( !no_true){	/* the exit is OK */
		if( dest > 0){
/* Do the player move */
			move_player(player, loc, dest, Level(player));

		}else {	/* its a command really */
		}

		updateage(exit);

	    }else {		/* look for fail message! */
		last_result = 0;	/* Failed */
/* 14/2/93 */
		if( Flags(exit)&TEMPLE){
			updateage(exit);
fprintf(logfile,"Time stamping exit %d\n", exit);
		}
	    }
	    break;
	}
}

#ifdef NO_QUIT

/* This routine is called when a player exits */

void do_no_quit(player)
dbref player;
{
	dbref thing;
	dbref loc, next;
	int qs = quiet;
	dbref auth =1;
	char *buf = NULL;
	struct dbref_chain *t, *t1;

	if( bad_dbref(player, 0)){
fprintf(logfile,"do_no_quit: bad player = %d\n", player);
		return;
	}

	auth = wizauth();

	loc = loc_of(player);
	if( bad_dbref(loc, 0)){
fprintf(logfile,"do_no_quit: bad player location for %d - %d\n", player, loc);
		return;
	}

	if(ArchWizard(player))
		return;

	quiet = 1;
	t = t1 = make_chain(Contents(player));

	buf = (char *)malloc(1024);
	if( !buf)
		return;
	for( ;t1 ; t1 = t1->next){
		thing = t1->this;
		if( bad_dbref(thing, 0)){
			fprintf(stderr,"do_no_quit(%d) bad contents %d %d\n", player, Contents(player), thing);
			goto done;
		}

		if( Typeof(thing) != TYPE_EXIT && (Flags(thing) & NO_QUIT)){
				moveto(thing, loc);
				sprintf(buf, ".home %s", Name(thing));
				send_message(thing, thing, buf, auth);
		}
	}

/* Check NO_QUIT on room */
done:
	free_chain(t);
	t = t1 = NULL;	

	if( Flags( loc) & NO_QUIT){
fprintf(logfile,"sending player home\n");
		t1 = t = make_chain(Contents(player));

		for( ;t1 ; t1 = t1->next){
			thing = t1->this;
			if( Typeof(thing) != TYPE_EXIT ){
					moveto(thing, loc);
			}
		}
		sprintf(buf, ".home %s", Name(player));
		send_message(player, player, buf, auth);
	}
	stop_bot(player, NOTHING);
	quiet = qs;
	if( buf)
		free( (FREE_TYPE) buf);
}
#endif

