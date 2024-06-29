#ifndef _MATCH_H
#define _MATCH_H

#include "copyrite.h"

#include "db.h"

#define MAX_ARGC 10

struct listofmatches{
struct listofmatches *next;
dbref	amatch;
dbref	sender, receiver;
int	argc;
char	*argv[MAX_ARGC];
};			/* list of matches */

struct	match_vars {
struct	match_vars *prev;
dbref	Exact_match;
int	Check_keys;
dbref	Last_match;
int	Match_count;
dbref	Match_who;
char	*Match_name;
int	Preferred_type;
dbref	Match_loc;
int	No_true;
dbref	World_cache;
struct	listofmatches *List_head;
int	Match_depth;	/* used to stop looping */
int	Match_auth;		/* Auth level of sender */
struct dbref_chain *Xargl[MAX_ARGC];	/* Used to resolve NT_ARGS in CONST's etc */
};

extern struct match_vars *match_vars;

#define exact_match match_vars->Exact_match
#define check_keys match_vars->Check_keys
#define last_match match_vars->Last_match
#define match_count match_vars->Match_count
#define match_who match_vars->Match_who
#define match_name match_vars->Match_name
#define preferred_type match_vars->Preferred_type
#define match_loc match_vars->Match_loc
#define world_cache match_vars->World_cache
#define list_head match_vars->List_head
#define match_depth match_vars->Match_depth
#define match_auth match_vars->Match_auth
#define xargl(x) (match_vars->Xargl[x])



/* match functions */
/* Usage: init_match(player, name, type); match_this(); match_that(); ... */
/* Then get value from match_result() */

/* initialize matcher */
extern void init_match( PROTO3(dbref , const char *, int ));
extern void init_match_check_keys( PROTO3(dbref , const char *, int) );

/* match (LOOKUP_TOKEN)player */
extern void match_player( PROTO(void));

/* match (NUMBER_TOKEN)number */
extern void match_absolute( PROTO(void));

/* match "me" */
extern void match_me( PROTO(void));

/* match "here" */
extern void match_here( PROTO(void));

/* match something player is carrying */
extern void match_possession( PROTO(void));

/* match something in the same room as player */
extern void match_neighbor( PROTO(void));

/* match an exit from player's room */
extern void match_exit( PROTO(void));

/* all of the above, except only Wizards do match_absolute and match_player */
extern void match_everything( PROTO(void));

/* return match results */
extern dbref match_result( PROTO(void)); /* returns AMBIGUOUS for multiple inexacts */
extern dbref last_match_result( PROTO(void)); /* returns last result */

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"

extern dbref noisy_match_result( PROTO(void)); /* wrapper for match_result */
				/* noisily notifies player */
				/* returns matched object or NOTHING */

/* Allow def classes to be inherited */

#define DEF_ROOM (-2)
#define DEF_PLAYER (-3)
#define DEF_THING (-4)

#endif	/* _MATCH_H */

