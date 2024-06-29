#include "copyrite.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#ifndef MSDOS
#include <sys/wait.h>
#else
#include <stdlib.h>
#endif

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "match.h"
#include "externs.h"

void do_noquit_scan();
#include <time.h>

#include "os.h"
#include "bsx.h"

char *ranmsg( PROTO2(char **, int));

extern dbref cur_player;

/* declarations */
const char *dumpfile = 0;
static int epoch = 0;
int alarm_triggered = 0;
int dumptype = 0;   /* 0 = normal, 1 = xml */
static int alarm_block = 0;
char *Lastcommand, *Lastsendcom;

static void fork_and_dump( PROTO(void));
void dump_database( PROTO(void));

#include "descript.h"
extern struct descriptor_data *descriptor_list;

extern int last_result;		/* from boolexp.c */
extern int extracting;		/* from extract.c */
extern char version_buf[];
extern char home_dir[];		/* pjc 30 Sep 95 */

static void inform_all( buf )
char *buf;
{
	struct descriptor_data *d;
	
	for (d = descriptor_list; d; d = d->next) {
		mud_write (d->descriptor, buf, strlen (buf));
	}
}

int logged_in(player)
dbref player;
{
	struct descriptor_data *d;

	for(d = descriptor_list; d; d= d->next)
		if(d->player == player)return 1;
	return 0;
}


void do_quit(player)
dbref player;
{
	struct descriptor_data *d;

	for(d = descriptor_list; d; d= d->next)
		if(d->player == player)
			d->connected = 3;		/* new state */
}


char *saved_dumpfile;

void do_dump(dbref player, char *arg)
{

    if(is_god(player)) {
		alarm_triggered = 1;
		if(arg && *arg=='/' && strncasecmp(arg, "/xml", 4)){
			dumptype = 1;
		}
			notify(player, "Dumping...");
    } else {
	notify(player, "Sorry, you are in a no dumping zone.");
    }
}

void do_shutdown(player)
dbref player;
{
    if(is_god(player)) {
	fprintf(logfile, "SHUTDOWN: by %s\n", unparse_object(player, player));
	fflush(logfile);
	shutdown_flag = 1;
    } else {
	if(ArchWizard(player))
		notify(player, "Sorry, only Real GODs can shutdown!");
	else
		notify(player, "Your delusions of grandeur have been duly noted.");
    }
}
	
/* should be void, but it's defined as int */
int alarm_handler()
{
    alarm_triggered = 1;
    if(!alarm_block) {
        fork_and_dump();
    }
#ifndef MSDOS
#ifndef WINDOWS
    signal(SIGALRM, alarm_handler); /* re-catch the alarm */
#endif
#endif
    return 0;
}

static void dump_database_internal()
{
    char tmpfile[128];
    FILE *f;
    int saved_alarm_block = alarm_block;
    char *p;

fprintf(logfile,"Dump directory is '%s'\n", home_dir);
	if( home_dir[0]){
		chdir(home_dir);
	}	
	alarm_block = 1;	/* Dont checkpoint with here! */
	strncpy(tmpfile, dumpfile, 127);
	tmpfile[127] = '\0';
	p = tmpfile;
	while(*p)p++;
#ifdef MSDOS
	while(p != tmpfile && p[-1] != '/' && p[-1] != '\\')p--;
	while(*p && *p != '.')p++;
#endif

	if( epoch > 1){
    	sprintf(p, ".%03d", epoch - 2);
    
		unlink(tmpfile);		/* nuke our predecessor */
	}
    
    if( epoch > 0){
    	sprintf(p, ".%03d", epoch - 1);

		unlink(tmpfile);		/* nuke our predecessor */
    
		rename(dumpfile, tmpfile);
	}

    sprintf(p, ".%03d", epoch);
    
    if((f = fopen(tmpfile, "w")) != NULL) {
		if( dumptype){
			db_write_xml(f);
		}else {
			db_write(f);
		}
		fclose(f);
		dumptype = 0;

#ifdef MSDOS
		unlink(dumpfile);
#endif
		if(rename(tmpfile, dumpfile) < 0){
			sprintf(tmpfile, "rename(%s.%03d,%s)", dumpfile, epoch, dumpfile);
			perror(tmpfile);
		}
	} else {
		sprintf(tmpfile, "fopen(%s.%03d)", dumpfile, epoch);
		perror(tmpfile);
    }
	alarm_block = saved_alarm_block;
}

void panic(message)
const char *message;
{
    char panicfile[128];
    FILE *f;
    int i;
	char *actime();
	char *p;

	alarm_block = 1;


    /* turn off signals */
    for(i = 0; i < NSIG; i++) {
	signal(i, SIG_IGN);
    }
	if(Lastcommand)
		fprintf(logfile,"\nLastcommand\n%s", Lastcommand);

	if( Lastsendcom)
		fprintf(logfile, "\nLastsendcom\n%s", Lastsendcom);

	fprintf(logfile, "\n%s PANIC: %s\n", actime(&timenow), message);

    /* shut down interface */
    emergency_shutdown();

	strncpy(panicfile, dumpfile, 127);
	panicfile[127] = '\0';
	p = panicfile;

	while(*p)p++;
	while(p != panicfile && p[-1] != '/' && p[-1] != '\\')p--;
	while(*p && *p != '.')p++;

    sprintf(p, ".pan");
    
	unlink(panicfile);		/* nuke our predecessor */

    /* dump panic file */
    if((f = fopen(panicfile, "w")) == NULL) {
	perror("CANNOT OPEN PANIC FILE, YOU LOSE:");
	_exit(135);
    } else {
	fprintf(logfile, "DUMPING: %s\r\n", panicfile);
	db_write(f);
	fclose(f);
	fprintf(logfile, "DUMPING: %s (done)\r\n", panicfile);
	_exit(136);
    }
}

void dump_database()
{
    struct descriptor_data *d, *dnext;
    int saved = euid;

    euid = NOTHING;

    epoch++;

    fprintf(logfile, "%s DUMPING: %s.%03d\r\n", actime( &timenow), dumpfile, epoch);

    dump_database_internal();
    fprintf(logfile, "%s DUMPING: %s.%03d (done)\r\n", actime( &timenow), dumpfile, epoch);
    euid = saved;
}

static void fork_and_dump()
{
    int child,saved = euid;
    struct descriptor_data *d, *dnext;

    epoch++;
#ifdef MSDOS
	inform_all("\r\nStarting DUMP, please wait...\r\n");
#endif
    fprintf(logfile, "CHECKPOINTING: %s.%03d\r\n", dumpfile, epoch);
    child = 0;
    euid = NOTHING;
    d = dnext = NULL;

#ifndef MSDOS
#ifdef USE_VFORK
    child = vfork();
#else /* USE_VFORK */
    child = fork();
#endif /* USE_VFORK */
    if(child == 0) {
	/* in the child */
	close(0);		/* get that file descriptor back */

	alarm_block = 1;	/* No point in checkpointing here! */

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if(d->descriptor >= 0){
    		fprintf(logfile,"close player descriptor%d\n",d->descriptor);
		if( d->player >0 && d->player < db_top){
			Flags(d->player) &= ~(ROBOT|WEAPON);
		}
		close (d->descriptor);
		d->descriptor = -1;
	}
    }
#endif /* !MSDOS */
	dump_database_internal();

#ifndef MSDOS
	_exit(0);		/* !!! */
    } else if(child < 0) {
	perror("fork_and_dump: fork()");
    }
#else
	inform_all("\r\n... Dump done, please carry on!\r\n");
#endif

    /* in the parent */

    if( saved_dumpfile)
	dumpfile = saved_dumpfile;
    extracting = 0;	/* No longer extracting */

    /* reset alarm */
    alarm_triggered = 0;
    alarm(0);
    alarm(DUMP_INTERVAL);
    euid = saved;
}

static int reaper()
{
#ifndef MSDOS
    union wait stat;

   signal(SIGCHLD, reaper);
    while(wait3(&stat, WNOHANG, 0) > 0){
    };
#endif
    return 0;
}

int init_game(infil, outfil)
const char *infil, *outfil;
{
   FILE *f;

   if((f = fopen(infil, "r")) == NULL){
	fprintf(logfile,"Could not open %s\r\n", infil);
	return -1;
    }
   
   /* ok, read it in */
   fprintf(logfile, "LOADING: %s\n", infil);
   if(db_read(f) < 0){
	fprintf(logfile, "db_read failed!\n");
	return -1;
   }
   fprintf(logfile, "LOADING: %s (done)\n", infil);

   /* everything ok */
   fclose(f);

   /* initialize random number generator */
#ifndef MSDOS
   srandom(getpid());
#else
   {	int x;
		long l;
		l = time(NULL);
		x = l;
		srand(x);
	}
#endif

   /* set up dumper */
   if(dumpfile) free((void *) dumpfile);
   dumpfile = alloc_string(outfil);
#ifndef MSDOS
   signal(SIGALRM, alarm_handler);
   signal(SIGHUP, alarm_handler);
   signal(SIGCHLD, reaper);
#endif
   alarm_triggered = 0;
   alarm(DUMP_INTERVAL);
   
   return 0;
}

void do_version(player)
dbref player;
{
	sprintf(msgbuf,"PRISM Version %s", version_buf);
	notify(player, msgbuf);
}

struct command_table {
const char	*name;
int		nargs,minmatch;
const void	(*func)();
const char	*syntax;
};

static int ncom = 0;

void do_broad(),do_chown(),do_cost(),do_create(),do_destroy();
void do_describe(),do_dig(),do_dump(),print_fights();
void do_find();
#ifdef NOTDEF
void do_force();
#endif
void do_link(),do_relink(),do_look_com(),do_lock();
void do_merge(),do_name(),do_newpassword(),do_open();
void do_password(),do_quit(),do_noquit_scan();
void do_set(),do_shutdown(),do_stats();
void list_commands(),do_time(),do_toad(),do_unlink(),do_unload();
void do_examine();
void do_gripe(),do_help(),do_inventory(),do_score();
void do_look_at(),do_news();
void print_player_levels(), do_local_broad();
void at_robot(), at_zone(), do_debug();
void do_class(),do_reset(), do_default(), list_bool_funcs();
void print_flags(),print_args(),do_extract(),at_which(),do_ten();
#ifdef MSDOS
#ifndef _WINDOWS
void do_edit();
#endif
#endif
#ifdef BSX
void do_draw();
#endif

#ifdef VM_DB
void print_mru();
#endif

struct command_table commands[] =
{
{ "@args",	0, 4, print_args,	"@args                .           . Print the argument table"},
{ "@boolfuncs", 1, 5, list_bool_funcs,	"@boolfuncs page_no   .           . List lock functions"},
{ "@broadcast", 2, 3, do_broad, 	"@broadcast msg       . <Arch>    . Broadcast 'msg'"},
{ "@chown",	2, 3, do_chown, 	"@chown obj=new_owner . <Arch>    . Change Ownership of obj"},
{ "@class",	2, 3, do_class, 	"@class obj=class_room. <Arch>    . Change Ownership of obj"},
{ "@commands",	1, 4, list_commands,	"@commands page_no                . List this info"},
{ "@cost",	2, 5, do_cost,  	"@cost obj=new_value  . <Arch>    . Change the pennies value of an object"},
{ "@create",	2, 3, do_create,	"@create obj=value    . <builder> . Create a Thing Object with name 'obj'"},
{ "@debug",	1, 6, do_debug,		"@debug number        . <god>     . Used to set debug flag"},
{ "@default",	2, 4, do_default,	"@default type=class  . <god>     . Used to set default classes"},
{ "@describe",	2, 5, do_describe,	"@describe obj=desc   . <builder> . Set the desciption of obj to 'desc'"},
{ "@destroy",	1, 8, do_destroy,	"@destroy obj         . <builder> . Destroy obj, you must own it."},
{ "@dig",	2, 4, do_dig,		"@dig name [=exit]    . <builder> . Create a new room"},
#ifdef BSX
{ "@draw",	0, 5, do_draw,		"@draw                . Invoke the BSX draw program"},
#endif
{ "@dump",	1, 5, do_dump,		"@dump                . <god>     . Does a checkpoint dump"},
#ifdef MSDOS
#ifndef _WINDOWS
{ "@edit",	1, 5, do_edit,		"@edit obj            . <Arch>    . Edit object"},
#endif
#endif
{ "@examine",	1, 3, do_examine,	"@examine obj         . <builder> . Get detailed look of object you own"},
{ "@extract",	2, 8, do_extract,	"@extract arg1=arg2   . <god>     . Extract part of the database"},
{ "@find",	1, 5, do_find,		"@find [/exit|/#obj] name         . Find objects with 'name' in their title"},
{ "@flags",	0, 4, print_flags,	"@flags               .           . Print the flag values"},
#ifdef NOTDEF
{ "@force",	2, 6, do_force,		"@force player=command. <wiz>     . Force player to execute 'command'"},
#endif
{ "@gripe",	2, 2, do_gripe, 	"@gripe msg                       . Complain"},
{ "@help",	0, 4, do_help,  	"@help                            . Print the help file"},
{ "@inventory",	0, 2, do_inventory,	"@inventory           . <wiz>     . See your contents"},
{ "@lbroadcast",2, 4, do_local_broad,	"@lbroadcast msg      . <wiz>     . Broadcast 'msg' within ZONE"},
{ "@levels",	1,4,print_player_levels,"@levels                          . Print the levels info"},
{ "@link",	2, 3, do_link,		"@link obj = loc      . <builder> . Link exit 'obj' to room 'loc'"},
{ "@lock",	2, 5, do_lock,		"@lock obj = expr     . <builder> . Lock the obj with the lock 'expr'"},
{ "@look",	2, 5, do_look_com,	"@look obj            . <wiz>     . Look at obj"},
{ "@merge",	1, 6, do_merge, 	"@merge file          . <god>     . Upload another TinyDatabase"},
{ "@name",	2, 5, do_name,  	"@name old = new                  . Change the name of obj 'old to 'new'"},
{ "@newpassword",2,12,do_newpassword,	"@newpassword player=password. <wiz>"},
{ "@news", 	0, 4, do_news,  	"@news                            . Print the news file"},
{ "@open",	2, 5, do_open,		"@open [/#obj] exit[= #room]  . <builder> . Open exit [command on obj] to room"},
{ "@password",  2, 9, do_password,	"@password old_pass=new_pass      . Change your password"},
{ "@quit",	0, 5, do_quit,		"@quit                            . Leave game."},
{ "@relink",	2, 5, do_relink,	"@relink exit=#room   . <builder> . Relink an exit/command to room"},
{ "@reset",	0, 6, do_reset, 	"@reset               . <wiz>     . Perform a reset, send .reset # to all"},
{ "@robots",	2, 5, at_robot,		"@robot name=stats    . <wiz>     . Control robots"},
{ "@score",	0, 3, do_score,		"@score                           . Tell you your score"},
{ "@set",	2, 4, do_set,		"@set obj = [!]flag   . <builder> . Set/Reset flags on obj"},
{ "@shutdown",	0, 9, do_shutdown,	"@shutdown            . <god>     . Save the database and shutdown"},
{ "@stats",	1, 6, do_stats,		"@stats	                          . Print various statistics"},
{ "@time",	2, 5, do_time,		"@time obj[=delta]    . <wiz>     . Change the timestamp of object"},
{ "@toad",	1, 5, do_toad,		"@toad player         . <Arch>    . Turn a player into toad (Non reversable)"},
{ "@top",	0, 4, do_ten,		"@top                 .           . Print the top ten."},
{ "@unlink",	1, 7, do_unlink,	"@unlink obj          . <builder> . Unlink an exit"},
{ "@unlock",	1, 7, do_unlock,	"@unlock obj          . <builder> . Remove the lock on object"},
{ "@version",   0, 4, do_version,	"@version             .           . Print version number"},
{ "@which",	2, 6, at_which,		"@which command       . <builder> . Print the exits that match command"},
{ "@zone",	2, 5, at_zone,		"@zone                .           . Print the zone you are in"},
{ "", 0, 0, NULL, ""}
};


void list_commands(player, arg1)
dbref player;
const char *arg1;
{	int n = atoi(arg1);
	int i, j;
	char *p;

	if( n < 0 || 16*n >= ncom){
		notify(player, "That page does not exist!");
		return;
	}

	sprintf(msgbuf,"Command list Page %d", n);
	notify(player, msgbuf);

	for(i=0; i < 16 && i+16*n < ncom; i++){
		sprintf(msgbuf, "%s", commands[i+16*n].syntax);
		p = msgbuf;
		j = commands[i+16*n].minmatch;
		while(*p){
			if( j ){ 
				if( *p >= 'a' && *p <= 'z')
					*p -= 0x20;
				j--;
			}
			if(*p == ' '){
				p++;
				while(*p == ' ')*p++ = '.';
			}else p++;
		}
		notify(player, msgbuf);
	}
	
}


struct command_table *find_com(command)
char *command;
{	struct command_table *t;
	int i, j, k, n;
	char *p, ch, *com;

	if( !ncom){
		t = commands;
		for(ncom=0; t->minmatch; t++)ncom++;
	}

	for(j = 1; j <= ncom; j = j+j);

	j = j/2;
	i = j-1;

	while(j){	/* some to try */
		j = j/2;
		t = &commands[i];
		p = (char *)t->name;
		com = command;
		n = t->minmatch;

		while( *com ){
			ch = *com;
			if(ch >= 'A' && ch <= 'Z')ch += 0x20;
			if( ch == *p){
				if(n )n--;
				com++;
				p++;
				if( !*com && n){	/* Try another before */
					i = i-j;
					break;
				}
			}else if(ch < *p){
				i = i-j;
				break;
			}else {
				i = i+j;
				while( i >= ncom && j){
					i = i-j;
					j = j/2;
					i = i+j;
				}
				break;
			}
		}
		if( !*com && !n)return t;	/* matched */
	}
	return NULL;
}

static char *p_str(s)
char *s;
{
	if( !s)
		return "Not set";
	return s;
}



void process_command( player, command)
dbref player;
char *command;
{
    char *arg1;
    char *arg2;
    char *q;			/* utility */
    char *p;			/* utility */
    int  saved_alarm_block = alarm_block;
    struct command_table *com;
    char *index( PROTO2(char *, char));
    dbref exit;

    if(command == 0) {
	fprintf(logfile,"Process_command: Command == NULL\n");
	abort();
    }

    /* robustify player */
    if(player < 0 || player >= db_top || Typeof(player) != TYPE_PLAYER) {
	fprintf(logfile, "process_command: bad player %d, db_top= %d\n", player, db_top);
	return;
    }

	if(!Lastcommand){
		Lastcommand = (char *)malloc(1024);
		strcpy(Lastcommand, "First command");
	}

    sprintf(Lastcommand, "COMMAND from %s(%d) in %s(%d): %s\n",
	    Name(player), player,
	    Name(Location(player)), Location(player),
	    command);
#ifdef LOG_COMMANDS
fprintf(logfile,"%s", Lastcommand);
#endif

    /* eat leading whitespace */
    while(*command && isspace(*command)) command++;

    /* eat extra white space */
    q = p = command;
    while(*p) {
	/* scan over word */
	while(*p && !isspace(*p)) *q++ = *p++;
	/* smash spaces */
	while(*p && isspace(*++p));
	if(*p) *q++ = ' '; /* add a space to separate next word */
    }
    /* terminate */
    *q = '\0';

    /* block dump to prevent db inconsistencies from showing up */
    saved_alarm_block = alarm_block;	/* process_commands can be recursive*/
    alarm_block = 1;
    arg1 = arg2 = NULL;

    if(can_move(player, command)) {
		/* command is an exact match for an exit */
		exit = last_match_result();
		if(exit < 0){
			arg1 ="";
			arg2 ="";
fprintf(logfile,".huh %s %s %s\n", command, arg1, arg2);
			goto bad;
		}
		do_move_sub(player, exit, command);
    } else {
	/* parse arguments */

	/* find arg1 */
	/* move over command word */
	for(arg1 = command; *arg1 && !isspace(*arg1); arg1++);
	/* truncate command */
	if(*arg1) *arg1++ = '\0';

	/* move over spaces */
	while(*arg1 && isspace(*arg1)) arg1++;

	/* find end of arg1, start of arg2 */
	for(arg2 = arg1; *arg2 && *arg2 != ARG_DELIMITER; arg2++);

	/* truncate arg1 */
	for(p = arg2 - 1; p >= arg1 && isspace(*p); p--) *p = '\0';

	/* go past delimiter if present */
	if(*arg2) *arg2++ = '\0';
	while(*arg2 && isspace(*arg2)) arg2++;


	com = find_com(command);

	if( com){
		switch(com->nargs){
		case 0:
			(*com->func)(player);
			break;
		case 1: 
			(*com->func)(player, arg1);
			break;
		case 2:
			(*com->func)(player, arg1, arg2);
			break;
		default:
fprintf(logfile,"com has nargs=%d, command=%s\n", com->nargs, command);
			break;
		}
	}else{
bad:
		last_result = 0;

		if( !quiet){
			char *buf1 = (char *)malloc(256);

			if(buf1){
				sprintf(buf1, ".huh [%s] %.50s %.150s", Name(player), command, reconstruct_message(arg1, arg2));
				send_message(player, Location(player), buf1, wizauth());
				free( (FREE_TYPE) buf1);
			}

#ifdef LOG_FAILED_COMMANDS
		    {
			fprintf(logfile, "HUH from %s(%d) in %s(%d)[%s]: %s %s\n",
				Name(player), player,
				Name(Location(player)),
				Location(player),
				Name(Owner(Location(player))),
				command,
				reconstruct_message(arg1, arg2));
		    }
#endif /* LOG_FAILED_COMMANDS */
		}
	}
    }

    /* unblock alarms */
ok: alarm_block = saved_alarm_block;
    if(alarm_triggered) {
	fork_and_dump();
    }
}
