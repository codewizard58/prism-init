#include "copyrite.h"

/* attack and flee */

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "match.h"
#include "externs.h"
#include "fight.h"

#define LEV_BARS 4			/* how often there is a BAR */

#ifndef FREE_TYPE
#ifdef MSDOS
#define FREE_TYPE (char *)
#else
#define FREE_TYPE (void *)
#endif
#endif


extern dbref euid;

extern void need_ext();
extern int debug;
extern int last_result;


struct player_levels player_level_table [22] =
{
{	10,        	"", 			"", 20},
{	20,        	"Novice ", 		"Novice ", 50},
{	50,        	"Initiate ", 		"Initiate ", 70},
{	100,       	"Brother ",		"Sister ", 80},
{	200,       	"Pauper ",		"Pauper ", 90},
{	500,       	"Mr ",			"Miss ", 100},
{	1000,      	"Master ",		"Mistress ", 120},
{	2000,      	"Scout ",		"Guide ", 140},
{	5000,      	"Yuppie ",		"Yuppie ", 160},
{	10000,     	"Manager ",		"Manageress ", 180},
{	20000,     	"Chairman ",		"Chairwoman ", 200},
{	50000,     	"Lord ",		"Lady ", 220},
{	100000,    	"Duke ",		"Duchess ", 240},
{	200000,    	"Prince ", 		"Princess ", 270},
{	500000,    	"King ",		"Queen ", 300},
{	1000000,   	"Enchanter ",		"Enchantress ", 330},
{	2000000,   	"High Enchanter ",	"High Enchantress ", 360},
{	5000000,   	"Royal Enchanter ",	"Royal Enchantress ", 400},
{	10000000,  	"Novice Wizard ", 	"Novice Witch ", 400},
{	20000000,   	"Apprentice Wizard ", 	"Apprentice Witch ", 400},
{       50000000,     	"Wizard ", 		"Witch ", 1000},
{       100000000,     	"Arch Wizard ",		"Arch Witch ", 10000}
};
extern int boolexp_depth;

int player_level( p)
dbref p;
{	struct player_levels *t = &player_level_table[0];

	int l,oldlev;
	money pennies;

	if( p < 0 || p >= db_top){
fprintf(stderr,"player_level: %d is Bad player\n", p);
		return 0;
	}
	if( !Player( p))
		return 0;

	pennies = Pennies(p);
	oldlev = Level(p);

	for(l=0; l < 21 && pennies > t->pennies; t++)l++;


	if( (euid != NOTHING || !ArchWizard(p)) &&
	    oldlev != -1){
/* Check for bars */
		if( ( l / LEV_BARS) > (oldlev / LEV_BARS) ){ /* Trying to jump bars */
			l = LEV_BARS*(oldlev/LEV_BARS)+LEV_BARS-1;
			Pennies(p) = player_level_table[l].pennies-1;
		}else if( l < oldlev)
			l = oldlev;	/* Don't go down BECAUSE you paid! */
 	}

	Level(p) = l;
	if( Chp(p) > Mhp(p)){
		Chp(p) = Mhp(p);
	}

	if( Chp(p) < 0){		/* If dead restart with half */
		Chp(p) = Mhp(p)/2;
	}

	return(l);
}

void print_player_levels(player, arg1)
dbref player;
char *arg1;
{	struct player_levels *t;
	int n, lev = Level(player);
	int start;
	char *p;

	if(arg1)
		start = atoi(arg1);
	else
		start = 0;
	start = start * LEV_BARS;
	sprintf(msgbuf,"Lev Pennies .. Male Title ........... Female Title .......");
	p = msgbuf;
	while(*p){
		if(*p == ' '){
			p++;
			while(*p == ' ')*p++ = '.';
		}else p++;
	}
	notify(player, msgbuf);

	sprintf(msgbuf,"==========================================================");
	notify(player, msgbuf);

	t = &player_level_table[start];

	for(n=0; n+start < 22 && n < 12;n++){
#ifdef MSDOS
		sprintf(msgbuf, "%c%02d %10ld %-20.20s . %-20.20s", (lev == n+start)?'*':' ', n+start,
			t->pennies, t->mtitle, t->ftitle);
#else
		sprintf(msgbuf, "%c%02d %10d %-20.20s . %-20.20s", (lev == n+start)?'*':' ', n+start,
			t->pennies, t->mtitle, t->ftitle);
#endif
		t++;
		p = msgbuf;
		while(*p){
			if(*p == ' '){
				p++;
				while(*p == ' ')*p++ = '.';
			}else p++;
		}
		notify(player, msgbuf);
		if( (n+start)% LEV_BARS == LEV_BARS-1){
			notify(player,"----------------------------------------------------------");
		}
	}
}



const char *player_title( p )
dbref p;
{	int lev,depth =0;
	int saved_flags = Flags(p);
	char *class_name, *title;
	static char buf[80];
	dbref classof;

	if( bad_dbref(p, 0))
		return "";

	if( !Player( p ) )	/* Non players don't have titles */
		return "";

	if(TrueWizard(p)){
		Flags(p) &= ~WIZARD;
		Level(p) = -1;	/* pretend it is new */
		lev = player_level(p);
	}

	lev = Level( p);
	Flags(p) = saved_flags;

	classof = Class(p);
	depth = 0;
	class_name = NULL;	
	while(depth++ < 20){
		if( bad_dbref(classof, 0))
			break;
		if( !(Flags(classof)&INHERIT) )
			break;
 		if((Flags(classof)&STICKY)){
		    if(ArchWizard(Owner(classof))){
			class_name = Name(classof);
		    }
		    break;
		}
		/* try the next in the hierarchy */
		classof  = Class(classof);
	}

	if( lev < 0 || lev > 22)
		lev = 0;

	if( Female(p))
		title = (char *)player_level_table[lev].ftitle;
	else    title = (char *)player_level_table[lev].mtitle;

	if(class_name)
		sprintf(buf,"%s ", class_name);
	else if( Flags(p) & ROBOT){
		return "";
	}else {
		strcpy(buf, title);
	}
	return buf;
}

/* 10/3/93
 * New death penalty. You lose 1/3 of the pennies for this level
 * So you could die twice without incuring a loss in level.
 */

void kill_player(player)
dbref player;
{	int levbefore;
	int auth = 1;
	int saved_res = last_result;
	money pen, lowerpen;
	int curchp;

	if( default_wizard > 1)
		auth = default_wizard;

	if( Player(player)){
		levbefore = Level(player);
sprintf(msgbuf,"Killing player %s(%d) was %d ", Name(player), player, Level(player));
		if(levbefore > 0){
			lowerpen = player_level_table[levbefore-1].pennies;
			pen = player_level_table[levbefore].pennies - lowerpen;
		}else {
			pen = player_level_table[0].pennies;
			lowerpen = 0;
		}
		pen = pen / 3;
/*		Pennies(player) = Pennies(player)/2; */
#ifdef MSDOS
fprintf(logfile,"Pennies=%ld pen = %ld\n", Pennies(player), pen);
#else
#endif

		if( Pennies(player) > pen){ /* Don't set pennies negative */
			Pennies(player) = Pennies(player) - pen;
		}
		if( Pennies(player) < lowerpen){
			Level(player) = Level(player) -1;
		}

		if( Level(player) < 0)
			Level(player) = 0;

		Chp(player) = Mhp(player) /2;

fprintf(logfile,"%sis now %d\n", msgbuf, Level(player) );

		sprintf(msgbuf, ".died %s", Name(player));
		simple_send_message(player, player, msgbuf, auth);

		last_result = saved_res;
	}
}	


int do_heal_sub(player, victim, cost)
dbref player,victim;
money cost;
{	int oldhp;

	oldhp = Chp(victim);

	Chp(victim) += cost;

	if( Chp(victim) >= Mhp(victim) ){
		Chp(victim) = Mhp(victim);
		return 1;

	}else if( Chp(victim) <= 0){	/* Kill player */
		kill_player(victim);
		return 0;
	}
	if( cost < 0)
		return 1;	/* they survived */
	return 0;		/* No change */
}


void do_heal(player, arg1)
dbref player;
char *arg1;
{
	money cost;

	if( !ArchWizard(player) ){
		notify(player,"Sorry, can't heal now");
		return;
	}

	cost = atomoney(arg1);

	do_heal_sub(player, player, cost);
}

int health(player)
dbref player;
{	int curchp = Chp(player);
	int mhp = Mhp(player);

	if( curchp < mhp/4)	/* sick */
		return 0;
	if( curchp < mhp/2)	/* weak */
		return 1;
	if( curchp < (mhp*3)/4) /* normal */
		return 2;
	return 3;		/* strong */
}

char *diagnose(player)
dbref player;
{
	switch(health(player)){
	case 3:
		return "strong";
	case 2:
		return "in good health";
	case 1:
		return "weak";
	}
	return "sick";
}


