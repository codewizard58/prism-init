#include "copyrite.h"

/* Routines for parsing arguments */
/* 25-9-91	pjc	Major rework to make recursive
 *			calls to match code work
 */

#include <ctype.h>

#include "db.h"
#include "config.h"
#include "match.h"
#include "boolexp.h"

extern char *unparse_object();

extern char translate_table[];
#define DOWNCASE(x) (translate_table[ x & 0xff])

struct match_vars *match_vars = NULL;

int no_true;
static char *argbuf;
static char *argvbuf[MAX_ARGC];
static char *argbufend;	/* Sentinal */
int xargc;
char **xargv = argvbuf;
char *xargp ;

dbref cur_receiver,cur_sender, cur_receiver_owner;
int cur_sender_iswiz;
dbref receiver,sender;
dbref absolute;
long match_counts;
extern void updateage( PROTO(dbref));
/* 22-2-92 */
extern dbref created_object;

extern long cmp_exit_count;
extern long cmp_char_count;
extern long cmp_star_count;

extern char *reconstruct_message(PROTO2(char *, char *));

extern char * Name0( PROTO(char *));

dbref wizauth()
{
	if( default_wizard < 1)
		return 1;
	return default_wizard;
}

/* New visability function for commands attached to objects */
/* is_exported(exit)
 * returns true if exit can be matched by the current sender
 *
 * Normally all exits are exported unless they have LINK_OK or
 * INHERIT set.
 *
 * INHERIT is exported if exit owner== (sender or euid).
 * LINK_OK is exported only to the person carrying the receiver.
 */

int is_exported(exit)
dbref exit;
{	int flags = Flags(exit);
	int ex_owner= Owner(exit);
	int auth = cur_sender;

	if( (flags&(INHERIT|LINK_OK) ) == 0)
		return 1;

	if( euid != NOTHING)
		auth = euid;

	if( auth == ex_owner )
		return 1;

	if( euid != NOTHING && TrueWizard(auth) && !TrueWizard(ex_owner)){
		return 0;
	}

	if( TrueWizard(auth) && (flags & LINK_OK) == 0)
		return 1;

	if( (flags&LINK_OK) && cur_sender == Location(cur_receiver)){
		return 1;
	}
	return 0;
}

void set_receiver(what)
dbref what;
{	dbref own = 0;

	if( bad_dbref(what, 0)){
fprintf(logfile,"Bad dbref in set_receiver (%d)\n", what);
		what = 0;
	}else {
		if( Typeof(what) == TYPE_PLAYER)
			own = what;
		else
			own = Owner(what);

		if( bad_dbref(own, 0)){
fprintf(logfile,"Bad dbref owner (%d of %d)\n", own, what);
			own = 0;
		}
	}

	cur_receiver = what;
	cur_receiver_owner = own;
}

dbref loc_of(thing)
dbref thing;
{	int depth =0;

	while(Typeof(thing) != TYPE_ROOM && depth++ < 20){
		thing = Location(thing);
		if( bad_dbref(thing, 0))
			return 0;
	}
	return thing;
}

int check_for_match(match, rec, sen)
dbref match, rec, sen;
{	struct listofmatches *l = list_head;

	while(l){
		if( l->amatch == match &&
		    l->receiver == rec &&
		    l->sender == sen){
			return 1;
		}
		l = l->next;
	}
	return 0;
}


void addtomatchlist(match)
dbref match;
{	struct listofmatches *l;
	char *p;

	if( check_for_match(match, cur_receiver, cur_sender) )
		return;

	l = (struct listofmatches *)malloc( Sizeof(struct listofmatches));
	if(!l){
		no_memory("Addtomatchlist");
		exact_match = NOTHING;
		return;
	}
	l->next = list_head;
	l->amatch= match;
	l->argc = xargc;
	l->receiver = cur_receiver;
	l->sender = cur_sender;

	if(xargc){
		if( l->argc >= MAX_ARGC)
			l->argc = MAX_ARGC-1;
		if( l->argc > 0 ){
			l->argv[0] = (char *)alloc_string(xargv[0]);
			p = l->argv[0];
			while(*p && *p != ';')p++;
			*p = '\0';
		}
		for(xargc = 1; xargc < l->argc; xargc++){
			l->argv[xargc] = (char *)alloc_string(xargv[xargc]);
		}
		for(; xargc < MAX_ARGC; xargc++)
			l->argv[xargc] = NULL;
	}
	list_head= l;
}

/* this routine saves away the last set of match variables */

void save_match_vars()
{	struct match_vars *t;
	int i;

	t = (struct match_vars *)malloc( Sizeof(struct match_vars) );

	if( !t ){
		no_memory("save_match_vars");
	}
	t->prev = match_vars;
	match_vars = t;

	for(i=0; i < MAX_ARGC; i++){
		xargl(i) = NULL;	/* Null the arg lists */
	}
	list_head = NULL;
}

void restore_match_vars()
{	struct match_vars *t;
	int i;

	t = match_vars;
	if( !t ){
		fprintf(logfile,"restore_match_vars: NULL pointer!\n");
		return;
	}
	no_true = match_vars->No_true;
	for(i=0; i< MAX_ARGC; i++){
		if( xargl(i)){
fprintf(logfile,"restore_match_vars: free_chain(%d)\n", i);
			free_chain( xargl(i));
		}
	}

	match_vars = t->prev;
	freemsg( (FREE_TYPE) t,"restore_match_vars");
}


void init_match( player, name, type)
dbref player;
const char *name;
int type;
{
    save_match_vars();
    match_counts++;

    exact_match = last_match = NOTHING;
    match_count = 0;
    match_who = player;
    cur_sender= player;
    xargc = 0;		/* Init xargc */
    if( euid > 0){
	cur_sender_iswiz= (Flags(euid)&WIZARD) == WIZARD;
    }else
	cur_sender_iswiz= (Flags(player)&WIZARD) == WIZARD;

    match_name = (char *) name;
    check_keys = 0;
    preferred_type = type;
    match_loc  = loc_of(player);
    list_head  = NULL;
    match_depth = 0;	/* simple looping check */
}

void init_match_check_keys( player, name, type)
dbref player;
const char *name;
int type;
{
    init_match(player, name, type);
    check_keys = 1;
}

void freematchlist()
{	struct listofmatches *l;
	int i;

	while(list_head){
		l = list_head->next;
		if(list_head->argc){
			for(i=0; i < list_head->argc; i++){
				free_string( list_head->argv[i]);
				list_head->argv[i] = NULL;
			}
		}
		freemsg( (FREE_TYPE) list_head, "freematchlist-2");
		list_head = l;
	}
}

/* Setup the xargc/xargv stuff for this attempted match */

int check_argbuf()
{
	if( !argbuf){
		argbuf = (char *)malloc(2050);
		if( argbuf)
			argbufend = &argbuf[2048];
	}
	if( !argbuf)
		return 1;
	return 0;
}


void setupxargs(c, v, l)
int c;
char **v;
struct dbref_chain *l;
{	char *p, *p1;
	int i, cnt;
	struct dbref_chain *l1;

	if(check_argbuf())
		return;

	xargc = c;
	xargv = argvbuf;
	*xargv= NULL;
	xargp = argbuf;

/* First remove debris from a previous attempt. */
	if( match_vars){
		for(i=0 ; i < MAX_ARGC; i++){
			if( xargl(i)){
				free_chain(xargl(i));
			}
			xargl(i) = NULL;
		}
		if( l){
			for(i=0; i < MAX_ARGC; i++){
				xargl(i) = l;
				if(l){
					l1 = l->next;
					l->next = NULL;
					l = l1;
				}
			}
		}
	}else
fprintf(logfile,"setupxargs: match_vars==NULL\n");

	if( !v && c){
fprintf(logfile,"setupxargs(%d, NULL)\n", c);
		c = 0;
	}

	p = argbuf;
	for(i=0; i < c; i++){
		cnt = 0;
		p1 = v[i];
		*xargv++ = p;
		*xargv = NULL;
		if( p1)
			while( (*p = *p1++) && cnt++ < (2047/(MAX_ARGC+1)))p++;
		*p++ = '\0';
	}
	if( created_object && created_object != NOTHING){
		char buf[10];

		sprintf(buf, "#%d", created_object);
		p1 = buf;
		*xargv++ = p;
		*xargv = NULL;
		xargc++;
		cnt = 0;
		while( (*p = *p1++) && cnt++ < (2047/(MAX_ARGC+1)))p++;
		*p++ = '\0';
		created_object = NOTHING;
	}
	xargv = argvbuf;
	xargp = p;	/* leave xargp pointing at first free position */
}

/* some_matched() returns true if anything matched */
int some_matched()
{	if( list_head )
		return 1;

	restore_match_vars();
	return 0;
}

dbref new_choose()
{	struct listofmatches *l, *sl = list_head;
	int n,m;
	dbref result = NOTHING;
	int debug = 0;

	created_object = NOTHING;
	match_vars->No_true = 0;			/* re init */

	if( !list_head )
		goto done;

	if( preferred_type != NOTYPE){
		debug = 1;
		l = list_head;
		for(n=0;l; l = l->next){
			if( Typeof(l->amatch) == preferred_type)
				n++;
		}
		if( n == 1){	/* only one preferred type */
			for(l=list_head;l;l=l->next){
				if(Typeof(l->amatch) == preferred_type){
					setupxargs(l->argc,l->argv, NULL);
					result = l->amatch;
					receiver = l->receiver;
					xargv[0] = (char *) Name(receiver);
					sender = l->sender;
					if(check_keys && !could_doit(match_who, l->amatch) ){
						match_vars->No_true = 1;
					}
					list_head = sl;
					goto matched;
				}
			}
			debug = 2;
		}
	}
/* either no preferred or more than one */
	if(check_keys){
		debug = 3;
		for(n=0,l = list_head;l;l = l->next)n++; /* count them */
		m = random() % n;	/* start looking at the mth one */
		l = list_head;
		while(m--){
			l = l->next;	/* move down to mth one */
			if( !l)
				l = list_head;
		}
		while(n--){
			debug = 4;
			setupxargs(l->argc, l->argv, NULL);
			receiver = l->receiver;
			sender = l-> sender;
			xargv[0] = (char *) Name(receiver);
			if(could_doit(match_who, l->amatch)){ /* return this one */
				list_head = sl;
				goto matched;
			}
			list_head = sl;
			l = l->next;
			if(!l)
				l = list_head;
		}
/* No locks were TRUE ! */
	match_vars->No_true = 1;		/* flag the fact */
	}

/* Just return the Nth one */
	if( list_head){
		for(n=0,l = list_head;l;l = l->next)n++; /* count them again */
		m = random() % n;	/* return the mth one */
		l = list_head;
		while(m--)
			l = l->next;	/* move down to mth one */
		goto matched;
	}else {
fprintf(logfile,"Broken new choose!\n");
	}
matched:
	result = l->amatch;
	setupxargs(l->argc,l->argv, NULL);
	receiver = l->receiver;
	sender = l->sender;
	xargv[0] = (char *) Name(receiver);
	freematchlist();

done:
	return result;
}

void at_which( player, direction, arg2)
dbref player;
const char *direction, *arg2;
{	struct listofmatches *l;
	char *p;

	if( !(ArchWizard(player) || controls(player, Location(player))) ){
		denied(player);
		return;
	}

	strcpy(msgbuf, reconstruct_message(direction, arg2));
	init_match_check_keys(player, msgbuf, TYPE_EXIT);
	if( direction && *direction == '@'){
		restore_match_vars();
		return;
	}
	match_exit();
	l = list_head;
	notify(player,"List of all command matches");
	while(l){
		sprintf(msgbuf,"%s : ",unparse_object(player, l->amatch));
		p = msgbuf;
		while(*p)p++;
		sprintf(p,"REC=%s",unparse_object(player, l->receiver));
		notify(player,msgbuf);
		l = l->next;
	}
	notify(player,"---------------------------");	

	restore_match_vars();
}


void match_player()
{
    dbref match;
    const char *p;

    xargc = 0;

    if(*match_name == LOOKUP_TOKEN
       && (payfor(match_who, LOOKUP_COST) || ArchWizard(match_who)) ) {
	for(p = match_name + 1; isspace(*p); p++);
	if((match = lookup_player(p)) > 0) {
	    set_receiver(match);
	    addtomatchlist(match);
	    exact_match = match;
	}
    }
}

/* returns nnn if name = #nnn, else NOTHING */
static dbref absolute_name()
{
    dbref match;

    xargc = 0;

    if(*match_name == NUMBER_TOKEN) {
	match = parse_dbref(match_name+1);
	if(match < 0 || match >= db_top) {
	    return NOTHING;
	} else {
	    return match;
	}
    } else {
	return NOTHING;
    }
}

void match_absolute()
{
    dbref match;

    xargc = 0;

    if((match = absolute_name()) != NOTHING) {
	if( controls(match_who, match)){
		set_receiver(match);
		addtomatchlist(match);
		exact_match = match;
	}
    }
}

void match_me()
{
    xargc = 0;

    if(!string_compare(match_name, "me")) {
	set_receiver(cur_sender);
	addtomatchlist(cur_sender);
	exact_match = cur_sender;
    }
}

void match_here()
{
    xargc = 0;

    if(!string_compare(match_name, "here")
       && match_loc != NOTHING) {
	set_receiver(match_loc);
	addtomatchlist(match_loc);
	exact_match = match_loc;
    }
}

void match_list( first)
dbref first;
{

    if( first == NOTHING)
		return;

    absolute = absolute_name();
    if(!controls(match_who, absolute)) absolute = NOTHING;

    if( absolute != NOTHING){
	exact_match = absolute;
	return;
    }

    DOLIST(first, first) {
	if( ++(match_depth) > 1024){
fprintf(logfile,"match_list: depth > 1024 first=%d\n", first);
		return;
	} else if(!string_compare(Name0(first), match_name)) {
	    /* if there are multiple exact matches, randomly choose one */
	    addtomatchlist(first);
	    exact_match = first;
	} else if(string_match(Name(first), match_name)) {
	    last_match = first;
	    match_count++;	/* count the partial matches */
	}
    }
}
    
void match_possession()
{   xargc = 0;
    set_receiver(cur_receiver);
    match_list( Contents(match_who) );
}

void match_neighbor()
{
    dbref loc;

    xargc = 0;

    if((loc = match_loc) != NOTHING) {
	set_receiver (loc);
	match_list( Contents(loc) );
    }
}

int cmpexit3(match, p)
char *match, *p;
{	int sargc = xargc;
	char *sargp = xargp;
	char **sargv= xargv;
	char ch,m;

	while(*match == ' ')match++;
	while(*p == ' ')p++;
	
	while( (m = *match) && (ch = *p) ){
		if( (m == '*' || m == '#') && !cmpexit2(match,p))
			return 0; /* matched */

		cmp_char_count++;
		
		if( DOWNCASE(m) != DOWNCASE(ch) )
			break;
		p++;
		match++;
	}
	if( !(*p) && (!m || m == EXIT_DELIMITER)){
		return 0;	/* matched */
	}
	xargc = sargc;
	xargp = sargp;
	xargv = sargv;
	return 1;		/* failed */
}

/* Used to match * and # meta chars */
/* Find the delimiting character. This is the first significant
 * character after the * or #. Copy across till the delimiter
 * is found then try and match whats left.
 *
 * NOTE. cmpexit3 restores xargc, xargv and xargp on failure.
 */

int cmpexit2(match, p)
char *match, *p;
{	int matchname = 0;
	int res;
	char * thisarg, delim;
	char * saved_xargp;

	xargc++;
	xargp++;
	*xargv++ = thisarg = xargp;
	*xargv   = NULL;
	*xargp   = '\0';

	if( *match == '#'){
		matchname = 1;
	}

	while( *match == '*' || *match == '#')match++;	/* skip #'s and *'s */

	while( *match == ' ')match++;	/* Skip WS after * */
	while( *p == ' ')p++;		/* Skip leading WS on argument*/
	switch( *match ){
	case '\0':
	case EXIT_DELIMITER:		/* pat = * ;.... */
		delim = 0;
		break;
	case '*':			/* pat = * *.... */
	case '#':
		delim = ' ';
		break;

	default:			/* pat = * something */
		delim = DOWNCASE(*match);
		match++;
		break;
	}

	cmp_star_count++;

/* Loop till we have a match or hit end */

	while(1){
		while((*xargp = *p) &&
			( DOWNCASE(*p) != delim && xargp != argbufend) ){
			xargp++;
			p++;
		}
		*xargp = '\0';
		
		if( !delim && !*p){	/* Both at end */
			while( xargp != thisarg && xargp[-1] == ' ')
				xargp--;
			*xargp = '\0';
			if( xargp == thisarg)
				return 1;	/* Can't be 0 length */

			if( matchname && (*thisarg != '#' || thisarg[1]) ){
				res = !string_match(Name(cur_receiver), thisarg);
				return res;
			}
			return 0;	/* matched */
		}
		if( !*p || !delim){/* Failed */
			return 1;
		}
		p++;	/* Skip the delimiter char */

		saved_xargp = xargp;
		if( !cmpexit3(match, p)){	/* Match whats left */
			while( saved_xargp != thisarg && saved_xargp[-1] == ' ')
				saved_xargp--;
			*saved_xargp = '\0';
			if( saved_xargp == thisarg)	/* Cant match null string */
				return 1;
			if( matchname && (*thisarg != '#' || thisarg[1]) ){
				res = !string_match(Name(cur_receiver), thisarg);
				return res;
			}
			return 0;	/* matched */
		}
/* else keep going */
		*xargp++ = delim;	/* wasn't this delimiter.. look again */
	}
}

int cmpexit(xmatch)
dbref xmatch;
{	char *p;
	char *match, *sargp;
	char m,ch;

	if( bad_dbref(xmatch, 0))
		return 1;	/* No match */
	match = (char *) Name(xmatch);

	sargp = xargp;

	while(*match) {
		cmp_exit_count++;
		xargc = 1;
		xargv = &argvbuf[1];
		*xargv= NULL;
		xargp = sargp;
		*xargp= '\0';

		/* check out this one */
		p = (char*)match_name;

		while(ch = *p){
			m = *match;
			if(m == EXIT_DELIMITER)
				break;	/* done */
			if(m == ' '){	/* match white space */
				if(ch != ' ')
					break;  /* failed */
				while(*p == ' ')p++;
				while(*match == ' ')match++;
				continue;	/* go around loop again */
			}
			if( ch != m){
				if( m == '*' || m == '#'){	/* parameter marker */
					if( !cmpexit2(match, p)){
						goto matched;
					}else 
						break;
				}
				cmp_char_count++;

				if( DOWNCASE(ch) != DOWNCASE(m))
					break;
			}
			p++;
			match++;
		}
		/* did we get it? */
		if(ch == '\0') {
			/* make sure there's nothing afterwards */
			while(isspace(*match)) match++;
			if(*match == '\0' || *match == EXIT_DELIMITER) {
				goto matched;
			}
		}
		/* we didn't get it, find next match */
		while(*match && *match++ != EXIT_DELIMITER);
		while(isspace(*match)) match++;
	}
not_matched:
	xargv = argvbuf;
	xargc = 0;
	xargp = sargp;
	return 1;
matched:
	xargv = argvbuf;
	return 0;
}

dbref Check_defaults(clas)
dbref clas;
{	if( clas == DEF_ROOM)
		clas = default_room_class;
	else if( clas == DEF_PLAYER)
		clas = default_player_class;
	else if(clas == DEF_THING)
		clas = default_thing_class;
	return clas;
}

/* SEND_MSG_TO */
/* Scan the commands attached to the object looking for a match */
/* If no match, scan the class of the object, and then the classes class */
/* And so on up the class hierarchy */

/* returns true if a F (fast) flagged exit is matched */
long send_msg_count;

int send_msg_to(thing, start_class)
dbref thing, start_class;
{	dbref exit, clas, last_exit;
	int depth = 0, depth2 = 0;
	int tomany=0;
	int amatch=0;
	int fast=0;

	if(bad_dbref(thing, 0))
		return 0;	/* Failed */
	if( Typeof(thing) == TYPE_EXIT)
		return 0;	/* Can't send messages to an exit */

	set_receiver(thing);	/* Thing that is being sent the message */

	if( check_argbuf()){
		return 1;
	}

	xargc = 1;
	xargp = argbuf;
	xargv = argvbuf;
	strcpy(xargp, Name0(cur_receiver));
	*xargv++ = xargp;
	while(*xargp++);
	*xargp= '\0';
	*xargv= NULL;

	if( start_class != NOTHING)
		clas = start_class;
	else
		clas = thing;

	clas = Check_defaults(clas);

	while(1){
		send_msg_count++;

		exit = Contents(clas);
		if( bad_dbref(exit, NOTHING)){
fprintf(logfile,"Bad contents for %d in send_msg_to(%s)\n", clas, match_name);
			return 1;	/* Treat bad as fast */
		}

		DOLIST(exit, exit){

			if( tomany++ > 2000){
fprintf(logfile,"To many in send_msg_to(%s, %d, %d, %d)\n", match_name, thing, clas, exit);
				return 1; /* Mark as fast! */
			}

			if( Typeof(exit) != TYPE_EXIT || Contents(exit) == NOTHING){
				continue;
			}else if(is_exported(exit) && !cmpexit( exit)){
			    /* we got it */
				addtomatchlist(exit);
				if( Flags(exit) & FEMALE){
					fast = 1;
				}
				amatch++;
			}
		}
		if( amatch)	/* We have some matches so don't search classes */
			return fast;

		clas = Class(clas);
		clas = Check_defaults(clas);
		if( bad_dbref(clas, NOTHING) )
			break;
		if( clas <= 0 || Typeof(clas) != TYPE_ROOM ||
			(Flags(clas) & INHERIT) != INHERIT){
			break;
		}
		if( depth++ > 20){
fprintf(logfile,"send_msg_to: depth loop in classes of %d\n", thing);
			break;
		}
	}

	if( Player(thing) && !amatch){	
/* If no match scan zone of here as if was player class*/
		clas = Zone(match_loc);

		if( !bad_dbref(clas, 0) && Typeof(clas) == TYPE_ROOM &&
		     (Flags(clas)&INHERIT) && clas != Class(match_loc)){

			while(1){
				send_msg_count++;

				exit = Contents(clas);
				if( bad_dbref(exit, NOTHING)){
fprintf(logfile,"Bad contents for %d in send_msg_to_zone(%s)\n", clas, match_name);
					return 1;	/* Treat bad as fast */
				}

				DOLIST(exit, exit){
					if( tomany++ > 2000){
fprintf(logfile,"To many in send_msg_to_zone(%s, %d, %d, %d)\n", match_name, thing, clas, exit);
						return 1; /* Mark as fast! */
					}

					if( Typeof(exit) != TYPE_EXIT || Contents(exit) == NOTHING){
						continue;
					}else if(is_exported(exit) && !cmpexit( exit)){
			    		/* we got it */
						addtomatchlist(exit);
						if( Flags(exit) & FEMALE){
							fast = 1;
						}
						amatch++;
					}
				}
				if( amatch)	/* We have some matches so don't search classes */
					return fast;

				clas = Class(clas);
				clas = Check_defaults(clas);
				if( bad_dbref(clas, NOTHING) )
					return 0;
				if( clas <= 0 || Typeof(clas) != TYPE_ROOM ||
					(Flags(clas) & INHERIT) != INHERIT){
					return 0;
				}
				if( depth++ > 20){
fprintf(logfile,"send_msg_to: depth loop in classes_zone of %d\n", thing);
					break;
				}
			}
		}
	}

	return 0;
}

/* This is the MAIN work 
 *
 * First the room is searched for a matching exit.
 * then all the things in the room and things the player
 * is carrying are searched. Then the player themself.
 * Then the other players,
 * Finally all things carried by other players are searched.
 *
 * If a match occurs with an exit that has the F flag set then the 
 * search is terminated early.
 */

void match_exit()
{
    dbref here, zone;
    dbref thing, who;
    int fast = 0;

	absolute = absolute_name();
	if(!controls(match_who, absolute)) absolute = NOTHING;

	if(absolute != NOTHING){
		addtomatchlist(absolute);
		exact_match = absolute;
		return;
	}

	here = loc_of(cur_sender);
	if( bad_dbref(here, 0)){
		return;
	}
/* This will look for exits in the room, commands in the class of the room */
	fast |= send_msg_to(here, NOTHING);

/* Scan the objects in the room */
	if( !fast){
		DOLIST(thing, Contents(here)){
			if( Typeof(thing) != TYPE_THING)
				continue;
			fast |= send_msg_to(thing, NOTHING);
			if( fast)
				break;
		}
	}

/* Scan the objects the player is carrying */
	if( !fast){
		DOLIST(thing, Contents(match_who)){
			if( Typeof(thing) != TYPE_THING )
				continue;
			fast |= send_msg_to(thing, NOTHING);
			if( fast)
				break;
		}
	}

/* Scan the player */
	if( !fast){
		fast |= send_msg_to(match_who, NOTHING);
	}

/* New stuff, look at objects that are carried but have WORLD (W)set*/
	if( !fast){
		DOLIST(who, Contents(here)){
			if( Typeof(who) != TYPE_PLAYER || who == match_who)
				continue;
			DOLIST(thing, Contents(who)){
				if( Typeof(thing) != TYPE_THING ||
				    (Flags(thing) & (WIZARD)) != (WIZARD) )
					continue;
				fast |= send_msg_to(thing, NOTHING);
				if( fast)break;
			}
			if( fast)
				break;
		}
	}

/* Scan the players in this Location */

	if( !fast){
		DOLIST(thing, Contents(here)){
			if( Typeof(thing) != TYPE_PLAYER || thing == match_who)
				continue;
			fast |= send_msg_to(thing, NOTHING);
			if( fast)
				break;
		}
	}
}

void match_everything()
{
    match_exit();
    match_neighbor();
    match_possession();
    match_me();
    match_here();
	match_absolute();
	match_player();
}

/* new refinement.
 * no_true set if exit names matched but no locks worked!
 */

dbref match_result()
{   dbref res;

    exact_match = new_choose();

    if(exact_match != NOTHING) {
	res = exact_match;
    } else {
	switch(match_count) {
	  case 0:
	    res = NOTHING;
	    break;
	  case 1:
	    res = last_match;
	    break;
	  default:
	    res = AMBIGUOUS;
	}
    }
    if( match_vars->No_true)
	    last_result = 0;	/* Return failure */

    restore_match_vars();
    return res;
}
	   
/* use this if you don't care about ambiguity */
dbref last_match_result()
{   dbref res;

    exact_match = new_choose();

    if(exact_match != NOTHING) {
	res =exact_match;
    } else {
	res = last_match;
    }
    if( match_vars->No_true)
	    last_result = 0;	/* Return failure */

    restore_match_vars();
    return res;
}

dbref noisy_match_result()
{
    dbref match;
    dbref who = match_who;

    match = match_result();	/* note: restores match stack */

    switch(match ) {
      case NOTHING:
	notify(who, NOMATCH_MESSAGE, NOTHING);
	return NOTHING;
      case AMBIGUOUS:
	notify(who, AMBIGUOUS_MESSAGE, NOTHING);
	return NOTHING;
      default:
	return match;
    }
}

/* SEND - Scan an object for a match against the message (command) */

dbref send_message(player, obj, message, auth)
dbref player, obj, auth;
char *message;
{	dbref saved_euid = euid;
	dbref result, thing;
	int amatch=0;
	dbref zone;


	if( bad_dbref(obj, 0)){
fprintf(logfile,"Bad dbref in send_message (%d,%d,%s,%d)\n", player, obj, message, auth);
		last_result =0;
		return NOTHING;
	}
	euid = auth;
	init_match_check_keys(player, message, NOTYPE);

	amatch = send_msg_to( obj, NOTHING);

/* Check the contents of the receiver */

	for(thing = Contents(obj); !amatch && thing != NOTHING; thing= Next(thing)){
		if( Typeof(thing) != TYPE_THING &&
		    !(Typeof(obj) == TYPE_ROOM && Typeof(thing)==TYPE_PLAYER) )
			continue;
		amatch |= send_msg_to( thing, NOTHING);
	}

	result = last_match_result();
	if( result >= 0){
		if( !no_true || (Flags(result)&TEMPLE)){
			updateage(result);
		}
	}
	if( result == NOTHING)
		last_result = 0;
	euid = saved_euid;
	return result;
}


dbref ssend_message(player, obj, message, auth, start_class)
dbref player, obj, auth, start_class;
char *message;
{	dbref saved_euid = euid;
	dbref result, thing;
	int amatch=0;

	if( bad_dbref(obj, 0)){
fprintf(logfile,"Bad dbref in ssend_message (%d,%d,%s,%d)\n", player, obj, message, auth);
		last_result = 0;
		return NOTHING;
	}

	if( bad_dbref(start_class, 0)){
		last_result = 0;
		return NOTHING;
	}
	start_class = Class(start_class);
		
	euid = auth;
	init_match_check_keys(player, message, NOTYPE);

	amatch = send_msg_to( obj, start_class);

	result = last_match_result();
	if( result >= 0){
		if(!no_true|| (Flags(result)&TEMPLE)){
			updateage(result);
		}
	}
	if( result == NOTHING)
		last_result = 0;
	euid = saved_euid;
	return result;
}


/* The following is used by @reset code in wiz.c */

dbref simple_send_message(player, obj, message, auth)
dbref player, obj, auth;
char *message;
{	dbref saved_euid = euid;
	dbref result, thing;
	int amatch=0;

	if( bad_dbref(obj, 0)){
fprintf(logfile,"Bad dbref in simple_send_message (%d,%d,%s,%d)\n", player, obj, message, auth);
		last_result =0;
		return NOTHING;
	}
	euid = auth;
	init_match_check_keys(player, message, NOTYPE);

	amatch = send_msg_to( obj, NOTHING);

	result = last_match_result();
	if( result >= 0 && !no_true){
		updateage(result);
	}
	if( result == NOTHING)
		last_result = 0;
	euid = saved_euid;
	return result;
}


