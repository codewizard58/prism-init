#include "copyrite.h"

/* Commands which involve speaking */

#include "db.h"
#include "interfac.h"
#include "match.h"
#include "config.h"
#include "externs.h"

extern dbref euid;
char *ranmsg( PROTO2(char **, int));
void do_shout();
void notify_all_except();
void do_broad();
extern int quiet;
static int noshout = 0;

dbref shout_zone = NOTHING;			/* zone you are shouting from */

dbref location_of(thing)
dbref thing;
{
	int depth=0;
	while( depth++ < 20 && thing != NOTHING &&
		Typeof(thing)!= TYPE_PLAYER && Typeof(thing) != TYPE_ROOM)
		thing = Location(thing);

	if(depth >= 20)
		fprintf(logfile,"Contents loop for %d\n", thing);

	return thing;
}

void check_message(buf)
char *buf;
{
	while(*buf){
		switch(*buf){
		case '\r':
		case '\n':
			*buf++ = '\n';
			break;
		case '\0':
			break;
		case '\177':
		case '\377':
			*buf++ = ' ';
			break;
		default:
			if( *buf < ' ')
				*buf = ' ';
			buf++;
			break;
		}
	}
}


/* this function is a kludge for regenerating messages split by '=' */
char *reconstruct_message(arg1, arg2)
const char *arg1, *arg2;
{
    static char buf[BUFFER_LEN];

    if(arg2 && *arg2) {
	strcpy(buf, arg1);
	strcat(buf, " = ");
	strcat(buf, arg2);
	return buf;
    } else {
	return (char *)arg1;
    }
}

char *name_anon(player)
dbref player;
{	static char nambuf[80];
	char *nam, *pnam;

	pnam = (char *) Name(player);

	if( !Player(player))
		return pnam;

	if( Dark(player))return (char *)"someone";

	nam = player_title(player);
	if( nam && *nam){
		sprintf(nambuf, "%.30s%.46s", nam, pnam);
	}else {
		strcpy(nambuf, pnam);
	}
	return nambuf;
}

void do_broad(player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
	const char *message;
	int qs = quiet;

	message = reconstruct_message(arg1, arg2);

	if( ArchWizard(player) ){
		quiet = 0;
		notify( player, message);
		shout_zone = NOTHING;
		notify_all_except(player, message);
	}
	quiet = qs;
}

/* Local Broadcast */
/* If room has a class then broadcast message in all rooms that are
 * of that class. Else if the room is a Zone room, broad to all in
 * that zone. Else broad to all in the same zone as player.
 */
void do_local_broad(player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
    const char *message;
    int qs = quiet;
    dbref class, loc;

    loc = Location(player);

	if( Flags(loc)&INHERIT)
		shout_zone = loc;
	     else
		shout_zone = zoneof(player);
/* Allow local broadcast if you control the shout zone. */

    if(ArchWizard(player) || controls(player, shout_zone)){
	message = reconstruct_message(arg1, arg2);
	quiet = 0;
/*	notify( player, message); */
	notify_all_except(player, message);
    }
    shout_zone = NOTHING;
    quiet = qs;
}


void do_gripe( player, arg1, arg2)
dbref player;
const char *arg1, *arg2;
{
    dbref loc;
    const char *message;

    loc = loc_of(player);
    message = reconstruct_message(arg1, arg2);
    fprintf(logfile, "GRIPE from %s(%d) in %s(%d): %s\n",
	    Name(player), player,
	    Name(loc), loc,
	    message);
    fflush(logfile);

    notify(player, "Your complaint has been duly noted.");
}

void do_succ(player, arg1, arg2)
dbref player;
char *arg1, *arg2;
{	int qs = quiet;
	dbref loc = player;

	if( loc == NOTHING)
		return;

	if( !Player(loc))
		loc = location_of(player);

	quiet = 0;
	if( Player(loc)){
		notify(loc, reconstruct_message(arg1, arg2) );
	}
	quiet = qs;
}	

void do_osucc(player, arg1, arg2, loc)
dbref player, loc;
char *arg1, *arg2;
{	int qs = quiet;
	char *message;

	if( bad_dbref(loc, 0) && !bad_dbref(player, 0) && Typeof(player)!= TYPE_ROOM)
		loc = Location(player);
	if( bad_dbref(loc, 0))
		return;

	quiet = 0;
	message = reconstruct_message(arg1, arg2);

	notify_except( Contents(loc), player, message );
	robot_notify_except( Contents(loc), player, message);
	quiet = qs;
}
