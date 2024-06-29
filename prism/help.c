#include "copyrite.h"

/* commands for giving help */

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "externs.h"

void spit_file(player, filename)
dbref player;
const char *filename;
{
    FILE *f;
    char buf[BUFFER_LEN];
    char *p;

    if((f = fopen(filename, "r")) == NULL) {
	sprintf(buf, "Sorry, %s is broken.  Management has been notified.",
		filename);
	notify(player, buf);
	fputs("spit_file:", stderr);
	perror(filename);
    } else {
	while(fgets(buf, Sizeof(buf), f)) {
	    for(p = buf; *p; p++) if(*p == '\n') {
		*p = '\0';
		break;
	    }
	    notify(player, buf);
	}
	fclose(f);
    }
}

void spit_file_d(d, filename)
struct descriptor_data *d;
const char *filename;
{
    FILE *f;
    dbref player = d->player;
    char buf[BUFFER_LEN];
    char *p;

    if((f = fopen(filename, "r")) == NULL) {
	sprintf(buf, "Sorry, %s is broken.  Management has been notified.",
		filename);
	queue_string(d, buf);
	fputs("spit_file:", stderr);
	perror(filename);
    } else {
	while(fgets(buf, Sizeof(buf), f)) {
	    for(p = buf; *p; p++) if(*p == '\n') {
		*p = '\0';
		break;
	    }
	    queue_string(d, buf);
	}
	fclose(f);
    }
}

void do_help(player)
dbref player;
{
    spit_file(player, HELP_FILE);
}

void do_news( player)
dbref player;
{
    spit_file(player, NEWS_FILE);
}

