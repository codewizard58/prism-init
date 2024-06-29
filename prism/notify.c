#include "copyrite.h"

#include <stdio.h>

#ifndef MSDOS
#include <sys/types.h>
#endif


void sorry_user();

#include <ctype.h>

#ifndef MSDOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


#include "db.h"
#include "interfac.h"
#include "config.h"
#include "descript.h"

extern struct descriptor_data *descriptor_list;
extern int shout_zone;
extern dbref euid;

/* Messages used all over the place */

void WizOnly(player)
dbref player;
{
	notify(player,"Sorry Wizards only!");
}


void no_memory(msg)
char *msg;
{
	fprintf(logfile,"\nNo memory in %s", msg);
	exit(1);
}

void denied(player)
dbref player;
{	notify(player, "Permission denied!");
}

int quiet;

/* Notify is used to send messages to logged in players */
/* notify() takes one string
 * notify2  takes two
 * notify3  takes three
 */

int ex_queue_string(d, msg)
struct descriptor_data *d;
char *msg;
{	char *p, *q;
	int cnt;
	int i,j;
	
    cnt = 0;
	p = msg;
	q = p;
	while(*p){
		i = 0;
		j = i;
		q = p;
		while(*p){
			if( *p == '\\'){
				if( p[1] == 'n'){
					if( i > 0){
						queue_write(d, q, i);
						queue_write(d, "\r\n", 2);
					}
					cnt = 0;
					p = &p[2];
					q = p;
					i = 0;
					j = 0;
					continue;
				}
			}
			if( *p == ' '){
				if( cnt + i > 65){	/* out then wrap */
					if( j > 0)
						queue_write(d, q, j);
					queue_write(d, "\r\n", 2);
					q = &q[j];
					cnt = 0;
					i = i - j;
					if( cnt+i > 65){
						queue_write(d, q, i);
						queue_write(d, "\r\n", 2);
						cnt = 0;
						q = &q[i];
						i = 0;
						j = 0;
					}
					continue;
				}
				j = i;
			}
			p++;
			i++;
		}
	}
	if( i > 0)
		queue_write(d, q, i);
	return cnt;
}


void notify(player, msg)
dbref player;
const char *msg;
{   struct descriptor_data *d;

	if( Flags(player)&ROBOT)
		return;

	if(!quiet)
	    for (d = descriptor_list; d; d = d->next) {
			if (d->connected>= 0 && d->connected <= 2 && 
				d->descriptor >= 0 &&	/* robots on -1 */
				d->player == player) {
		    		ex_queue_string (d, msg);
		    		queue_write (d, "\r\n", 2);
			}
    	}
}

#ifdef NOTDEF
void bsxnotify(player, msg)
dbref player;
const char *msg;
{
    struct descriptor_data *d;

	if( Flags(player)&ROBOT)
		return;

	if(!quiet)
	    for (d = descriptor_list; d; d = d->next) {
		if (d->connected>= 0 && d->connected <= 2 && 
			d->descriptor >= 0 &&	/* robots on -1 */
			d->player == player &&
			(d->caps & CAP_BSX)) {
	    		queue_string (d, msg);
	    		queue_write (d, "\r\n", 2);
		}
    	}
}
#endif

#ifdef NOTDEF
void notify2(player, msg, msg2)
dbref player;
const char *msg, *msg2;
{
    struct descriptor_data *d;

	if( Flags(player)&ROBOT)
		return;

	if(!quiet)
	    for (d = descriptor_list; d; d = d->next) {
		if (d->connected>= 0 && d->connected <= 2 && 
			d->descriptor >= 0 &&	/* robots on -1 */
			d->player == player) {
	    		ex_queue_string (d, msg);
	    		ex_queue_string (d, msg2);
	    		queue_write (d, "\r\n", 2);
		}
    	}
}

void notify3(player, msg, msg2, msg3)
dbref player;
const char *msg, *msg2, *msg3;
{
    struct descriptor_data *d;

	if( Flags(player)&ROBOT)
		return;

	if(!quiet)
	    for (d = descriptor_list; d; d = d->next) {
		if (d->connected>= 0 && d->connected <= 2 && 
			d->descriptor >= 0 &&	/* robots on -1 */
			d->player == player) {
	    		ex_queue_string (d, msg);
	    		ex_queue_string (d, msg2);
	    		ex_queue_string (d, msg3);
	    		queue_write (d, "\r\n", 2);
		}
    	}
}
#endif

void notify_except( first, exception, msg)
dbref first, exception;
const char *msg;
{	int depth =0;
	dbref saved_first = first;

  if( !quiet){
    DOLIST (first, first) {
	if( depth++ > 1000){
		fprintf(logfile,"Depth loop in notify_except(%d, %d, %s)\n", saved_first, exception, msg);
		break;
	}
	if ( Player(first) && first != exception) {
	    notify (first, msg);
	}
    }
  }
}

#ifdef NOTDEF
void bsxnotify_except( first, exception, msg)
dbref first, exception;
const char *msg;
{	int depth =0;
	dbref saved_first = first;

  if( !quiet){
    DOLIST (first, first) {
	if( depth++ > 1000){
		fprintf(logfile,"Depth loop in notify_except(%d, %d, %s)\n", saved_first, exception, msg);
		break;
	}
	if ( Player(first) && first != exception) {
	    bsxnotify (first, msg);
	}
    }
  }
}

void notify2_except( first, exception, msg, msg1)
dbref first, exception;
const char *msg, *msg1;
{	int depth = 0;
	dbref saved_first = first;

  if( !quiet){
    DOLIST (first, first) {
	if( depth++ > 1000){
		fprintf(logfile,"Depth loop in notify2_except(%d, %d, %s, %s)\n", saved_first, exception, msg, msg1);
		break;
	}
	if ( Player(first) && first != exception) {
	    notify2(first, msg, msg1);
	}
    }
  }
}

void notify_except2( first, exc1, exc2, msg)
dbref first, exc1, exc2;
const char *msg;
{	int depth = 0;
	dbref saved_first = first;

    DOLIST (first, first) {
	if( depth++ > 1000){
fprintf(logfile,"Depth loop in notify_except2(%d, )\n", saved_first);
		break;
	}
	if ((Flags(first) & TYPE_MASK) == TYPE_PLAYER
	    && first != exc1
	    && first != exc2) {
		    notify (first, msg);
		}
    }
}
#endif

/* This is a WORLD broadcast */

void notify_all_except( player, arg2)
dbref player;
const char *arg2;
{   struct descriptor_data *d;
    char abuf[10];

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected == 2 && d->player != player 
	   ) {
		if( shout_zone== -1 || shout_zone == zoneof(d->player) ){
			if( Flags(d->player) & WIZARD ){
				sprintf(abuf, "%d: ", player);
				queue_string(d, abuf);
			}
			ex_queue_string (d, arg2);
		    	queue_write (d, "\r\n", 2);
		}
	}
    }
}


void wprintf( player, msg)
dbref player;
const char* msg;
{
    struct descriptor_data *d;
    char tempBuff[1024];

	if( Flags(player)&ROBOT)
		return;

	if(!quiet)
	    for (d = descriptor_list; d; d = d->next) {
		if (d->connected>= 0 && d->connected <= 2 && 
			d->descriptor >= 0 &&	/* robots on -1 */
			d->player == player) {
			vsprintf(tempBuff, msg, (&msg)+1);
	    		ex_queue_string (d, tempBuff);
		}
    	}
}
