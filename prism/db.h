#ifndef __DB_H
#define __DB_H

#include "proto.h"

#ifdef NO_CONST
#define const /* */
#endif

#ifdef _WINDOWS
#define NO_UNISTD
#endif

#include <stdio.h>
#ifndef NO_STDLIB
#include <stdlib.h>
#define NO_STDLIB /* just included it... */
#endif
#ifndef NO_UNISTD
#include <unistd.h>
#define NO_UNISTD
#endif

#ifdef MSDOS
#ifndef HAVE_DEFS
/* The command line got too long, so the -D stuff now put in the file defs.h */
#include "defs.h"
#endif
#endif

/* Here are a whole host of DEFS used to make special versions and do 
 * different customisations.
 */


#ifdef MSDOS
#define FREE_TYPE char *
#else
#define FREE_TYPE void *
#endif

#ifdef _WINDOWS
#define class CLass
#define this This
#ifdef const
#undef const
#endif
#endif

#ifdef TEST_MALLOC

#ifndef malloc
#define malloc(x) dosmalloc(x)
#define free(x)   dosfree(x)
#define realloc(x, y) dosrealloc(x, y)

#define freemsg(x, y) dosfreemsg(x, y)

extern void dosfree( PROTO( FREE_TYPE));
extern char *dosmalloc(PROTO(int));
extern char *dosrealloc( );
extern void dosfreemsg( PROTO2(FREE_TYPE, char *) );
#endif
#else
#define freemsg(x,y) free(x)

#endif


#define BOOLEXP struct n_boolexp
#ifdef VM_BOOL
#define BOOLEXP_PTR long
extern BOOLEXP *vm_bool(PROTO(BOOLEXP_PTR));
#else
#define BOOLEXP_PTR BOOLEXP *
#define vm_bool(x) x
#endif


#define Sizeof(x) ((int ) sizeof( x))

#ifdef ATARI
#define dbref short
#else
#define dbref int
#endif

/* Defines that deal with money */
#ifdef MSDOS
#define money  long
#define atomoney(x) atol(x)
#else
#define money  int
#define atomoney(x) atoi(x)
#endif

#ifdef VM_DESC
typedef long desctype ;
extern desctype alloc_desc(PROTO(char *)), alloc_desc_2(PROTO(char *));
char *get_desc(PROTO(long));
#else
typedef const char *desctype;
#endif

extern FILE *logfile;

typedef char * nametype;
typedef char * string;

/* typedef int dbref;	*/	/* offset into db */

#define TYPE_ROOM 	0x0
#define TYPE_THING 	0x1
#define TYPE_EXIT 	0x2
#define TYPE_PLAYER 	0x3
#define NOTYPE		0x7	/* no particular type */
#define TYPE_MASK 	0x7	/* room for expansion */
#define ANTILOCK	0x8	/* negates key (*OBSOLETE*) */
#define WIZARD		0x10	/* gets automatic control */
#define LINK_OK		0x20	/* anybody can link to this room */
#define DARK		0x40	/* contents of room are not printed */
#define TEMPLE		0x80	/* objects dropped in this room go home */
#define STICKY		0x100	/* this object goes home when dropped */
#define BUILDER		0x200	/* this player can use construction commands */
#define FEMALE		0x400	/* SEX of player. */
#define NO_QUIT		0x800   /* Drop on quit/disconnect */
#define INHERIT		0x1000	/* Inherit flag */
#define WEAPON		0x2000	/* Weapon flag */
#define CAN_INHERIT	(DARK|FEMALE|BUILDER)
#define ROBOT		0x4000	/* Robot flag */
#define CONTAINER	0x8000  /* Container for things */

typedef int object_flag_type;

#define Typeof(x) (Flags(x) & TYPE_MASK)
#define TrueWizard(x) (Flags(x) & WIZARD) 
#define Dark(x) ((Flags(x) & (INHERIT | DARK) ) == DARK)
#define Player(x) (Typeof(x) == TYPE_PLAYER)
#define Robot(x) ( ( Flags(x) & (TYPE_MASK |ROBOT) ) == (TYPE_PLAYER|ROBOT))
#define Female(x) (Player(x) ? ((Flags(x) & FEMALE) == FEMALE) : 0)

/* From robot.c */

struct robot {
int	speed;			/* how often robot makes a move */
int	fight_p;		/* How likely to attack unprovoked */
int	cur_speed;		/* current time counter */
char	*comment;
int	move_p, get_p, drop_p;
char	*pass;
};

struct extension {
int  chp;
int  level;
money pennys;				/* How much money you have */
char *password;
struct robot *robot;			/* Robot info */
#ifdef THREAD
struct thread_rec *exthread;		/* Pointer to a thread rec */
#endif
char *robstr;
};

struct locking {
BOOLEXP_PTR key;
};

#define TRUE_BOOLEXP ((BOOLEXP_PTR) 0)

/* special dbref's */
#define NOTHING (-1)		/* null dbref */
#define AMBIGUOUS (-2)		/* multiple possibilities, for matchers */
#define HOME (-3)		/* virtual room, represents mover's home */

#define MAINFIELDS     int ver;\
    object_flag_type flags; dbref CLass;\
    desctype name;\
dbref location; dbref contents; dbref next; \
    dbref owner; long age;long position;

struct object {
    MAINFIELDS
    desctype description;
    money pennies;		/* number of pennies object contains */
    struct locking *(*lock)[];	/* if not NOTHING, must have this to do op */
    struct extension *ext;	/* Extension record  for players */
};

struct small_object {
    MAINFIELDS
    desctype description;
 };

struct player_object {
    MAINFIELDS
    desctype description;
     struct extension extp;
};

struct exit_object {
    MAINFIELDS
    BOOLEXP_PTR key;
};

struct room_object {
    MAINFIELDS
    desctype description;
     dbref zone;
};

struct thing_object {
    MAINFIELDS
    desctype description;
 };

#ifdef VM_DB
union any_object {
struct object o;
struct exit_object e;
struct room_object r;
struct player_object p;
struct thing_object t;
};
#endif

#ifdef VM_DB
extern struct object *vm_db(PROTO(dbref));
extern struct object *vm_dbw(PROTO(dbref));
#else
extern struct object **db;
#endif
extern dbref db_top;
extern char *msgbuf;		/* Used all over the place */
extern const char *alloc_string(PROTO(const char *));
extern dbref new_object();	/* return a new object */
extern dbref getref (PROTO(FILE *));   /* Read a database reference from a file. */
extern void putref (PROTO3(FILE *, dbref, char *)); /* Write one ref to the file */
extern BOOLEXP_PTR getboolexp(PROTO(FILE *)); /* get a boolexp */
extern void putboolexp(PROTO3(FILE *, BOOLEXP_PTR, dbref)); /* put a boolexp */
extern int db_write_object(PROTO2(FILE *, dbref)); /* write one object to file */
extern dbref db_write(PROTO(FILE *));	/* write db to file, return # of objects */
extern dbref db_read(PROTO(FILE *));	/* read db from file, return # of objects */
				/* Warning: destroys existing db contents! */
extern void free_boolexp(PROTO(BOOLEXP_PTR));
extern void db_free(PROTO(void));
extern dbref parse_dbref(PROTO(const char *));	/* parse a dbref */
extern dbref cur_player;	/* who is being done. */
extern dbref euid;

#define DOLIST(var, first) \
  for((var) = (first); (var) != NOTHING; (var) = Next( (var)) )
#define PUSH(thing, locative) \
    ((Next(thing) = (locative)), (locative) = (thing))
#define getloc(thing) Location(thing)

#define SET_BUILDER_LEVEL 12
#define ALLOW_TELEPORT	  12

#ifndef VERP
/* Debugging stuff */
#define VERP 2
#define VERR 4
#define VERE 3
#define VERD 0
#define VERT 5	/* Thing version number */
#endif

extern struct locking *has_lock(PROTO2(dbref, int));
extern BOOLEXP_PTR has_lock_key(PROTO2(dbref, int));

#ifndef Key
#ifdef VM_DB
#define Db(x) (vm_db(x))
#define DbW(x, y) (vm_dbw(x)->y)
#else
#define Db(x) (db[x])
#define DbW(x, y) (db[x]->y)
#define DbFree(x) free((FREE_TYPE) db[x])
#endif

/* access macros */
#define Flags(x) (Db(x)->flags)
#ifdef VM_DESC
#define Desc(x) (Db(x)->description ? get_desc(Db(x)->description) : NULL)
#define DescW(x, y) DbW(x,description = alloc_desc( y))
#define Name(x)  (Db(x)->name ? get_desc(Db(x)->name) : NULL)
#define NameW(x, y) DbW(x,name = alloc_desc( y))
#else
#define Desc(x)  (Db(x)->description)
#define DescW(x, y) DbW(x,description = alloc_string(y))
#define Name(x)  (Db(x)->name)
#define NameW(x, y)  DbW(x,name = alloc_string(y))
#endif
#define Owner(x) (Db(x)->owner)
#define Class(x) (Db(x)->CLass)
#define Next(x) (Db(x)->next)
#define Age(x) (Db(x)->age)
#define Location(x)(Db(x)->location)
#define Contents(x) (Db(x)->contents)
#define Position(x) (Db(x)->position)

/* Player only fields */
#define Ext(x)	 ( ( ( struct player_object *) Db(x))->extp )
#define Chp(x)   ( Ext(x).chp )
#define Level(x) ( Ext(x).level )
#define Mhp(x)   (player_level_table[Level(x)].mhp)
#define Pennies(x) ( Ext(x).pennys)

/* Exit only */
#define Key(x)   ( ( ( struct exit_object *)Db(x))->key )

/* Room only */
#define Zone(x) ( ( ( struct room_object *)Db(x))->zone )

/* Obsolete fields */

#define Commands(x) ( Db(x)->commands)
#define KeyN(x,y)   ( has_lock_key(x,y))
#define Lock(x)	(Db(x)->lock)
#define OldPennies(x) (Db(x)->pennies)
#define OldExt(x)   ( Db(x)->ext)


#endif /* Key */

extern dbref default_room_class, default_thing_class, default_player_class;
extern dbref default_player_start, default_wizard, default_beaker;
extern dbref default_temple_class;
extern void version(PROTO3(dbref, int, char *));

#ifndef CHAIN
#define CHAIN

struct dbref_chain {
struct dbref_chain *next;
dbref This;
};

extern struct dbref_chain *make_chain(PROTO(dbref));
extern void free_chain(PROTO(struct dbref_chain *));
extern struct dbref_chain *add_chain(PROTO2(struct dbref_chain *, dbref));
extern struct dbref_chain *alloc_chain(PROTO(void));
#endif

#ifdef TEST_MALLOC
#define MALLOC(res, type, num) res = (type *) dosmalloc( num * Sizeof(type))
#define FREE(x) (dosfree(x))

#else
#define MALLOC(result, type, number) do {			\
	if (!((result) = (type *) malloc ((number) * Sizeof (type))))	\
		panic("Out of memory");				\
	} while (0)

#define FREE(x) (free(x))
#endif

#ifndef TIME

#ifdef MSDOS

#ifdef MSC
#include <time.h>
#define TIME time_t
#else
#define TIME long
#endif

#else
#include <sys/types.h>
#include <time.h>
#ifndef NO_TIMEB
#include <sys/timeb.h>
#endif
#define TIME time_t
#endif

#endif

#ifndef _WINDOWS
#define class CLass
#define this This
#endif

extern int logged_in __PROTO((dbref));
extern void setupxargs __PROTO((int, char **, struct dbref_chain *));
extern void free_string __PROTO((char *));
extern dbref loc_of __PROTO((dbref));
extern int bad_dbref __PROTO((dbref, dbref));
extern int astrcmp __PROTO((char *, char *));

/* from media.c */
extern int show __PROTO((dbref, dbref, dbref));
extern int visualise __PROTO((dbref, dbref, char *));
extern int show_flush __PROTO((dbref, dbref, int));
extern int hide __PROTO((dbref, dbref, dbref));
 
#endif /* __DB_H */
