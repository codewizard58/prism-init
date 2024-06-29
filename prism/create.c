#include "copyrite.h"

/* Commands that create new objects */

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "externs.h"
#include "match.h"

extern dbref euid;
extern dbref noisy_match_result();
extern int last_result;

/* 22-2-92 */
dbref created_object;

#ifndef Builder
/* was a macro */

int Builder(player)
dbref player;
{	int res = 0;
	int loc;

	if( ArchWizard(player)) res = 1;

	if( (Flags(player)&(BUILDER|TYPE_MASK)) == (BUILDER|TYPE_PLAYER) )
		res = 1;
	return res;
}
#endif

dbref get_new_object(player)
dbref player;
{	dbref thing,room;

	for(thing = 0; thing < db_top ; thing++){
		if(is_rubbish(thing))
			break;
	}
	if(thing < db_top){
		room = thing;
	}else
		room = new_object();
/* Basic initialisation */
	Class(room) = NOTHING;
	Contents(room) =NOTHING;
	Location(room)=NOTHING;
	Age(room) = timenow;
	Next(room)= NOTHING;

	return room;
}

void check_set_flags(player)
dbref player;
{
	if( !(Flags(player) & BUILDER) && 
		Level(player) >= 19){
		Flags(player) |= BUILDER; 
		fprintf(logfile,"Advancing %s to BUILDER\n", Name(player));
  	}

	if( !TrueWizard(player) && Level(player) >= 20){
/*			Flags(player) |= WIZARD; */
		fprintf(logfile,"Advancing %s to WIZARD\n", Name(player));
	}

	if( !ArchWizard(player) && (Flags(player) & BUILDER) && Level(player) < 19){
		fprintf(logfile,"Removing BUILDER from %s(%d)\n", Name(player), player);
		Flags(player) &= ~BUILDER;
	}
}

/* utility for open and link */
dbref parse_linkable_room(player, room_name)
dbref player;
const char *room_name;
{
    dbref room;

    /* skip leading NUMBER_TOKEN if any */
    if(*room_name == NUMBER_TOKEN) room_name++;

    /* parse room */
    if(!string_compare(room_name, "here")) {
	room = Location(player);
    } else {
	room = parse_dbref(room_name);
    }

    if( room == DEF_PLAYER || room == DEF_ROOM || room == DEF_THING){
	goto done;
    }

    /* check room */
    if(room < 0 || room >= db_top ){
		notify(player, "That's not a valid room!");
		room = NOTHING;
		goto done;
    }
    if( Typeof(room) != TYPE_ROOM){
		notify(player, "That's not a room!");
		room = NOTHING;
		goto done;
    }

    if(!can_link_to(player, room)) {
	notify(player, "You can't link to that.");
	room = NOTHING;
    }

done:
	return room;
}

dbref is_command(player, loc, direction, buf)
dbref player, loc;
char **direction,*buf;
{	char *com;
	dbref loc1;

    if( **direction == '/'){
	(*direction)++;
	com = buf;
	while( (*com = *(*direction)++) && *com != ' ' && *com != '\t' && com != &buf[63])com++;
	while( **direction == ' ')(*direction)++;
	*com = '\0';

	init_match(player, buf, TYPE_THING);
	match_absolute();
	match_possession();
	match_me();
	match_neighbor();
	loc1 = match_result();

	switch(loc1){
	case NOTHING:
		sprintf(msgbuf,"You don't have %s!", buf);
		notify(player, msgbuf);
		break;
	case AMBIGUOUS:
		sprintf(msgbuf,"Which %s did you mean?", buf);
		notify(player, msgbuf);
		break;
	default:
		if( Typeof(loc1) == TYPE_EXIT){
			sprintf(msgbuf,"You can't add a command to an exit!");
			notify(player, msgbuf);
			break;
		}
		loc = loc1;
		break;
	}
    }
    return loc;
}

void do_link_sub(player, thing, room)
dbref player,thing,room;
{	dbref loc;

	check_set_flags(player);

	switch(Typeof(thing)) {
	  case TYPE_EXIT:
	    if(room && Typeof(room) != TYPE_ROOM){	/* not a room */
		notify(player, "Can only link to a room!");
		last_result = 0;
		return;
	    }

	    /* we're ok, check the usual stuff */
	    if(Contents(thing) != NOTHING) {
		if(controls(player, thing)) {
			notify(player, "That exit is already linked.");
		} else {
		    denied(player);
		}
		last_result = 0;
		return;
	    } else {
		/* handle costs */
		if(Owner(thing) != player) {
		     if(!Builder(player)) {
			notify(player,
			       "Only authorized builders may seize exits.");
			last_result = 0;
			return;
		    }
		}

		/* link has been validated and paid for; do it */
		loc = Location(thing);		/* what is carrying exit */
		if( Typeof(loc) == TYPE_ROOM && !room && (Flags(loc)&INHERIT) == 0){
fprintf(logfile,"%s(%d) tried to link an exit to #0\n", Name(player), player);
			denied(player);
			last_result = 0;
			return;
		}
		Owner(thing) = player;
		if( (Typeof(loc)!= TYPE_ROOM || (Flags(loc)&INHERIT)) && room ){
			sprintf(msgbuf,"Player %s(%d) tried to link command %d to room %d\n", Name(player), player, thing,room);
			fnotify(player, msgbuf);
			return;
		}
/* Non Archwizards must own destination and location of exit */
		if( !ArchWizard(player) && 
		    (player != Owner(loc) || (room && player != Owner(room)) )){
			sprintf(msgbuf,"You (%s) can not link an exit to %d!", Name(player), room);
			fnotify(player, msgbuf);
			last_result = 0;
			return;
		}
		Contents(thing) = room;
		notify(player, "Linked.");
	    }
	    break;

	  case TYPE_THING:
	  case TYPE_PLAYER:
	  case TYPE_ROOM:
	    Notify(player,"You can only link exits..!");
		last_result = 0;
		return;
	  default:
	    notify(player, "Internal error: weird object type.");
	    fprintf(logfile, "PANIC weird object: Typeof(%d) = %d\n",
		    thing, Typeof(thing));
	    break;
	}
}

void no_build(player)
dbref player;
{
	notify(player, "Sorry you are not allowed to build!.");
}

/* use this to create an exit */
void do_open( player, direction,linkto)
dbref player;
const char *direction;
const char *linkto;
{
    dbref loc, loc2;
    dbref exit;
    char buf[64];


    check_set_flags(player);

#ifdef RESTRICTED_BUILDING
    if(!Builder(player)) {
	no_build(player);
	return;
    }
#endif /* RESTRICTED_BUILDING */

    if((loc = getloc(player)) == NOTHING) return;

/* pete commands */
    loc = is_command(player, loc, &direction, buf);

    if(!*direction) {
	notify(player, "Open where?");
	return;
    } else if(!TrueWizard(player) && !ok_name(direction)) {
	notify(player, "That's a strange name for an exit!");
	return;
    }

    if(!controls(player, loc)) {
	denied(player);
    } else {

	exit = get_new_object(player);

	change_object_type(exit, TYPE_EXIT);

	/* initialize everything */
	NameW(exit, (char *)(direction));
	Owner(exit) = player;
	Flags(exit) = TYPE_EXIT;
	Age(exit) = timenow;
	Location(exit) = loc;		/* Record where it lives */
/*	DescW(exit, NULL); */
	Contents(exit) = NOTHING;
        Key(exit) = TRUE_BOOLEXP;

	/* link it in */
	PUSH(exit, Contents(loc));

	/* and we're done */
	sprintf(msgbuf, "%d opened exit number %d from %s(%d).", player,
		exit, Name(loc), loc);
	fnotify(player, msgbuf);
	created_object = exit;

	/* check second arg to see if we should do a link */
	if(*linkto != '\0') {
	    notify(player, "Trying to link...");
	    if((loc2 = parse_linkable_room(player, linkto)) != NOTHING) {
		do_link_sub(player, exit, loc2);
	    }
	}else {
		if( Typeof(Location(exit)) == TYPE_ROOM && !(Flags(loc)&INHERIT)){
			notify(player,"Linking exit to HERE");
			do_link_sub(player, exit, loc);
		}else {
			notify(player,"Linked command to #0");
			do_link_sub(player, exit, 0);
		}
	}
    }
}

dbref find_the_exit(player, name)
dbref player;
const char *name;
{
    init_match(player, name, TYPE_EXIT);
    match_exit();
    match_neighbor();
    match_possession();
    match_me();
    match_here();
    if( Builder(player)) {
	match_absolute();
	match_player();
    }
    return noisy_match_result();
}

void do_link( player, name, room_name)
dbref player;
const char *name;
const char *room_name;
{
    dbref thing;
    dbref room;

    if((room = parse_linkable_room(player, room_name)) == NOTHING) return;

    if((thing = find_the_exit(player, name)) != NOTHING) {
	do_link_sub(player, thing, room);
    }
}

void do_relink( player, name, room_name)
dbref player;
const char *name;
const char *room_name;
{
    dbref thing;
    dbref room;

    if((room = parse_linkable_room(player, room_name)) == NOTHING) return;

    if((thing = find_the_exit(player, name)) != NOTHING) {
	if( Typeof(thing) != TYPE_EXIT){
		notify(player,"Can only relink exits!");
	}else {
		Contents(thing) = NOTHING;
		do_link_sub(player, thing, room);
	}
    }
}

/* use this to create a room */
void do_dig(player, name, exit_name)
dbref player;
const char *name, *exit_name;
{
    dbref room, loc;
    static char room_name[10];	/* used to pass arg to open */
    dbref class = NOTHING;

    check_set_flags(player);

#ifdef RESTRICTED_BUILDING
    if(!Builder(player)) {
	no_build(player);
	return;
    }
#endif /* RESTRICTED_BUILDING */

    if(*name == '\0') {
	notify(player, "Dig what?");
    } else if(!ok_name(name)) {
	notify(player, "That's a silly name for a room!");
    } else if( !can_link_to(player, Location(player)) ){
	notify(player, "You cannot dig a room here.");
    }else{
	room = get_new_object(player);
	loc = Location(player);
	if( bad_dbref(loc, 0))
		loc = 0;
	change_object_type(room, TYPE_ROOM);
	/* Initialize everything */
	NameW(room, (char *)(name));
	Owner(room) = player;
	Flags(room) = TYPE_ROOM;
	Zone(room) = zoneof(player);
	class = Class(loc);
	if( class < 1 && default_room_class != NOTHING)
		class = default_room_class;
	Class(room) = class;
	sprintf(msgbuf, "%d dug room %s with number %d of class %d in zone %d", 
		player, name, room, class, Zone(room));
	fnotify(player, msgbuf);
	created_object = room;

	if( exit_name && *exit_name){
		sprintf(room_name,"%d",room);
		do_open(player, exit_name, room_name);
	}
    }
}

/* use this to create an object */
void do_create(player, name, arg2)
dbref player;
char *name;
char *arg2;
{
    dbref loc;
    dbref thing;
    dbref reset;

    check_set_flags(player);
    if(!Builder(player)) {
	no_build(player);
	return;
    }
    if(*name == '\0') {
	notify(player, "Create what?");
	return;
    } else if(!ok_name(name)) {
	notify(player, "That's a silly name for a thing!");
	return;
    }

    if( !controls(player, Location(player)) || Location(player) == 0){
	goto bad;
    } else {
	if((loc = Location(player)) != NOTHING
	   && can_link_to(player, loc) && loc) {
	} else {
		goto bad;
	}
	for(thing = 0; thing < db_top ; thing++){
		if(is_rubbish(thing))
			break;
	}
	if(thing < db_top){
		sprintf(msgbuf, "Allocating rubbish %d", thing);
		notify(player, msgbuf);
	}else	/* create the exit */
		thing= new_object();
	change_object_type(thing, TYPE_THING);

	/* initialize everything */
	NameW(thing, (char *)(name));
	Location(thing) = player;
	Owner(thing) = player;
	Flags(thing) = TYPE_THING;
	if( default_thing_class != NOTHING)
		Class(thing) = default_thing_class;
	else
		Class(thing) = 0;

	Age(thing) = timenow;
	
	PUSH(thing, Contents(player));

	/* and we're done */
	sprintf(msgbuf,"%d created object number %d.", player, thing);
	notify(player, msgbuf);
	created_object = thing;
    }
    return;

bad:
    notify(player,"You cannot create things here!");
    return;
}

/* search for a reference to obj */

int find_refs(player, obj)
dbref player, obj;
{	dbref thing;
	int ret,res;

	res = 0;
	for(thing = 0; thing < db_top; thing++){
		ret = 0;
		if(thing == obj || 
			( Typeof(obj) == TYPE_THING &&
			  Typeof(thing)==TYPE_ROOM &&
			  thing == Location(obj))
		  )
			continue;
		if( Typeof(thing) == TYPE_EXIT && 
		    Key(thing) && 
		    search_boolexp(player, Key(thing), obj)){
			sprintf(msgbuf, "%s is in key of exit %d\n", Name(obj), thing);
			ret = 1;
		}
		switch(Typeof(thing)){
		case TYPE_ROOM:
			if( member(obj, Contents(thing))){
				sprintf(msgbuf, "%s is in room %d\n", Name(obj), thing);
				ret = 1;
			}
			break;
		case TYPE_PLAYER:
			if( thing != player && member(obj, Contents(thing) )){
				sprintf(msgbuf, "%s is carried by player %d\n", Name(obj), thing);
				ret = 1;
			}
			if( obj == Location(thing) ){
				sprintf(msgbuf, "%s location of player %d\n", Name(obj), thing);
				ret = 1;
			}
			break;
			
		case TYPE_THING:
			if( member(obj, Contents(thing) )){
				sprintf(msgbuf, "%s is inside object %d\n", Name(obj), thing);
				ret = 1;
			}
			if( obj == Location(thing)){
				sprintf(msgbuf, "%s location of thing %d\n", Name(obj), thing);
				ret = 1;
			}
			break;
		case TYPE_EXIT:
			if( obj == Contents(thing) ){
				sprintf(msgbuf, "%s is the destination of exit %d", Name(obj), thing);
				ret = 1;
			}
			if( obj == Location(thing)){
				sprintf(msgbuf, "%s is location of exit %d\n", Name(obj), thing);
				ret = 1;
			}
			break;
		default:
			break;
		}
		if( ret)
			notify(player, msgbuf);
		res |= ret;
	}
	return res;
}



void make_rubbish(thing)
dbref thing;
{	struct object *o = Db(thing);
	struct extension *e;
	dbref loc;

	if(!o)
		return;
	if(o->name) 
		free((void *) o->name);
	o->name = NULL;

	o->name = (char *)alloc_string("  RUBBISH");
#ifndef VM_DESC
	if(o->description) 
		free((void *) o->description);
#endif
	o->description = NULL;

	if(o->ver == VERD){
fprintf(logfile,"Make_rubbish: %d is old version\n", thing);
	}else if( Typeof(thing) == TYPE_EXIT){
		free_boolexp(Key(thing) );
		Key(thing) = TRUE_BOOLEXP;
	}else if( Typeof(thing) == TYPE_PLAYER){
		e = &Ext(thing);
		if(e){
			if(e->password) 
				free((void *) e->password);
			e->password = NULL;
		}
	}

	change_object_type(thing, TYPE_THING);	/* New 14-1-92 */
	o = Db(thing);

	o->owner = 1;
	if( (loc = o->location) != NOTHING){
		if( !bad_dbref(loc, 0))
			Contents(loc) = remove_first(Contents(loc), thing);
	}
	o->location = NOTHING;
	o->contents = NOTHING;
	o->next = NOTHING;
	o->flags = TYPE_THING;
	o->age = 0l;
	o->class = NOTHING;

}

void do_destroy( player, name)
dbref player;
const char *name;
{
    dbref thing, thing_2;

    if(*name == '\0') {
	if((thing = getloc(player)) == NOTHING) return;
    } else {
	/* look it up */
	init_match(player, name, NOTYPE);
	match_exit();
	match_neighbor();
	match_possession();
	match_absolute();

	if(ArchWizard(player)) 
		match_player();
	match_here();
	match_me();

	/* get result */
	if((thing = noisy_match_result()) == NOTHING) return;
    }
    if( is_rubbish(thing) ){
	notify(player, "To late it is already mangled!");
	return;
    }

    if(!ArchWizard(player) && Owner(thing) != player) {
	notify(player,
	       "You can only @DESTROY what you own.");
	return;
    }

    if( find_refs(player, thing)){
	notify(player, "**********");
	notify(player, "Object is referenced");
	return;
    }
    switch(Typeof(thing)) {
      case TYPE_ROOM:
	make_rubbish(thing);
	break;
      case TYPE_THING:
      case TYPE_EXIT:
	if( Location(thing) != player){
		notify(player, "You have to be carrying it");
		return;
	}
	if( thing == Contents(player) ){
		Contents(player) = Next(thing);
	}else for(thing_2 = Contents(player); thing_2 != NOTHING; thing_2 = Next(thing_2) ){
		if(thing == Next(thing_2) ){
			Next(thing_2) = Next(thing);
			break;
		}
	}
	make_rubbish(thing);
	break;
      case TYPE_PLAYER:
	notify(player, "You @TOAD players not @DESTROY!");
	return;
      default:
	/* do nothing */
	notify(player, "Not a valid object type!");
	notify(player,"making it a thing anyway.");
	make_rubbish(thing);
	return;
    }

    notify(player, "Destroyed!");
}
