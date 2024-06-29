#include "copyrite.h"

#include "db.h"
#ifndef _DESCRIPT_
#include "descript.h"
#endif

#ifndef MAX_USERS
#define MAX_USERS (5+20)
#endif

#define Notify( a, b) notify( a, b)

/* these symbols must be defined by the interface */
extern void notify(PROTO2(dbref , const char *));
extern void notify2(PROTO3(dbref , const char *, const char *));
extern void notify3(PROTO4(dbref , const char *, const char * ,const char *));
extern int shutdown_flag; /* if non-zero, interface should shut down */
extern void emergency_shutdown(PROTO(void));
extern int quiet;			/* Non Zero dont tell player */

/* the following symbols are provided by game.c */

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 1024
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)
extern void process_command(PROTO2(dbref , char *));

extern dbref create_player(PROTO3(const char *, const char *, struct descriptor_data *));
extern dbref connect_player(PROTO3(const char *, const char *, struct descriptor_data *));
extern void do_look_around(PROTO(dbref ));

extern int init_game(PROTO2(const char *, const char *));
extern void dump_database(PROTO(void));
extern void panic(PROTO(const char *));
