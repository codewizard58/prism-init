#include "copyrite.h"

/* Predicates for testing various conditions */

#include <ctype.h>

#include "db.h"
#include "interfac.h"
#include "config.h"
#include "externs.h"

extern dbref euid;

int can_contain(thing)
dbref thing;
{
	if( thing >= 0 && thing <= db_top)
		switch(Typeof(thing)){
		case TYPE_PLAYER:
		case TYPE_ROOM:
			return 1;
		case TYPE_THING:
			if(Flags(thing) & CONTAINER)
				return 1;
			else
				return 0;
		default:
		     break;
		}
	return 0;
}

int can_link_to( who, where)
dbref who, where;
{
   if( !where)
	return 1;

    return(where >= 0
	   && where < db_top
	   && (Typeof(where) == TYPE_ROOM || ( Typeof(where)==TYPE_THING && (Flags(where) & CONTAINER) ) )
	   && (controls(who, where) || (Flags(where) & LINK_OK)));
}

int could_doit( player, thing)
dbref player, thing;
{
    if( thing < 0 || thing >= db_top){
fprintf(logfile,"could_doit:Bad thing value %d, player=%s\n",thing, Name(player));
	return 0;
    }
    switch(Typeof(thing) ){
	case TYPE_EXIT:
		if( Contents(thing) == NOTHING)	/* Unlinked */
			return 0;
		break;
	default:
		break;
    }
    if( Typeof(thing) == TYPE_EXIT)
	    return(eval_boolexp (player, Key(thing) ,thing));
    return 0;
}

int could_doit2( player, thing, nlock)
dbref player, thing;
int nlock;
{   BOOLEXP_PTR b;

if(nlock)
fprintf(logfile,"nlock != 0 in could_doit2(%d, %d, %d)\n", player, thing, nlock);

    switch(Typeof(thing) ){
	case TYPE_EXIT:
		if( Contents(thing) == NOTHING)
			return 0;
		if( (b = Key(thing) ) == TRUE_BOOLEXP)
			return 1;	/* not locked! */
  	  	return(eval_boolexp (player, b ,thing));
	default:
		break;
    }
    return 0; 

}

int can_doit2( player, thing, default_fail_msg, nlock)
dbref player, thing;
const char *default_fail_msg;
int nlock;
{
    dbref loc;

    if((loc = getloc(player)) == NOTHING) return 0;

    if(!could_doit2(player, thing, nlock) ) {
	return 0;
    } else {
	return 1;
    }
}


int can_doit( player, thing, default_fail_msg)
dbref player, thing;
const char *default_fail_msg;
{
	return can_doit2(player, thing, default_fail_msg, 0);
}

int can_see( player, thing, can_see_loc)
dbref player, thing;
int can_see_loc;
{
    if(player == thing || Typeof(thing) == TYPE_EXIT) {
	return 0;
    } else if(can_see_loc) {
	return(!Dark(thing) || controls(player, thing));
    } else {
	/* can't see loc */
	return(controls(player, thing));
    }
}

int controls( who, what)
dbref who, what;
{
    if( what < 0 || what >= db_top)
	return 0;	/* not valid */

    if( ArchWizard(who))
	return 1;

    if( Owner( what) == who)return 1;

    if( euid != NOTHING)
	return Owner( what) == euid;
    return 0;
}

int can_link( who, what)
dbref who, what;
{
    return((Typeof(what) == TYPE_EXIT && 
	Contents(what) == NOTHING)		/* Unlinked ? */
	   || controls(who, what));
}

int payfor( who, cost)
dbref who;
money cost;
{
    if(Pennies(who) >= cost) {
	Pennies(who) = Pennies(who) - cost;
	return 1;
    }
    return 0;
}

char *index(s,c)
char *s,c;
{
	while(s && *s && *s != c){
		s++;
	}
	if(s && *s)
		return s;
	return (char*)NULL;
}

int ok_player_name_sub(name)
char *name;
{   char *p;

    p = name;
    while(p && *p){
	if( *p == '\\')
		return 0;
	if( *p == ' ')
		return 0;
	if( *p == '$')
		return 0;
	p++;
    }

    return (name && *name
	    && *name != LOOKUP_TOKEN
	    && *name != NUMBER_TOKEN
	    && ok_name(name));
}		

int ok_name(name)
char *name;
{  char *p;

   if( strlen(name) > PLAYER_NAME_LIMIT)
	return 0;	/* Too long */

    return (name && *name
	    && astrcmp(name, "me")
	    && astrcmp(name, "home")
	    && astrcmp(name, "here")
	    && astrcmp(name, "myself") 
	    && astrcmp(name, "someone") 
	    && astrcmp(name, "it")
    );
}

int isoknam(ch)
char ch;
{	if(ch >= 'a' && ch <= 'z')return 1;
	if(ch >= 'A' && ch <= 'Z')return 1;
	if(ch >= '0' && ch <= '9')return 1;
	if(ch == '$' || ch == '\'' || ch == '_' ||
		ch == '-' || ch =='"')return 1;
	return 0;
}

int ok_player_name(name)
char *name;
{
    const char *scan;

    if(!ok_player_name_sub(name) || strlen(name) > PLAYER_NAME_LIMIT) return 0;

    if( strlen(name) < 3) return 0;	/* pjc 6-6-91 */

    for(scan = name; *scan; scan++) {
	if(!isoknam(*scan) ) { /* was isgraph(*scan) */
	    return 0;
	}
    }

    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}

int ok_password(password)
char *password;
{
    const char *scan;
    int cnt = 0;

    if(*password == '\0') return 0;

    for(scan = password; *scan ; scan++,cnt++) {
	if(!(isprint(*scan) && !isspace(*scan))) {
	    return 0;
	}
    }
    if( cnt >= 5)
	return 1;
    return 0;	/* To short */
}

