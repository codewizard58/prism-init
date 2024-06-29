#include "copyrite.h"

#include <stdio.h>

#ifndef MSDOS
#include <sys/types.h>

#ifdef sun
#include <sys/time.h>
#endif
#ifdef linux
#include <sys/time.h>
#include <unistd.h>
#endif

#endif



#ifdef MSDOS

#include "defs.h"

#include "net.h"


#else
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>
#endif

void sorry_user(), sorry2_user();

#include "mud_set.h"

#include <ctype.h>

#ifndef MSDOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


#include "db.h"
#include "interfac.h"
#include "config.h"

#include "os.h"
#include "bsx.h"

#ifdef OUTPUT_WRAP
#define OUTPUT_WIDTH 76
#endif

#define INPUT_EDIT

extern char *zone_name(PROTO(dbref));
extern char *printsince(PROTO(dbref));
extern char *printsince_sub(PROTO(long));

#ifdef MSDOS
struct timezone {
char * zone;
};

#endif

#ifdef ATARI
unsigned long _STACK = 32767;
#endif

TIME timenow;
int  ticks=0, maxticks=0;

/* options */
int reset_money = 0;
int no_robots = 0;

extern int xargc;
extern char **xargv;
extern dbref euid;              /* At top level euid always == NOTHING */
extern long send_msg_count;
extern char version_buf[];

int depth = 0;

/* END OF HEADER */
#ifndef MSC
#ifndef MSDOS
extern int      errno;
#endif
#endif
int     shutdown_flag = 0;

static const char *connect_fail = "Either that player does not exist, or has a different password.\r\n";
static const char *create_fail = "Either there is already a player with that name, or that name is illegal.\r\n";
static const char *flushed_message = "<Output Flushed>\r\n";
static const char *shutdown_message = "Going down - Bye\r\n";

#include "descript.h"

struct descriptor_data *descriptor_list = 0;

int sock;
int httpsock = INVALID_SOCKET;
int ndescriptors = 0;
dbref cur_player;
struct descriptor_data *cur_desc;

#ifdef EXTRACT
extern int extracting;
extern dbref *extract_list;
#endif

void process_commands(PROTO(void));
int shove_sub();
void shovechars();
void shutdownsock(PROTO(struct descriptor_data *));
struct descriptor_data *initializesock(PROTO(int) );
void make_nonblocking(PROTO(int ));
void freeqs( PROTO(struct descriptor_data *));
void welcome_user(PROTO(struct descriptor_data *));
void check_connect(PROTO2(struct descriptor_data *, const char *));
void close_sockets();
const char *addrout (PROTO(long));
void dump_users(PROTO(struct descriptor_data *));
void set_signals(PROTO(void));
struct descriptor_data *new_connection(PROTO(int) );
void parse_connect (PROTO5(const char *, char *, char *, char *, struct descriptor_data *));
void set_userstring (PROTO2(char **, const char *));
int do_command (PROTO2(struct descriptor_data *, char *));
char *strsave (PROTO(const char *));
int make_socket(PROTO(int));
int queue_string(PROTO2(struct descriptor_data *, const char *));
int queue_write(PROTO3(struct descriptor_data *, const char *, int));
int process_output(PROTO(struct descriptor_data *));
int process_input(PROTO(struct descriptor_data *));
#ifndef MSDOS
void bailout(PROTO3(int, int, struct sigcontext *));
#endif

extern int quiet;               /* turn off notifies */
FILE *logfile;

TIME last_alarm = 0l;
TIME timelast = 0l;
int avail_descriptors;
int maxd;
mud_set input_set, output_set;
long now;

/* pjc 12/12/93 */
#ifdef NOCOM1
int     nocom1 = 1;
#else
int     nocom1 = 0;
#endif
int notcp;

#ifdef MSDOS
#ifndef _WINDOWS
struct timeval {
long    tv_sec,tv_usec;
};
#endif
#endif

struct timeval last_slice, curnt_time;
struct timeval next_slice;
struct timeval timeout, slice_timeout;
struct descriptor_data *d, *dnext;
struct descriptor_data *newd;
int work;

#ifdef _WINDOWS
#define strncasecmp strnicmp
#endif


/* Networking stuff */
int tcp_port = 4201;

/* from text.c */

void replace_text(PROTO5(struct text_queue *, struct text_block *, const char *, int, int));
/* */

#ifndef EINTR
#define EINTR (-1)
#endif

#ifndef EMFILE
#define EMFILE (-1)
#endif

char home_dir[257];
extern char media_dir[];

/* Routine to return a stripped time string. */

char *actime( t )
TIME *t;
{       static char buf[32], *p;
	long atime = *t;

#ifdef MSDOS
	atime = atime + 0x83abcd48l;
#endif

	strcpy(buf, ctime(&atime));
	p = buf;
	while(*p && *p != '\n')p++;
	*p = '\0';
	return buf;
}


char *get_name_from_fd(fd)
int fd;
{       struct descriptor_data *d= descriptor_list;
	static char buf[128];

	while(d && d->descriptor != fd)d = d->next;

	if( d){
		if( d->connected == 2 && d->player != NOTHING){
			strcpy(buf, Name(d->player));
			return buf;
		}
	}
	return NULL;
}


char *msgbuf;

int main_sub(argc, argv)
int argc;
char **argv;
{
    int port;

    timenow = time(NULL);
#ifdef MSDOS
    timenow = timenow - 0x83abcd48l;
#endif


    port = argc >= 4 ? atoi (argv[3]) : TINYPORT;
    tcp_port = port;

    msgbuf = malloc( 4096);
    if( !msgbuf){
	no_memory("main\n");
	return 0;
    }

#ifdef VM_DESC
  init_vm();
#endif
	netdevinit();

    init_translate_table();     /* Faster way of DOWNCASEing */
    return 1;
}

int main_sub2(argc, argv)
char **argv;
int argc;
{
    if (init_game (argv[1], argv[2]) < 0) {
	fprintf (logfile, "Couldn't load %s!\n", argv[1]);
	return 0;
    }
    return 1;
}

int main_sub3()
{

    do_noquit_scan();

    set_signals ();
    /* go do it */
    gettimeofday(&last_slice, (struct timezone *) 0);

    avail_descriptors = getdtablesize() - 4;

    init_robots();
    
    time(&timenow);
#ifdef MSDOS
    timenow = timenow - 0x83abcd48l;
#endif

    last_alarm = timenow;
    timelast= timenow;
    return 1;

}

#ifndef _WINDOWS

void main(argc, argv)
int argc;
char **argv;
{
	logfile = stderr;

    if (argc < 3) {
	fprintf (logfile, "Usage:  %s infile dumpfile [port] [-RESET -ROBOTS]\n", *argv);
	exit (1);
    }

	while( argc > 1 && (*argv[1] == '-' 
#ifdef MSDOS
		|| *argv[1] == '/'
#endif
		) ){
		argc--;
		argv++;
		if( !strcmp(&argv[0][1], "nocom1")){
			nocom1 = 1;
		}else if( !strcmp(&argv[0][1], "notcp")){
			notcp = 1;
		}else if( !strcmp(&argv[0][1], "mediadb")){
			argc--;
			argv++;
			strcpy(media_dir, argv[0]);
		}else {
			switch(argv[0][1]){
			case 'l':
				logfile = fopen( &argv[1][2], "a");
				continue;
			default:
				fprintf(logfile,"Unknown option %s\n", argv[0]);
				exit(1);
			}
		}
	}


    if( argc >= 5){
	if(!strcmp(argv[4], "-RESET") ){
		reset_money = 1;
	}else if( !strcmp(argv[4], "-ROBOTS") ){
		no_robots = 1;
#ifdef EXTRACT
	}else if( !strcmp(argv[4], "-EXTRACT") ){
		extracting = 1;
#endif
	}else{
fprintf(logfile,"Unknown option %s\n", argv[4]);
		exit(3);
	}
    }

    if(main_sub(argc, argv) && 
       main_sub2(argc, argv) &&
	main_sub3()){
	shovechars ();
    }

    netdevshut();
    dump_database ();

    exit (0);
}

#endif

#ifndef MSDOS
void waitwaitwait()
{	int x;

	while( waitpid(0, &x, WNOHANG) > 0);
	signal(SIGCHLD, waitwaitwait);
}
#endif

void set_signals()
{
#ifndef MSDOS
    void bailout (), dump_status ();

    /* we don't care about SIGPIPE, we notice it in select() and write() */
    signal (SIGPIPE, SIG_IGN);

    /* standard termination signals */
    signal (SIGINT, bailout);
    signal (SIGTERM, bailout);

    /* catch these because we might as well */
    signal (SIGQUIT, bailout);
    signal (SIGILL, bailout);
    signal (SIGTRAP, bailout);
    signal (SIGIOT, bailout);
    signal (SIGFPE, bailout);
    signal (SIGBUS, bailout);
    signal (SIGSEGV, bailout);
#ifndef linux
    signal (SIGEMT, bailout);
    signal (SIGSYS, bailout);
#endif
    signal (SIGTERM, bailout);
#ifndef HPUX
    signal (SIGXCPU, bailout);
    signal (SIGXFSZ, bailout);
#endif
    signal (SIGVTALRM, bailout);
    signal (SIGUSR2, bailout);

    /* status dumper (predates "WHO" command) */
    signal (SIGUSR1, dump_status);

    signal(SIGCHLD, waitwaitwait);
#endif
}


struct timeval timeval_sub( now, then)
struct timeval now;
struct timeval then;
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if (now.tv_usec < 0) {
	now.tv_usec += 1000000;
	now.tv_sec--;
    }
    return now;
}

int msec_diff( now, then)
struct timeval now, then;
{
	int dif;

    dif = ((now.tv_sec - then.tv_sec) * 1000
	    + (now.tv_usec - then.tv_usec) / 1000);
	return dif;
}

struct timeval msec_add( t, x)
struct timeval t;
int x;
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;
    if (t.tv_usec >= 1000000) {
	t.tv_sec += t.tv_usec / 1000000;
	t.tv_usec = t.tv_usec % 1000000;
    }
    return t;
}

struct timeval update_quotas( last, curnt)
struct timeval last, curnt;
{
    int nslices;
    struct descriptor_data *d;

    nslices = msec_diff (curnt, last) / COMMAND_TIME_MSEC;
    if (nslices > 0) {
	for (d = descriptor_list; d; d = d -> next) {
	    d -> quota += COMMANDS_PER_TIME * nslices;
	    if (d -> quota > COMMAND_BURST_SIZE)
		d -> quota = COMMAND_BURST_SIZE;
	}
    }
    return msec_add (last, nslices * COMMAND_TIME_MSEC);
}

long fighttime = 0l;

/* List of machines that I don't want to accept calls from */
char *bad_hosts[] =
{       (char *)"dir.ic.ac.uk",
	(char *)"dir.ic",
	NULL
};

int bad_host(h)
char *h;
{       char **p;
	p = bad_hosts;
	while(h && *p){
		if( !strcmp(h, *p)){
			return 1;       /* don't allow */
		}
		p++;
	}
	return 0;       /* Not in list */
}

/* This is the MAIN loop */
int shove_sub()
{
	int work= 0;

	(void) time(&now);
#ifdef MSDOS
	now = now - 0x83abcd48l;
#endif
	timenow = now;

#ifndef MSDOS
	gettimeofday(&curnt_time, (struct timezone *) 0);
	last_slice = update_quotas (last_slice, curnt_time);
#endif

	ticks++;
	depth = 0;      /* Used to stop too deep recursion in locks */

	if(timenow - timelast > 1l
#ifndef MSDOS
		 && ticks >= maxticks
#endif
			){
		if(ticks > maxticks)
			maxticks = ticks;
		ticks = 0;

		timelast = timenow;

#ifdef ROBOT
		do_robots();
#endif
	}

	process_commands();

	if (shutdown_flag)
	    return 0;
	timeout.tv_sec = 1000l;
	timeout.tv_usec = 0;

/* If someone is logged in move robots at normal speed */
	for (d = descriptor_list; d; d = d->next) {
		if( d->descriptor >= 0){
			timeout.tv_sec = 1l;
			break;
		}
	}
	next_slice = msec_add (last_slice, COMMAND_TIME_MSEC);
	slice_timeout = timeval_sub (next_slice, curnt_time);
	MUD_ZERO (&input_set);
	MUD_ZERO (&output_set);

	if (ndescriptors < avail_descriptors){
	    MUD_SET (0, &input_set);
	}

	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0){
		    if (d->input.head)
			timeout = slice_timeout;
		    else
			MUD_SET (d->descriptor, &input_set);
		    if (d->output.head)
			MUD_SET (d->descriptor, &output_set);
		}
	}
    if( httpsock != INVALID_SOCKET){
    	MUD_SET( httpsock, &input_set);
		http_setfd(&input_set, &output_set);
    }

	if (mud_select (maxd, &input_set, &output_set,
		    (mud_set *) 0, &timeout) < 0) {
	    if (errno != EINTR) {
		perror ("select");
		return 0;
	    }
	} else {
#ifdef MSDOS
		if( now - last_alarm < 0){ /* bug */
			last_alarm = now;
		}else if(now - last_alarm > 1800){
			alarm_handler();
			last_alarm = now;
		}
#endif

	    if (MUD_ISSET (0, &input_set)) {
			if (!(newd = new_connection (sock))) {
			    if (errno != EINTR && errno != EMFILE) {
					perror ("new_connection");
					return 0;
			    }
			} else {
				if (newd->descriptor >= maxd)
					maxd = newd->descriptor + 1;
				if(newd->descriptor >= MAX_USERS){
					sorry_user(newd);
				}
				if(bad_host(newd->host)){
fprintf(logfile,"Had an attempt to login from %s\n", newd->host);
					sorry2_user(newd);
				}
			}
	    }
	    if( httpsock != INVALID_SOCKET && MUD_ISSET(httpsock, &input_set)){
	    	new_http(httpsock);
	    }

	    for (d = descriptor_list; d; d = dnext) {
			dnext = d->next;

			if (d->descriptor < 0 || /* Robots: throw output */
				MUD_ISSET (d->descriptor, &output_set)
			) {
				if (!process_output (d)) {
					shutdownsock (d);
					continue;
				}
			}

			if (d->descriptor >= 0 && 
			    d->connected != 3  &&       /* shutting down */
				MUD_ISSET (d->descriptor, &input_set)
			) {
				d->last_time = now;
				if (!process_input (d)) {
					shutdownsock (d);
					continue;
				}
			}

			if( d->connected == 3){
				shutdownsock(d);
				break;
			}
	    }
	    if( httpsock != INVALID_SOCKET){
	    	do_http(&input_set, &output_set);
	    }
	}
	return work;
}


/* non windows main loop that runs the game*/

void shovechars()
{
	if( home_dir[0] == '\0'){
		getcwd(home_dir, 255);
	}
     while (shutdown_flag == 0) {
		shove_sub();
    }
}


void prompt_user(d)
struct descriptor_data *d;
{
	queue_string(d, "\r\nEnter one of the following commands,\r\n");
	queue_string(d, "\r\nconnect user password (to connect to an existing username)\r\ncreate user password, (create a new username,\r\n");
	queue_string(d, "                      password must be atleast 6 characters long)\r\n");
	queue_string(d, "WHO (list currently logged on users,note must be in CAPITALS)\r\nQUIT (Disconnect without logging in. note must be in CAPITALS)\r\nPRISM: ");
}

void welcome_user(d)
struct descriptor_data *d;
{
	sprintf(msgbuf,"Welcome to PRISM %s (Petes Reduced Instruction Set MUD)\r\n\r\nWarning! playing this game can be addictive. You play at your own risk!", 
		version_buf);
	queue_string(d, msgbuf);
#ifdef MSDOS
	queue_string (d, "MSDOS Port by P.J.Churchyard\r\n");
#endif
	prompt_user(d);
}


void sorry_user(d)
struct descriptor_data *d;
{
    queue_string (d, WELCOME_MESSAGE);
	queue_string (d, "\r\nSorry but I have had to limit the number of connections\n");
	queue_string (d, "So please try again later. I can be reached by email\r\n");
	queue_string( d, "at pjc@tis.com\r\n");
	queue_string( d, "I hope to get back to full operation shortly\r\n");
	queue_string( d,"Pete. 4/17/95\r\n");
}

void sorry2_user(d)
struct descriptor_data *d;
{
	queue_string( d, "  \r\n");
	queue_string( d, "  \r\n");
	queue_string( d, "Sorry but access via this route is not permitted.\r\n");
	queue_string( d, "If you have problems, mail pjc@tis.com\r\n");
}


void sorry3_user(d)
struct descriptor_data *d;
{       char *h;

	h = d->host;
	if(!h)
		h = "Unknown";

	sprintf(msgbuf, "Your access from %s has been logged!!", h);
	queue_string( d, msgbuf);
fprintf(logfile,"HACK_HOST:%s %s %s(%d)\n", actime(&timenow), h, Name(d->player), d->player);
	d->connected = 3;
}


void goodbye_user(d)
struct descriptor_data *d;
{
    if( d->caps & CAP_BSX){
	mud_write(d->descriptor, "@TMS\r\n\r\n", 8);
    }
    mud_write (d->descriptor, LEAVE_MESSAGE, strlen (LEAVE_MESSAGE));
}

char *strsave (s)
const char *s;
{
    char *p;
    int cnt = strlen(s) + 1;

    MALLOC (p, char, cnt);

    if (p)
	strcpy (p, s);
    return p;
}

/* 4/10/91 added ^[ type processing to allow control chars
 * to be put into prefix strings 
 */

void set_userstring (userstring,command)
char **userstring;
const char *command;
{       char *p1,*p2;

    if (*userstring) {
	FREE (*userstring);
	*userstring = NULL;
    }
    while (*command && isascii (*command) && isspace (*command))
	command++;
    if (*command){
	*userstring = strsave (command);

/* post process the copy. can only make it shorter... */
	p1 = p2 = *userstring;
	while(*p1){
		*p2 = *p1;
		if( *p1 == '^'){
			p1++;
			*p2 = *p1 & 0x1f;
		}else if( *p1 == '\\'){
			p1++;
			*p2 = *p1;
		}
		p2++;
		if( *p1)
			p1++;
	}
	*p2 = '\0';
    }
}

void process_commands()
{
    int nprocessed;
    struct descriptor_data *d, *dnext;
    struct text_block *t;

    do {
	depth = 0;

	nprocessed = 0;
	for (d = descriptor_list; d; d = dnext) {
	    dnext = d->next;
	    if (
#ifndef MSDOS
		d -> quota > 0 && 
#endif
			(t = d -> input.head)) {
			work = 1;
			d->quota--;
			nprocessed++;
			cur_desc = d;
/* unlink the block we are going to throw */
			d->input.head = t -> nxt;
			if (!d -> input.head)
				d -> input.tail = &d -> input.head;

			if (!do_command (d, t -> start)) {
				shutdownsock (d);
				break;
			} else {
				free_text_block (t);
			}
	    }
	}
    } while (nprocessed > 0);
}

char *mediatab[] = {"BSX","HIT",NULL};

int do_command (d,command)
struct descriptor_data *d;
char *command;
{   char *p;
	int n;
	dbref loc;
	char buf[100];

/* Always allow QUIT and WHO */

	if( command[0] == '@'){ 	/* Look for a multi-media flag command */
		n=0;
		while(mediatab[n] != NULL){
			if( !strncasecmp(&command[1], mediatab[n], strlen(mediatab[n]))){
				d->caps |= 1 << n;
				command += strlen(mediatab[n])+1;
				while(*command == ' ')command++;
				while(*command){
					n = 0;
					while(mediatab[n] != NULL){
						if( !strncasecmp(command, mediatab[n], strlen(mediatab[n]))){
							d->caps |= 1 << n;
							command += strlen(mediatab[n]);
							while(*command == ' ')command++;
						}else{
							n++;
						}
					}
					if( mediatab[n] == NULL)
						break;	/* rest of line is ignored */
				}
fprintf(logfile," cap set to %04x\n", d->caps);
				return 1;
			}
			n++;
		}
	}
	if (!strcmp (command, QUIT_COMMAND)) {

			if(d->connected == 2){
			    if( (loc = getloc(d->player)) != NOTHING){
				sprintf(buf, "[%s] %s turns to stone.",Name(d->player), Name(d->player));
				send_message(d->player, loc_of(d->player), buf, 1);
			    }
			    fprintf(logfile,"QUIT %s(%d)\n", Name(d->player), d->player);
			}

			Age(d->player) = timenow;

			goodbye_user (d);
			return 0;
	} else if (!strcmp (command, WHO_COMMAND)) {
			dump_users (d);
			if( d->connected == 0)
				prompt_user(d);
#ifdef X29_MVAX
	} else if ( command[0] == 'A' && command[1] == 'D' && command[2] == 'D' && command[3] == 'R'){
		if( !strcmp(d->host, "mvax") || !strcmp(d->host, "mvax.cc.ic.ac.uk") ){
			free(d->host);
			d->host = (char *)alloc_string( &command[5]);
		}else {
fprintf(logfile,"%s, host=%s\n", command, d->host);
		}
#endif
	}else  /* everything else only if connected */

	   if( d->connected == 2){
		if (!strncmp (command, PREFIX_COMMAND, strlen (PREFIX_COMMAND))) {
			set_userstring (&d->output_prefix, command+strlen(PREFIX_COMMAND));
		} else if (!strncmp (command, SUFFIX_COMMAND, strlen (SUFFIX_COMMAND))) {
			set_userstring (&d->output_suffix, command+strlen(SUFFIX_COMMAND));
		} else {
			if (d->output_prefix) {
				queue_string (d, d->output_prefix);
				queue_write (d, "\r\n", 2);
			}
/* Do some top level initialization */
			euid = NOTHING;
			quiet= 0;
			xargc= 0;
			xargv[0] = NULL;
			process_command (d->player, command);

			if (d->output_suffix) {
				queue_string (d, d->output_suffix);
				queue_write (d, "\r\n", 2);
			}
		}
	} else {
	    check_connect (d, command);
	}
    return 1;
}


void check_connect (d, msg)
struct descriptor_data *d;
const char *msg;
{
    char command[MAX_COMMAND_LEN];
    char user[MAX_COMMAND_LEN];
    char password[MAX_COMMAND_LEN];
    dbref player;
    static char buf[256];
	dbref auth = 1;

	if( default_wizard > 0)
		auth = default_wizard;

    parse_connect (msg, command, user, password, d);

    if (d->connected == 0 ){
	if( !strncmp (command, "co", 2)) {
		player = connect_player (user, password, d);
		if (player == NOTHING || !*password) {
			queue_string (d, connect_fail);
			fprintf(logfile,"Connect fail: '%s' '%s' on %d\n", user, password, d->descriptor);
		} else {
			d->connected = 2;
			d->player = player;
			fprintf (logfile, "\n%s CONNECTED %s(%d) on descriptor %d from %s\n",
				     actime(&timenow), Name(player), player, d->descriptor, d->host);
		}
	} else if (d->connected == 0 && !strncmp (command, "cr", 2)) {
		if( password && strlen(password) > 5)
			player = create_player (user, password, d);
		else
			player = NOTHING;

		if (player == NOTHING || !*password) {
			queue_string (d, "\r\nEither that player exists, or you have invalid characters in the name\r\nor you didn't specify a password of atleast 6 characters\r\n");
			fprintf(logfile,"Create fail: '%s' '%s' on %d\n", user, password, d->descriptor);
		} else {
			d->connected = 2;
			d->player = player;
			fprintf (logfile, "\n%s CREATED %s(%d)on descriptor %d from %s\n",
				     actime(&timenow), Name(player), player, d->descriptor, d->host);
		}
	}
    }

	if( d->connected == 0){
		prompt_user (d);
	}

	if(d->connected == 2){
		if(hack_host(d->host) ){
			sorry3_user(d);
		}else{
			notify(d->player," ");
			notify(d->player, "@PRISM CLIENT START");       /* Send BSX version request */
			if( Age(d->player) != 0l){
				sprintf(buf, "Last logged on %s", ctime(&timenow));
				notify(d->player, buf);
			}
			push_command(d->player, "look here");
			sprintf(buf, "[%s] %s statue stretches", Name(d->player), Name(d->player));
			send_message(d->player, Location(d->player), buf, auth);
			Age(player) = timenow;
		}
	}
}

void parse_connect (msg,command, user, pass, d)
const char *msg;
char *command, *user, *pass;
struct descriptor_data *d;
{
    char *p;

	if( d->connected == 0){
	 while (*msg && isascii(*msg) && isspace (*msg))
			msg++;
	p = command;
	while (*msg && isascii(*msg) && !isspace (*msg))
			*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace (*msg))
			msg++;
	p = user;
	while (*msg && isascii(*msg) && !isspace (*msg))
			*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace (*msg))
			msg++;
	p = pass;
		*p = '\0';
	while (*msg && isascii(*msg) && !isspace (*msg))
			*p++ = *msg++;
	*p = '\0';
	} else {
	p = pass;
	while (*msg && isascii(*msg) && !isspace (*msg))
			*p++ = *msg++;
	*p = '\0';
	}
}

void close_sockets()
{
    struct descriptor_data *d, *dnext;

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;

	if(d->descriptor >= 0){
		if( d->player >0 && d->player < db_top){
			Flags(d->player) &= ~(ROBOT|WEAPON);
		}
		if( d->caps & CAP_BSX){
			mud_write(d->descriptor,"@TMS\r\n", 6);
		}
		mud_write (d->descriptor, shutdown_message, strlen (shutdown_message));
		if (mud_shutdown (d->descriptor, 2) < 0)
		    perror ("shutdown");
		mud_close (d->descriptor);
	}
    }
    mud_close (sock);
}

void emergency_shutdown()
{
	close_sockets();
}

#ifndef MSDOS
void bailout( sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
    char message[1024];
    
    sprintf (message, "BAILOUT: caught signal %d code %d", sig, code);
    panic(message);
    _exit (7);
    return ;
}
#endif

void dump_status()
{
    struct descriptor_data *d;
    long now;

    (void) time(&now);
#ifdef MSDOS
	now = now - 0x83abcd48l;
#endif
    fprintf (logfile, "STATUS REPORT:\n");
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected) {
	    fprintf (logfile, "PLAYING descriptor %d player %s(%d)",
		     d->descriptor, Name(d->player), d->player);

	    if (d->last_time)
		fprintf (logfile, " idle %d seconds\n",
			 now - d->last_time);
	    else
		fprintf (logfile, " never used\n");
	} else {
	    fprintf (logfile, "CONNECTING descriptor %d", d->descriptor);
	    if (d->last_time)
		fprintf (logfile, " idle %d seconds\n",
			 now - d->last_time);
	    else
		fprintf (logfile, " never used\n");
	}
    }
    return ;
}

/* The WHO command */

void dump_users(e)
struct descriptor_data *e;
{
    struct descriptor_data *d;
    long now;
    dbref player = NOTHING, victim;

    strcpy(msgbuf,"");

    (void) time(&now);
#ifdef MSDOS
	now = now-0x83abcd48l;
#endif

    if( e->connected == 2 && !bad_dbref(e->player, 0) &&
	  Typeof(e->player) == TYPE_PLAYER){
		player = e->player;
	notify(player,"Current Players:");
	for (d = descriptor_list; d; d = d->next) {
		if( d->connected==2 && !bad_dbref(d->player, 0) &&
			!(Flags(d->player)&(ROBOT)) ){
			victim = d->player;
			if( TrueWizard(player)){
			    if( Dark(victim)){
				sprintf (msgbuf, "%s (%s) <DARK> in %s (#%d)idle %s",
					Name(victim), d->host, zone_name(victim),
					Location(victim), printsince_sub(now - d->last_time));
			    }else{
				sprintf (msgbuf, "%s (%s) in %s (#%d)idle %s",
					Name(victim), d->host, zone_name(victim),
					Location(victim), printsince_sub(now - d->last_time));
			    }
			    notify(player, msgbuf);
			}else if( !Dark(victim)){
				sprintf (msgbuf, "%s (%s) in %s idle %s",
					Name(victim), d->host,
					zone_name(victim), printsince_sub(now - d->last_time));
				notify(player, msgbuf);
			}
		}
	}
	notify(player,"----------------");
    }else {
	queue_string (e, "Current Players:\r\n");
	for (d = descriptor_list; d; d = d->next) {
		if (d->connected==2  && !bad_dbref(d->player, 0) &&
			!(Flags(d->player) & (ROBOT)) && !Dark(d->player)){
				if (d->last_time)
					sprintf (msgbuf, "%s (%s) in %s idle %d seconds\r\n",
							Name(d->player),
							d->host,
							zone_name(d->player), now - d->last_time);
				else
					sprintf (msgbuf, "%s (%s) idle forever\r\n",
						Name(d->player),
						d->host);
				queue_string (e, msgbuf);
		}
	}
	queue_string(e, "--------------\r\n");
    }
}

/* Hosts that we consider access from as dubious */

char *hack_hosts[] =
{       (char *)"ja.net",
	(char *)"macap.ma.ic.ac.uk",
	NULL
};

int strcmptail(f,t)
char *f, *t;
{       char *f1, *t1;
	char ch;

	f1 = f;
	t1 = t;
	while( *t)t++;
	while( *f)f++;
	while( t != t1 && f != f1){
		ch = *f;
		if( ch >= 'A' && ch <= 'Z')ch = *f + 0x20;
		if( *t != ch)
			break;
		t--;
		f--;
	}
	if( t == t1)
		return 1;       /* T is a tail of f */
	return 0;
}


int hack_host(h)
char *h;
{       char **p;
	if( !h)
		return 0;

	p = hack_hosts;
	while( *p){
		if( strcmptail(h, *p)){
			return 1;       /* don't allow */
		}
		p++;
	}
	return 0;       /* Not in list */
}

