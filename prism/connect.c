/* Connect.c
 */
#include "copyrite.h"

#include <stdio.h>
#ifdef MSDOS

#include "defs.h"

#include "net.h"

#else
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>
#endif
#include <ctype.h>

#ifndef MSDOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef DECNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif

#endif


#include "db.h"
#include "interfac.h"
#include "config.h"
#include "externs.h"

#include "os.h"

#ifdef MSDOS
struct timezone {
char * zone;
};
#endif

extern char *mud_name(PROTO(int));

#ifndef FD_SET
#include "fd_set.h"
#endif

extern struct descriptor_data *descriptor_list ;

extern SOCKET sock;
extern int ndescriptors ;
#ifndef MSC
#ifndef MSDOS
extern int      errno;
#endif
#endif
struct descriptor_data *initializesock( );
char *getpeer();

struct descriptor_data *new_connection(sock)
SOCKET sock;
{
	SOCKET newsock= 0;
	struct descriptor_data *d;

	d = NULL;
	newsock = mud_open();
	if( newsock >= 0){
		d = initializesock(newsock);
	}
    return d;
}

void clearstrings(d)
struct descriptor_data *d;
{
    if (d->output_prefix) {
	FREE (d->output_prefix);
	d->output_prefix = 0;
    }
    if (d->output_suffix) {
	FREE (d->output_suffix);
	d->output_suffix = 0;
    }
}

void shutdownsock(d)
struct descriptor_data *d;
{       SOCKET desc;
	int con;
	dbref play;
	struct descriptor_data *d1 = d;
	
	if( !d)
		return;

	desc = d->descriptor;
	con = d->connected;
	play = d->player;


    if (con == 2 || con == 3) {
	if( !bad_dbref(d->player, 0)){
		fprintf (stderr, "Netdbm:%s DISCONNECT descriptor %d player %s(#%d)\n",
			actime(&timenow),
			desc, Name(play), play);
		d->connected = -1;
		d->player = -1;
#ifdef NO_QUIT
		do_no_quit(play);
#endif
	}
    } else {
	fprintf (stderr, "Netdbm:%s DISCONNECT descriptor %d never connected\n",
		actime(&timenow),
		desc);
    }
    clearstrings (d);
    if(desc >= 0){
	mud_shutdown (desc, 2);
	mud_close (desc);
	d->descriptor = -1;
    }
    freeqs (d);
    if( descriptor_list == d)           /* Throwing head */
	descriptor_list = d->next;
    if(d->prev)
	    d->prev->next = d->next;
    if (d->next)
	d->next->prev = d->prev;
    FREE (d1);
    ndescriptors--;
}

struct descriptor_data *initializesock( s)
SOCKET s;
{
    struct descriptor_data *d;

	if( s == INVALID_SOCKET || s == NULL)
		return NULL;    /* Error */

    ndescriptors++;
    MALLOC(d, struct descriptor_data, 1);
    d->descriptor = s;
    d->connected = 0;
    d->player = 0;      /* Init to something */
#ifdef ROBOT
    if( s >= 0){
#endif

#ifdef ROBOT
    }
#endif
    d->output_prefix = 0;
    d->output_suffix = 0;
    d->output_size = 0;
    d->output.head = 0;
    d->output.tail = &d->output.head;
    d->input.head = 0;
    d->input.tail = &d->input.head;
    d->raw_input = 0;
    d->raw_input_at = 0;
    d->quota = COMMAND_BURST_SIZE;
    d->last_time = 0;

    d->next = descriptor_list;
    d->prev = NULL;
    if (descriptor_list){
	descriptor_list->prev = d;
    }
    d->caps = CAP_NORMAL;
    d->client_data = NULL;
	
    descriptor_list = d;
#ifdef OUTPUT_WRAP
	d->olen = 0;
#endif

    d->host = (char *)alloc_string(mud_name(s));
    
    welcome_user (d);
    return d;
}

