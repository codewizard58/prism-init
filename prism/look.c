#include "copyrite.h"

/* commands which look at things */

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "match.h"
#include "externs.h"
#include "descript.h"
#include "fight.h"

extern struct descriptor_data *descriptor_list;

extern void look_exits();

/* Define the number of seconds after which statues become invisible */

#define NO_SEE_STATUES (3600l*24l*14l)

struct agerec {
long	age;
const char 	*descript;
} agetable[] = {
	10*60l,	      "Warm s",
	3600*24l*7l,    "S",		/* Less than a week */
	3600*24l*30l,   "Dusty s",		/* Less than a month */
	3600*24l*30*6l, "Crumbling s",
	0l, "Petrified s"
};

long ageof( thing)
dbref thing;
{	long now;

	if( Age(thing) == 0l){	/* Never set */
		now = 3600l*24l*60l;
		Age(thing) = now;
		return now;	/* Two months */
	}
	now =  timenow - Age(thing);
	if( now < 0l){		/* in the future ? */
		Age(thing) = timenow;
		now = 0l;
	}
	return now;
}

char *printageof( thing)
dbref thing;
{	struct agerec *p;
	long t;

	p = agetable;
	t = ageof(thing);
	while( p->age && p->age < t)p++;
	return (char *)p->descript;
}

struct time_rec{
long	secs;
const char *title, *unary, *plural;
}time_table []=
{
	{3600*24*365l, "year","", "s"},
	{3600*24*30l,  "month","","s"},
	{3600*24*7l,   "week", "","s"},
	{3600*24l,     "day", "", "s"},
	{3600l,	      "hour","", "s"},
	{60l,	      "minute","","s"},
	{1l,	      "second","","s"}
};

char *printsince_sub( t)
long t;
{
	long n;
	static char buf[100];
	struct time_rec *tr = time_table;

	if( t < 1l )
		return (char *)"0 seconds";
	while(1){
		if( t >= tr->secs){
			n = t/tr->secs;
			if( n == 1l){
        			sprintf(buf, "%ld %s%s", n, tr->title, tr->unary);
			}else {
	        		sprintf(buf, "%ld %s%s", n, tr->title, tr->plural);
			}
			break;
		}
		tr++;
	} 
	return buf;
}

char *printsince(thing)
dbref thing;
{
	if( bad_dbref(thing, 0))
		return "Unknown";
	return printsince_sub(ageof(thing));
}


void updateage( thing)
dbref thing;
{
	Age(thing) = timenow;
}

static char name0buf[64];

char *Name0(obj)
dbref obj;
{	char *q;

	strncpy(name0buf, Name(obj), 62);
	name0buf[62] = '\0';
	q = name0buf;
	while(*q && *q != ';')q++;
	*q = '\0';
	return name0buf;
}

char *Name1(obj)
dbref obj;
{	char *q;

	q = Name(obj);
	while(*q && *q != ';')q++;

	if( *q == '\0')
		q = Name(obj);
	else
		q++;

	strncpy(name0buf, q, 62);
	name0buf[62] = '\0';
	q = name0buf;
	while(*q && *q != ';')q++;
	*q = '\0';
	return name0buf;
}


char *zone_name(zone)
dbref zone;
{	static char zbuf[12];	/* Must be static */

	zone = zoneof(zone);
	if( zone >= 0 && zone < db_top && Typeof(zone)==TYPE_ROOM &&
		(Flags(zone) & INHERIT)){
		return (char *)Name(zone);
	}else
		sprintf(zbuf,"Zone %.5d", zone);
	return zbuf;
}


char *owner_name(thing)
dbref thing;
{	static char buf[128];
	char *p;
	dbref owner2 = Owner(thing);

	sprintf(buf, "%.60s", Name0(Owner(thing)) );
	for(p=buf; *p; p++);	/* skip to end */

	return buf;
}


char *prt_position(thing)
dbref thing;
{	static char pbuf[40];

	long pos = Position(thing);
	int x,y,z;

	x = pos & 0x3ff;
	y = (pos >> 10)&0x3ff;
	z = (pos >> 20)&0x3ff;

	if( x & 0x200)
		x = x - 0x400;
	if( y & 0x200)
		y = y - 0x400;
	if( z & 0x200)
		z = z - 0x400;
	sprintf(pbuf, "<%d,%d,%d>",x,y,z);
	return pbuf;
}


int indent = 0;
static char spaces[]="                                         ";

void do_indent(player, msg, thing)
dbref player,thing;
char *msg;
{
	struct descriptor_data *d;
	int ind;

	ind = indent;
	if(ind < 0 || ind > 32)
		ind = 0;
	else
		ind = 40-ind;

	if(!quiet)
		for(d=descriptor_list; d; d=d->next){
			if(d->connected == 2 && d->player == player){
				if( ind)
					queue_string(d, &spaces[ind]);
				queue_string(d, msg);
				queue_string(d, "\r\n");
			}
		}
}

static int somestatues;

void pplayername(player, thing)
dbref player, thing;
{
	if( logged_in(thing) || ( Flags(thing)&ROBOT)){
	    sprintf(msgbuf,"%s%s", player_title(thing), unparse_object(player, thing));
	    do_indent(player, msgbuf, NOTHING);
	}else {
		if( ageof( thing) < NO_SEE_STATUES){
	    		sprintf(msgbuf,"%statue of %s%s", printageof(thing), 
				player_title(thing), 
				unparse_object(player, thing));
	 		do_indent(player, msgbuf, NOTHING);
		}else somestatues++;
	}
}

/* Bsx helper routine.. */
int scramble[8] = {4, 6, 2, 3, 5, 1, 7, 0};

void bsx_vis_object(player, thing, pos, depth)
dbref player, thing;
int *pos, *depth;
{	long where;
	int x,y,z;

	where = Position(thing);
	if( where != 0){
		x = where & 0x3ff;
		y = (where >> 10) & 0x3ff;
		z = (where >> 20) & 0x3ff;
		if( y >= 8 || x >= 8)
			where = 0;	/* reposition */
	}

	if( where == 0){
		x = scramble[*pos];
		y = scramble[*depth];
		*pos = *pos + 1;
		if( *pos == 8){
			*pos = 0;
			*depth = *depth+1;
		}
		if( depth == 8)
			depth = 0;
fprintf(logfile,"Reposition %d at %d,%d\n", thing, x, y);
		Position(thing) = ((y&0x3ff)*1l << 10)|(x&0x3ff);
	}

	show(player, -1, thing);
}


/* Display the contents of something *
 * returns 1 if you were told something else 0
 * You are not told about dark objects that you don't own
 * or about unlinked exits you are carrying.
 */	

static int look_contents(player, loc, contents_name)
dbref player, loc;
char *contents_name;
{
    dbref thing;
    dbref can_see_loc;
    char *msg;
    int seen_something = 0;
    int mflags = 0;

    somestatues = 0;
    /* check to see if he can see the location */
    can_see_loc = (!Dark(loc) || controls(player, loc));

    /* check to see if there is anything there */
    DOLIST(thing, Contents(loc)) {
	if(can_see(player, thing, can_see_loc)) {
	    /* something exists!  show him everything */
	    seen_something++;
	    notify(player, contents_name);
	    DOLIST(thing, Contents(loc) ) {
		if( Typeof(thing) == TYPE_EXIT &&
			Contents(thing) != NOTHING) /* Don't list exits in contents list */
			continue;
		if(can_see(player, thing, can_see_loc)) {
		    if(Typeof(thing)== TYPE_PLAYER ){
			pplayername(player, thing);
		    } else {
			msg = unparse_object(player, thing);
		    	do_indent(player, msg, thing);
		    }
		    mflags |= show(player, -1, thing);
		}
	    } /* DOLIST */
	    if(somestatues){
		do_indent(player, "Some old statues", NOTHING);
		mflags |= show(player, -1, -5);	/* Statues */
	    }
	    break;		/* we're done */
	}
    } /* DOLIST */
    show_flush(player, -1, mflags);
    return seen_something;
}

static void look_simple(player, thing)
dbref player, thing;
{   static char buf[128];
    char *p, *q;
    int flag = 0;	/* Used to flag if we have shown something */
    dbref exit;

    if( bad_dbref(thing, 0)){
	fprintf(logfile,"BAD dbref in look simple(%d,%d)\n",player, thing);
	return;
    }
    if( Typeof(thing) == TYPE_EXIT){
	sprintf(msgbuf,"Cant look simple an EXIT (%d,%d)",player,thing);
	fnotify(player,msgbuf);
	return;
    }

    if( Dark(thing) && !controls(player, thing)){
	notify(player,"You cannot see that here!");
	return;
    }
    p = Desc(thing);
    if( p && !Robot(player)) {
	notify(player, p);
	flag++;
    }
    if( can_contain(thing) && Contents(thing) != NOTHING){
	if(Typeof(thing)==TYPE_PLAYER)
		sprintf(buf, "carrying:");
	else
		sprintf(buf, "%.60s contains:", Name0(thing));
	indent += 8;
	flag |= look_contents(player, thing, buf);
	indent -= 8;
    }else if( Contents(thing) != NOTHING){
		indent +=8;
		q = msgbuf;
		*q = '\0';
		p = NULL;	/* Also used as a first time flag */
		DOLIST(exit, Contents(thing)){
			if( Typeof(exit) != TYPE_EXIT ||
				Contents(exit) != NOTHING)
				continue;
			if( (Flags(exit)&(INHERIT|DARK))== DARK){
				if( !p){
					Notify(player, "Obvious commands:");
				}
				p = Name0(exit);
				while(*p && *p != ';')*q++ = *p++;
				*q++ = ',';
				*q++ = ' ';
				*q = '\0';
			}
		}
		if( msgbuf[0]){
			q[-2] = '.';
			q[-1] = '\0';
			do_indent(player, msgbuf, thing);
			flag++;
		}
		indent -= 8;
    }
    if( !flag)
	notify(player, "You see nothing special.");
}

void look_room( player, loc)
dbref player, loc;
{	char *p;
	int mflags = 0;

    /* tell him the description */
    p = Desc(loc);

    if( controls(player, loc) || !p){
	notify(player, unparse_object(player, loc));
    }

    mflags |=	show(player, -1, loc);

    if( p && !Robot(player)){
	notify(player, p);	/* Tell him the desc */
    }

    /* tell him the contents */
    indent += 8;
    look_contents(player, loc, "Contents:");
    indent -= 8;

    /* tell obvious exits */
    look_exits(player, loc, "Obvious exits:");
    show_flush(player, -1, mflags);
}

void do_look_around( player)
dbref player;
{
    dbref loc;

    if((loc = getloc(player)) == NOTHING) return;

    send_message(player, loc, "look here", player);
}

/* This is  the routine that the @look command calls */

void do_look_at( player, name)
dbref player;
char *name;
{
    dbref thing;

    if(*name == '\0') {
	if((thing = getloc(player)) != NOTHING) {
	    look_room(player, thing);
	}
    } else {
	/* look at a thing here */
	init_match(player, name, NOTYPE);
	match_neighbor();
	match_possession();

	match_here();
	match_me();

	if((thing = noisy_match_result()) != NOTHING) {
	    switch(Typeof(thing)) {
	      case TYPE_ROOM:
		look_room(player, thing);
		break;
	      case TYPE_PLAYER:
		look_simple(player, thing);
		break;
	      case TYPE_THING:
		look_simple(player, thing);
		break;
	      default:
sprintf(msgbuf,"CANT @look the exit (%d) %s",thing, name);
fnotify(player,msgbuf);
		last_result = 0;
		break;
	    }
	}
    }
}

void do_look_com(player, name)
dbref player;
char *name;
{
	int qs = quiet;
	object_flag_type saved;
	dbref  saved_euid = euid;

	if( !ArchWizard(player)){
		notify(player,"You see nothing special.");
		return;
	}
	saved = Flags(player);
	quiet = 0;
	euid  = NOTHING;
	Flags(player) &= ~WIZARD;

	do_look_at(player, name);

	Flags(player) = saved;
	quiet = qs;
	euid  = saved_euid;
}

void do_examine( player, name)
dbref player;
const char *name;
{
    dbref thing;
    dbref content;
    dbref exit, loc;
    dbref saved = euid;
    BOOLEXP_PTR b;
    char *m, *p;

    if( !Builder(player)){
	notify(player,"Builders only!");
	return;
    }

    euid = NOTHING;

    if(*name == '\0') {
	if((thing = getloc(player)) == NOTHING) return;
    } else {
	/* look it up */
	init_match(player, name, NOTYPE);
	match_everything();

	/* get result */
	if((thing = noisy_match_result()) == NOTHING) return;
    }

    if(!can_link(player, thing)) {
	notify(player,
	       "You can only examine what you own.");
	euid = saved;
	return;
    }

    notify(player, unparse_object(player, thing));

    if( Typeof(thing) == TYPE_EXIT){
    sprintf(msgbuf, "Owner: %s  Key: %s Age: %s old",
	    owner_name(thing),
	    unparse_boolexp(player, Key(thing) ), printsince( thing) );
    }else if( Typeof(thing) == TYPE_PLAYER){
	    sprintf(msgbuf, "Owner: %s  Pennies: %s Age: %s old",
		    owner_name(thing),
	    	prt_money( Pennies(thing) ), printsince( thing) );
    }else {
	    sprintf(msgbuf, "Owner: %s  Age: %s old",
		    owner_name(thing),
	   	 printsince( thing) );
    }
    notify(player, msgbuf);


    if(Player(thing)){
	sprintf(msgbuf, "Level: %d Title: %s", Level( thing), player_title( thing));
	    notify(player, msgbuf);
    }

    if( Class(thing) != NOTHING && Class(thing)) {
	sprintf(msgbuf, "Class: %s", unparse_object(player, Class(thing)));
	notify(player, msgbuf);
    }

    if( ArchWizard(player) ){   
	    sprintf(msgbuf, "Next: #%d    Position: %s", Next(thing), prt_position(thing));
    	notify(player,msgbuf);
    }

    if(Typeof(thing) == TYPE_ROOM){
	sprintf(msgbuf, "Zone: %s (#%d)", zone_name(thing), zoneof(thing));
	notify(player, msgbuf);
    }

    if(Typeof(thing) != TYPE_EXIT){
	    p = Desc(thing);
	    if(p && !Robot(player)) 
		notify(player, p);

    /* show him the contents */
    	if( Contents(thing) != NOTHING) {
		indent += 8;
		notify(player, "Contents:");
		DOLIST(content, Contents(thing)) {
	    		if( Typeof(content)== TYPE_EXIT){
				if( Contents(content)!= NOTHING)
					continue;
				do_indent(player, unparse_object(player, content), thing);
				continue;
	    		}
	    		if(can_see(player, content, 1))
				do_indent(player, unparse_object(player, content), thing);
		}
		indent -= 8;
    	}
    }

    if( (Typeof(thing) == TYPE_THING || Typeof(thing) == TYPE_PLAYER)
	&& Contents(thing) != NOTHING) {
	indent += 8;
	notify(player, "Commands:");
	DOLIST(content, Contents(thing)) {
	    if( Typeof(content)!= TYPE_EXIT || Contents(content) == NOTHING)
		continue;
	    do_indent(player, unparse_object(player, content), thing);
	}
	indent -= 8;
    }

    switch(Typeof(thing)) {
      case TYPE_ROOM:
	/* tell him about exits */
	if(Contents(thing) != NOTHING) {
	    indent +=8;
	    notify(player, "Exits:");
	    DOLIST(exit, Contents(thing)) {
		if(Typeof(exit) == TYPE_EXIT && Contents(exit) != NOTHING)
			notify(player, unparse_object(player, exit));
	    }
	    indent -=8;
	} else {
	    notify(player, "No exits.");
	}

	break;
      case TYPE_THING:
      case TYPE_PLAYER:
	/* print location if player can link to it */
	if(Location(thing) != NOTHING
	   && (controls(player, Location(thing))
	       || can_link_to(player, Location(thing)))) {
	    sprintf(msgbuf, "Location: %s",
		    unparse_object(player, Location(thing)));
	    notify(player, msgbuf);
	}
	if(Typeof(thing) == TYPE_PLAYER ){	/* Has an extension */
	    sprintf(msgbuf, "Chp= %d", Chp(thing));
	    notify(player, msgbuf);
	    if( Ext(thing).robot){	/* has robot stats */
		struct robot *r = Ext(thing).robot;

		notify(player,"Robot info");
		sprintf(msgbuf, "Sp fp fsp hv ha pennies mp gp dp commant");
		notify(player,msgbuf);
		sprintf(msgbuf, "%02d %02d %03d %02d %02d %07d %02d %02d %02d, %.15s",
			r->speed, r->fight_p, 0, 0,
			0, 0, r->move_p, r->get_p, r->drop_p, r->comment? r->comment : "");
		notify(player, msgbuf);
	    }
	}
	break;
      case TYPE_EXIT:
	/* print destination */
	switch(Contents(thing)) {
	  case NOTHING:
	    break;
	  case HOME:
	    notify(player, "Destination: *HOME*");
	    break;
	  default:
	    sprintf(msgbuf, "Destination: %s", unparse_object(player, Contents(thing)));
	    notify(player, msgbuf);
	    loc = Location(thing);
	    if( loc >= 0 && loc < db_top && controls(player, loc)){
		    sprintf(msgbuf, "From: %d", loc);
		    notify(player, msgbuf);
	    }
	    break;
	}
	break;
      default:
	/* do nothing */
	break;
    }
    euid = saved;
}

void do_inventory( player)
dbref player;
{
    dbref thing;
    dbref saved = euid;
    int mflags = 0;


	if( !ArchWizard(player)){
		last_result = 0;
		notify(player,"Sorry you are not authorized to @inv.");
		return;
	}
    euid = NOTHING;

	mflags |= show(player, -1, -4);	/* inventory */
	indent += 8;
	if( !look_contents(player, player, "You are carrying:") ){
		indent -= 8;
		notify(player, "You aren't carrying anything.");
	}else {
		indent -= 8;
	}
	show_flush(player, -1, mflags);
    euid = saved;
}

extern char *diagnose();

void do_score( player)
dbref player;
{   money pen, lowerpen;
    dbref thing;
    dbref saved = euid;

    euid = NOTHING;

	sprintf(msgbuf, "You have %s", prt_money( Pennies(player)));
	notify(player, msgbuf);
    sprintf(msgbuf,"Level %d, Title %s",Level(player), player_title(player));
    notify(player, msgbuf );
    sprintf(msgbuf,"Chp = %d, Mhp = %d", Chp(player),Mhp(player));
    notify(player, msgbuf);

    sprintf(msgbuf, "You are in %s", zone_name(player));
    notify(player, msgbuf);
    sprintf(msgbuf,"You feel %s.", diagnose(player));
    notify(player, msgbuf);

    pen = player_level_table[Level(player)].pennies;
    lowerpen = 0;
    if( Level(player) > 0){
	lowerpen = player_level_table[Level(player)-1].pennies;
    }
    pen = (pen-lowerpen) /3;

    switch( (Pennies(player)-lowerpen) / pen){
    default:
	notify(player, "You will be demoted if you die.");
	break;
    case 1:
	notify(player,"You can die once before you get demoted.");
	break;
    case 2:
    case 3:
	notify(player,"You could die a couple of times before you get demoted.");
	break;
    }

    euid = saved;
}

/* do_find.
 * flags bit meaning
 * 0 - allow exit matching  /exit
 * 1 - 
 * 2 - find objects in a class/zone
 */

void do_find( player, name1)
dbref player;
const char *name1;
{
    dbref i;
    dbref owner,class;
    char nbuf[128],*name;
    int exitf = 0, flags=0;

    owner = class = NOTHING;
    nbuf[0] = '\0';
    name = name1;

    while( *name == '/'){
	strncpy(nbuf, name+1, 120);
	nbuf[120] = '\0';
	name = nbuf;
	while(*name && *name != ' ')name++;
	if( *name){
		*name++ = '\0';
	}
	while(*name && *name == ' ')name++;

	if( !strcmp("exit", nbuf)){
		exitf++;
		notify(player,"Looking for an EXIT");
		flags |= 1;
	}else{
		init_match(player, nbuf, NOTYPE);
		match_neighbor();
		match_possession();
		match_here();
		match_me();
		if(TrueWizard(player)){
			match_player();
		}
		match_absolute();
		owner = noisy_match_result();

		if( bad_dbref(owner, 0)){
			owner = NOTHING;
		}else if( Typeof(owner) == TYPE_ROOM &&
			 (Flags(owner) & INHERIT)){
			if( controls(player, owner)){
				class = owner;
				flags |= 4;
			}
			owner = NOTHING;
		}else {
			flags |= 2;
		}
	}
    }

    if(!TrueWizard(player) && !payfor(player, LOOKUP_COST)) {
	notify(player, "You don't have enough pennies.");
    } else {
	if( class == NOTHING && owner == NOTHING){
		for(i = 0; i < db_top; i++) {
		    if( (flags & 1)== 0 && (Typeof(i) != TYPE_EXIT)
		       && controls(player, i)
		       && (!*name || string_match(Name(i), name))) {
			notify(player, unparse_object(player, i));
		    }
		    if( (flags & 1) == 1 && (Typeof(i) == TYPE_EXIT)
		       && controls(player, i)
		       && (!*name || string_match(Name(i), name))) {
			notify(player, unparse_object(player, i));
		    }
		}
	}else if( class == NOTHING && TrueWizard(player)){
		notify(player,"Looking for objects owned by..");
		for(i = 0; i < db_top; i++) {
		    if( ((flags & 1) || Typeof(i) != TYPE_EXIT) && 
			Owner(i) == owner &&
		        (!*name || string_match(Name(i), name))) {
			notify(player, unparse_object(player, i));
		    }
		}
	}else {
		notify(player,"Looking for objects in class/zone..");
		for(i = 0; i < db_top; i++) {
		    if( Typeof(i) != TYPE_EXIT &&
			(Class(i) == class || (Typeof(i)==TYPE_ROOM && zoneof(i)== class) )
		       && (!*name || string_match(Name(i), name))) {
			notify(player, unparse_object(player, i));
		    }
		}
	}
	notify(player, "***End of List***");
    }
}

/* Print list of obvious exits */
/* NOTE: DARK bit has opposite effect here! */

void look_exits(player, loc, contents_name)
dbref player, loc;
char *contents_name;
{
    char *p,*q;
    dbref thing;

    q = msgbuf;
    *q = '\0';

    /* check to see if there is anything there */
    DOLIST(thing, Contents(loc)) {
	if( Typeof(thing) == TYPE_EXIT && 
	    ( (Dark(thing) && !Dark(loc)) || controls(player, thing)) ) {
	    /* something exists!  show him everything */
	    notify(player, contents_name);
	    DOLIST(thing, Contents(loc)) {
		if( Typeof(thing) == TYPE_EXIT &&
		    ((Dark(thing) && !Dark(loc)) || controls(player, thing)) ) {
			sprintf(q, "%s", Name0(thing));
			for(p = q; *p && *p != ';';p++);
			*p = '\0';
			if( controls(player, thing)){
			    indent += 8;
			    do_indent(player, unparse_object(player, thing), thing);
			    indent -= 8;
			    *q= '\0';
			}else {
				while(*q)q++;
				*q++ = ',';
				*q++ = ' ';
				*q = '\0';
			}
		}
	    }
	    break;		/* we're done */
	}
    }

    if(msgbuf[0]){		/* print non controlled list */
	indent += 8;
	q[-2] = '.';
	q[-1] = '\0';
	do_indent(player, msgbuf, NOTHING);
	indent -= 8;
    }
}
