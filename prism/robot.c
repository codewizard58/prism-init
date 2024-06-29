/* 
 * Robot.c
 * Provide some basic automatics
 *
 * By P.J.Churchyard (c) 1991
 */
#include <stdio.h>

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "externs.h"
#include "fight.h"
#include "descript.h"

extern dbref euid;
void stop_bot();

#define NOT_NOTHING(x) (x!= NOTHING && x >= 0 && x < db_top)

extern int no_robots;

struct initbot{
const char *name;
const char *pass;
int	speed;
int	fight_p;
int	fight_sp;	/* Hit speed in fight */
int	hits_v;		/* number of hits when retaliating */
int	hits_a;		/* number of hits when attacking */
int	pennies;	/* starting pennies */
int	move_p, get_p, drop_p;
const char	*comment;
} init_bot_tab[] = {
/*         name    pass     sp fp fsp hv ha pen mp  gp  dp pose */
	{ "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0,""}
};

struct initbot input_bot_tab[2] ={
	{ "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0,""},
	{ "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0,""}
};

struct robot_list {
struct robot_list *next;
dbref  r;
struct text_queue input;
};

struct robot_list *robots;

int robot_running(player, bot)
dbref player,bot;
{	struct robot_list *rl = robots;

	for(; rl;rl=rl->next){
		if( rl->r == bot)
			return 1;
	}
	return 0;
}

int has_robot_running(player)
dbref player;
{	struct robot_list *rl = robots;

	for( ; rl ; rl = rl->next){
		if( Owner(rl->r) == player)
			return 1;
	}
	return 0;
}

/* Heartbeat code */

void start_heart(thing)
dbref thing;
{	struct robot_list *rl;
	dbref player;

	if(Typeof(thing) != TYPE_THING)
		return;

	player = Owner(thing);

	if( !ArchWizard(player) ){
		notify(player,"Heartbeats can only be set by Wizards!");
		Flags(thing) &= ~LINK_OK;
		return;
	}

	if( ( Flags(thing) &LINK_OK) != LINK_OK){
		notify(player, "Needs LINK_OK to be set");
		return;
	}
	rl = (struct robot_list *)malloc( Sizeof(struct robot_list));
	if( !rl){
		fprintf(stderr,"No memory in Start_heart!");
		return;
	}
	rl->next = robots;
	robots = rl;
	rl->r  = thing;
	rl->input.head = NULL;
	rl->input.tail = &rl->input.head;
}

void stop_heart(thing)
dbref thing;
{
	struct robot_list *rl, *rl1;

	if(Typeof(thing) != TYPE_THING)
		return;

	rl = robots;
	if(rl && rl->r == thing ){
		robots = rl->next;
		free( (FREE_TYPE) rl);
		Flags(thing) &= ~LINK_OK;
	}else { 
		while(rl && rl->next && rl->next->r != thing)
			rl = rl->next;

		if(rl && rl->next){
			rl1 = rl->next;
			rl->next = rl->next->next;
			free( (FREE_TYPE) rl1);
			Flags(thing) &= ~LINK_OK;
		}
	}
}


void do_heart(thing)
dbref thing;
{	dbref saved_euid = euid;
	char buf[256];

	if( (Flags(thing) & LINK_OK)!= LINK_OK){
		stop_heart(thing);
		return;
	}

	if( !Player(thing) )
		euid = Owner(thing);

	sprintf(buf,".heartbeat %s", Name(thing));

	send_message(thing, thing, buf, euid); 
	euid = saved_euid;
}


static char *parsep;

static int getstr( s)
char *s;
{	int ch, quoted;
	char *p = s;

	quoted = 0;
	while( (ch = *parsep++) )
		if(ch != ' ' && ch != '\t')
			break;
	if(!ch )return 0;
	if(ch == '"'){
		quoted = ch;
		ch = *parsep++;
	}
	*p = '\0';
	while(ch != EOF){
		if(ch == quoted)
			break;
		if(!quoted && (ch == ' ' || ch == '\t') )
			break;
		if(ch == '\n' || ch == '\r')
			break;
		*p++ = ch;
		ch = *parsep++;
	}
	if(ch != quoted)
		*--parsep = ch;

	*p++ = '\0';
	return 1;
}

static void skipeoln(f)
FILE *f;
{	int ch;
	ch = getc(f);
	while(ch != EOF && ch != '\n')
		ch = getc(f);
}

static int getnumb()
{	int n = 0;
	int ch;

	ch = *parsep++;
	while( (ch ==' '||ch== '\t'))
		ch = *parsep++;
	while(ch >= '0' && ch <= '9'){
		n = n*10 + ch - '0';
		ch = *parsep++;
	}
	*--parsep = ch;
	return n;
}

void parse_bot(str, ir)
char *str;
struct initbot *ir;
{	static char name[32],pass[32],comment[256];
	int ch;

	parsep = str;

	ch = *parsep++;
	if(ch == '#')
		return;

	ir->name = "";

	parsep--;

	if(!getstr( name))
		return;	
	if(!getstr( pass))
		return;
	ir->name = name;
	ir->pass = pass;
	ir->speed = getnumb();
	ir->fight_p=getnumb();
	getnumb();
	getnumb();
	getnumb();
	ir->move_p  =getnumb();
	ir->get_p   =getnumb();
	ir->drop_p  =getnumb();


	if(!getstr( comment))
		return;
	getnumb();
	ir->comment = alloc_string(comment);

}

int setup_bot(player, str)
dbref player;
char *str;
{	struct initbot *ir = input_bot_tab;
	struct robot   *r;
	char pass[32],*p;
	char user[32];
	int n;
	dbref bot;

	p = str;
	for(n=0; n <30; n++){
		user[n] = *p;
		if(!*p || *p == ' ')
			break;
		p++;
	}
	user[n] = '\0';

	while(*p && *p != ' ')p++;
	while(*p && *p == ' ')p++;
	for(n=0;n<30;n++){
		pass[n] = *p;
		if(!*p || *p == ' ')
			break;
		p++;
	}
	pass[n] = '\0';

	bot = lookup_player(user);
	if(bot == NOTHING ||
	   !Ext(bot).password ||
	   chkpass(pass, Ext(bot).password)){
fprintf(stderr,"Robot password failed, %s %s'%s'\n", user, pass, str);
		return 0;
	}

	if( !Player(bot) || !(Flags(bot)&ROBOT) ){
fprintf(stderr,"setup_bot: %s not a robot\n", Name(bot));
		return 0;
	}

	parse_bot(str, ir);

	if(Typeof(player) != TYPE_PLAYER)
		return 0;

	r = Ext(player).robot;
	if(!r){
		r = (struct robot *)malloc( Sizeof(struct robot));
		Ext(player).robot = r;
	}
	r->speed = ir->speed;
	r->fight_p=ir->fight_p;
	r->cur_speed = r->speed;
		
	Flags(player) |= ROBOT;
	Flags(player) &= ~WEAPON;

	r->comment= (char *)ir->comment;
	r->move_p = ir->move_p;
	r->get_p  = ir->get_p;
	r->drop_p = ir->drop_p;
	r->pass   = (char *)alloc_string(ir->pass);
	Level(player) = -1;
	Level(player) = player_level(player);
	return 1;

}

void start_bot(player, bot)
dbref bot,player;
{
	struct robot_list *rl;
	dbref rbot,owner;

	if(Typeof(bot)!= TYPE_PLAYER || !(Flags(bot)&ROBOT) ){
		notify(player, "Not a robot");
		return;
	}

	if( !Ext(bot).robot){
fprintf(stderr,"No robot info available! %d\n", bot);
	notify(player,"No robot info available!");
		return;
	}

	owner = Owner(bot);
	if( owner != player && owner != euid && !ArchWizard(player) ){
		notify(player, "Only a Wizard can start a ROBOT");
		fprintf(stderr,"%s failed to start robot %s\n",Name(player),Name(bot));
		return;
	}

	rbot = connect_player(Name(bot), Ext(bot).robot->pass, NULL);
	if( rbot == NOTHING){
fprintf(stderr,"Failed to connect robot! %s(%d, %s)\n", Name(bot), bot,
			Ext(bot).robot->pass);
		notify(player, "Could not connect robot!");
		return;
	}
		
	Flags(rbot) |= ROBOT;
	Flags(rbot) &= ~WEAPON;

	rl = (struct robot_list *)malloc( Sizeof(struct robot_list));
	rl->next = robots;
	robots = rl;
	rl->r  = rbot;
	rl->input.head = NULL;
	rl->input.tail = &rl->input.head;
fprintf(logfile,"Started robot %s(%d)\n", Name(bot), bot);
	notify(player, "Started");
	Age(rbot) = timenow;
}

void init_robots()
{	dbref player;

	for(player=0; player < db_top; player++){
		if(Typeof(player) == TYPE_PLAYER && (Flags(player) & ROBOT) )
			start_bot( Owner(player), player);
		else if( Typeof(player)== TYPE_THING && (Flags(player)&LINK_OK) )
			start_heart(player);
	}
}

void move_bot();
void exit_bot( PROTO4(dbref, int, dbref, struct dbref_chain *));
void pick_bot(), drop_bot();
int heartbeat = 0;

void bot_command(bot, command, arg)
dbref bot;
char *command, *arg;
{	char buf[256];
		sprintf(buf, "%s %.150s", command, arg);
		process_command(bot, buf);
}

void do_robots()
{
	struct robot_list *r;
	struct robot *r1;
	struct text_block *t;

	heartbeat--;

	for(r=robots; r; r = r->next){
		if( Typeof(r->r) == TYPE_THING){
			if(!heartbeat){
				do_heart(r->r);
				continue;
			}
		}else if(Typeof(r->r)== TYPE_PLAYER && Robot(r->r)){
			if( ( !Ext(r->r).robot) ){
				fprintf(stderr,"Skipping robot %d: stopped\n", r->r);
				stop_bot(1, r->r);
				return;
			}else {
				r1 = Ext(r->r).robot;
				t = r->input.head;
				if( t){
					/* unlink the block we are going to throw */
					r->input.head = t -> nxt;
					if (!r->input.head)
						r->input.tail = &r->input.head;
					process_command( r->r, t->start);
			    		free_text_block (t);
				}else {
					r1->cur_speed--;
					if( r1->cur_speed <= 0){	/* make a move */
						r1->cur_speed = r1->speed;
							move_bot(r->r);
					}
				}
			}
		}else { /* Bad automatic object */
		}
	}
	if(heartbeat <= 0)
		heartbeat = 8;
}

void move_bot(bot)
dbref bot;
{	dbref thing,thing_2;
	int exits, o_exits;
	int objs, o_objs;
	int players;
	int carried;
	static char buf[128];
	int n, depth;
	struct robot *r;
	dbref loc, saved_euid = euid;
	struct dbref_chain *c_exits, *c_oexits;

	loc = Location(bot);
	if( bad_dbref(loc, 0)){
fprintf(stderr,"Bad location %d for robot %s(%d)\n", loc, Name(bot), bot);
		return;
	}

	if( Typeof(bot) != TYPE_PLAYER || (Flags(bot) & ROBOT)==0){
fprintf(stderr,"Not a bot in move bot(%d)\n", bot);
		return;
	}

	c_oexits = c_exits = NULL;

	r = Ext(bot).robot;
	if( !r){
fprintf(stderr,"move_bot: r == NULL for %d\n", bot);
		return;
	}

	euid = NOTHING;		/* 13/8/92 clear euid... */
	exits = objs = players = carried = 0;
	o_exits = 0;
	o_objs  = 0;

/* count the number of objects that the bot is carrying */
	for( thing = Contents(bot); NOT_NOTHING(thing) &&
			o_exits < 50 && exits < 50 &&
			carried < 200; thing = Next(thing)){
		if( Typeof(thing) == TYPE_EXIT && (Flags(thing)&INHERIT) == 0){
			if( Owner(thing) == bot || (Flags(thing) & ROBOT) ){
				c_oexits = add_chain(c_oexits, thing);
				o_exits++;
			}else{
				c_exits = add_chain(c_exits, thing);
				exits++;
			}
			continue;
		}
		carried++;
		if(Contents(thing)){
			for(thing_2= Contents(thing); NOT_NOTHING(thing_2) &&
					o_exits < 50 && exits < 50 ;
					 thing_2= Next(thing_2) ){
				if(Typeof(thing_2) != TYPE_EXIT || (Flags(thing_2) & INHERIT))
					continue;
				if(Owner(thing_2) == bot || (Flags(thing_2)&(ROBOT) ) ){
					c_oexits = add_chain(c_oexits, thing_2);
					o_exits++;
				}else{
					c_exits = add_chain(c_exits, thing_2);
					exits++;
				}
			}
		}
	}
/* count other objects */

	for( thing = Contents( loc); NOT_NOTHING(thing) &&
		players < 50 && o_exits < 50 && exits < 50 ;thing= Next(thing) ){
		switch(Typeof(thing)){
		case TYPE_PLAYER:
			if(thing != bot && !Dark(thing) &&
				(logged_in(thing) || (Flags(thing)&ROBOT)))
				players++;
			break;
		case TYPE_EXIT:
			if( Flags(thing)&INHERIT)
				continue;
			if( (Owner( thing) == bot) || ( Flags(thing) & ROBOT)){
				c_oexits = add_chain(c_oexits, thing);
				o_exits++;
			}else{
				c_exits = add_chain(c_exits, thing);
				exits++;
			}
			break;
		default:
		     if( Owner(thing) == bot)
			o_objs++;
		     else
			objs++;
		     if( o_objs > 200 || objs > 200){
fprintf(stderr,"To many objects(%d)\n", bot);
			goto tomany1;
		     }
		}
	}
tomany1:
/* Now decide to do something */
	if(players && (random()%100 < r->fight_p)){
		n = random()%players;
		depth = 0;
		for(thing = Contents(Location(bot)) ;depth++ < 200 &&
		    NOT_NOTHING(thing);thing= Next(thing)){
			if(Player(thing) && thing != bot &&
			   !n && !Dark(thing) && Owner(thing) != Owner(bot) &&
				(logged_in(thing) || (Flags(thing)&ROBOT)) ){
				euid = wizauth();
				sprintf(buf,".attack %s by robot", Name(thing) );
				bot_command(bot, buf, "");
				break;
			}else
				n--;
		}
		goto done;
	}
	if( (random()%100) < r->move_p){
		if(o_exits){	/* Take an exit we own */
			exit_bot(bot, o_exits, bot, c_oexits);
		}else if(exits){
			exit_bot(bot, exits, bot, c_exits);
		}
		goto done;
	}
	free_chain(c_oexits);
	free_chain(c_exits);
	c_oexits = c_exits = NULL;

	if( (random()%100) < r->get_p){
		if(o_objs){	/* Pick something up */
			pick_bot(bot, o_objs, bot);
			euid = saved_euid;
			return;
		}else if(objs){
			pick_bot(bot, objs, NOTHING);
			euid = saved_euid;
			return;
		}
	}
	if( (random()%100) < r->drop_p){
		if(carried){
			drop_bot(bot, carried, NOTHING);
			euid = saved_euid;
			return;
		}
	}
	sprintf(buf, ":%s", r->comment);
	bot_command(bot, buf, "");
done:
	if(c_oexits)
		free_chain(c_oexits);
	if(c_exits)
		free_chain(c_exits);
	euid = saved_euid;
	return;
}

void exit_bot(bot, n, owner, t)
dbref bot, owner;
int n;
struct dbref_chain *t;
{	dbref thing, thing_2;
	char buf[128], *p;
	int savedn = n;
	struct dbref_chain *st = t;

	n = random() % n;	/* Take the nth exit */

	while(n && t){
		n--;
		t = t->next;
		if( !t)
			t = st;
	}

	if( t){
		thing = t->this;
		strncpy(buf, Name(thing), 120);
		buf[120] = '\0';
		p = buf;
		while(*p && *p != ';'){
			p++;
		}
		*p = '\0';
		bot_command(bot, buf, "");
		return;
	}

fprintf(stderr,"Robot %d failed to find his exit of %d!\n", bot, savedn);
}

void pick_bot(bot, n, owner)
dbref bot, owner;
int n;
{	dbref thing;
	char buf[128], *p;
	
	n = random() % n;	/* Take the nth object */
	for(thing= Contents(Location( bot));
		NOT_NOTHING(thing);
			thing= Next(thing) ){
		if( thing != bot && !Player(thing) && Typeof(thing)!= TYPE_EXIT && (
		     (owner == NOTHING && Owner(thing) != bot) ||
		     (Owner(thing) == bot) )
		  ){
			if( !n ){
/* make a move */		strcpy(buf, Name(thing));
				p = buf;
				while(*p && *p != ';'){
					p++;
				}
				bot_command(bot, "get", buf);
				return;
			}else n--;
		}
	}
}


void drop_bot(bot, n, owner)
dbref bot, owner;
int n;
{	dbref thing;
	char buf[128], *p;
	
	n = random() % n;	/* Take the nth exit */
	for(thing= Contents(bot); NOT_NOTHING(thing);thing= Next(thing)){
		if( thing != bot ){
			if( Typeof(thing) == TYPE_EXIT)
				continue;
			if( !n ){
/* make a move */		strcpy(buf, Name(thing));
				p = buf;
				while(*p && *p != ';'){
					p++;
				}
				bot_command(bot, "drop", buf);
				return;
			}else n--;
		}
	}
}

void stop_bot(player, victim)
dbref player, victim;
{	struct robot_list *rl, *rl1;
	dbref owner;

	if( victim < 0)
		return;

	owner = Owner(victim);

	if( !ArchWizard(player) && player != owner && euid != owner){
		notify(player,"You cannot stop that");
		return;
	}
	rl = robots;
	if(rl && rl->r == victim ){
		robots = rl->next;
		victim = rl->r;
		free( (FREE_TYPE) rl);
	}else { 
		while(rl && rl->next && rl->next->r != victim)
			rl = rl->next;
		if(!rl || !rl->next){
			notify(player, "Robot not running");
			return;
		}
		rl1 = rl->next;
		victim = rl1->r;
		rl->next = rl->next->next;
		free( (FREE_TYPE) rl1);
	}

	sprintf(msgbuf, "Robot %s(%d) stopped by %s(%d)", Name(victim), victim,
			Name(player), player);
	notify(player, msgbuf);
	fprintf(logfile,"%s\n", msgbuf);
	Owner(victim) = victim;		/* Stopped robots own themselves */
}

/*
 * @robot rob=pass sp fp 0 0 pen mp gp dp pose fsp
 * @robot rob=/start
 * @robot rob=/stop
 * @robot rob=/clear
 * @robot
 */


void at_robot(player, arg1, arg2)
dbref player;
char *arg1, *arg2;
{
	dbref victim;
	char buf[256], *p;
	struct robot_list *rl;
	struct robot *r;

	if(!*arg1 ){
		for(rl = robots; rl ; rl = rl->next){
			if( Typeof(rl->r) == TYPE_PLAYER){
#ifndef MSDOS
				sprintf(msgbuf,"%-20s . %05d %s", Name(rl->r),
					Pennies(rl->r), 
					Name(Location(rl->r) ));
#else
				sprintf(msgbuf,"%-20s . %05ld %s", Name(rl->r),
					Pennies(rl->r), 
					Name(Location(rl->r) ));
#endif
			}else
				sprintf(msgbuf,"%-20s .       %s", Name(rl->r), 
					Name(Location(rl->r) ));
			p = msgbuf;
			while(*p){
				if(*p == ' '){
					p++;
					while(*p == ' ')*p++ = '.';
				}else p++;
			}
			if( !Robot(rl->r))
				strcpy(p, " Heartbeat");

			if( Level(player) >= 14 || Owner(rl->r)==player)
				notify(player, msgbuf);
		}
		notify(player, "-----------------");
		return;
	}

	init_match(player, arg1, TYPE_PLAYER);
	match_neighbor();
	match_me();
	match_absolute();
	match_player();

	if((victim = noisy_match_result()) == NOTHING) {
	    notify(player,"Cant find it!");
	    return;
	}
	if( Typeof(victim) != TYPE_PLAYER || !(Flags(victim) & ROBOT)){
		notify(player,"Not a robot!");
		return;
	}
	if( !*arg2){
		notify(player, "Try look");
		return;
	}
	if( *arg2 == '/'){
		if(arg2[1] == 's' && arg2[2]== 't'){
			if(arg2[3] == 'a'){	/* /start */
				start_bot(player, victim);
				return;
			}else if(arg2[3] == 'o'){/* /stop */
				stop_bot(player, victim);
				return;
			}
		}else if( arg2[1] == 'c' && arg2[2] == 'l' ){ /* clear */
			if( robot_running(player, victim)){
				notify(player,"You must stop it first!");
				return;
			}
			if( Ext(victim).robot){
				free( (FREE_TYPE) Ext(victim).robot);
			}
			Ext(victim).robot = NULL;
			Flags(victim) &= ~ROBOT;
			notify(player,"Cleared robot stats!");
			Owner(victim) = victim; /* Give it back */
			return;
		}else {
			notify(player, "Unknown @robot command");
			return;
		}
	}
	if(euid != NOTHING){
		notify(player,"Can only be TOP level command");
		return;
	}
	if( robot_running(player, victim)){
		notify(player, "You will have to stop it first");
		return;
	}

	if( !ArchWizard(player) ){
		notify(player, "Wizards only!");
		return;
	}

	sprintf(buf, "%s %s", Name(victim), arg2);
	if( !setup_bot(victim, buf) ){
		notify(player,"Bad password");
		return;
	}

	r = Ext(victim).robot;
	if(r){	/* Do a basic validity check */
		if( r->speed < 1 || r->speed > 99 ||
		    r->fight_p < 0 || r->fight_p > 75
		){
			notify(player, "Robot stats out of range!");
			free( (FREE_TYPE) r);
			Ext(victim).robot = NULL;
			return;
		}
		notify(player, "Set.");

		Owner(victim) = player; /* You get it */
	} 
}

/* Added 24/9/91 */

static char push_buf[512];

void push_command_robot(player, msg)
dbref player;
char *msg;
{	int cnt;
	char *p, *p2;
	struct robot_list *r = robots;

	for(r = robots; r ; r = r->next){
		if( r->r == player)
			break;
	}
	if( !r)
		return;

	if( !queue_full(&r->input))
		push_on_queue( &r->input, msg, strlen(msg)+1);
	else {
fprintf(logfile,"Queue full in push_command_robot(%d, '%s')\n", player, msg);
	}
}

void they_heard(player, who, msg1, msg2)
dbref player, who;
char *msg1, *msg2;
{	int cnt;
	char *p, *p2;
	struct robot_list *r = robots;

	for(r = robots; r ; r = r->next){
		if( r->r == player)
			break;
	}
	if( !r)
		return;

	sprintf(push_buf, "<%s> ", Name(who));
	p2 = push_buf;
	while(*p2)p2++;		/* find end */
	p = p2;
	cnt = 0;

	while(msg1){
		*p = *msg1++;
		switch(*p){
		case '\0':
			msg1 = msg2;
			msg2 = NULL;
			if( msg1){
				continue;	/* concatenate */
			}
		case '\n':
			break;
		default:
			cnt++;
			if( cnt > 400)
				break;
			p++;
		case '\r':
			continue;
		}

		*p = '\0';
		if( !queue_full(&r->input))
			push_on_queue( &r->input, push_buf, strlen(push_buf)+1);
		else {
fprintf(logfile,"Queue full in they_heard(%d, '%s')\n", player, push_buf);
		}
		p = p2;
		cnt = 0;
	}
}

robot_notify_except( cont, player, msg)
dbref cont, player;
char *msg;
{	dbref thing;
	int depth = 0;

	for(thing = cont; depth++ < 500 && thing != NOTHING; thing = Next(thing)){
		if( (Flags(thing)&(ROBOT|LINK_OK))==(ROBOT|LINK_OK)&&
		    Typeof(thing) == TYPE_PLAYER &&
			thing != player){
			they_heard(thing, player, msg, NULL);
		}
	}
}
