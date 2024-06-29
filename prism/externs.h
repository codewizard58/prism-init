#include "copyrite.h"

/* Prototypes for externs not defined elsewhere */
#include "db.h"
#ifndef __PROTO
#ifndef NOPROTO
#define __PROTO(x) x
#else
#define __PROTO(x) ()
#endif
#endif

extern dbref home_of(PROTO(dbref));

/* From create.c */
extern void do_open(PROTO3(dbref , const char *, const char *));
extern void do_link(PROTO3(dbref , const char *, const char *));
extern void do_relink(PROTO3(dbref , const char *, const char *));
extern void do_dig(PROTO3(dbref , const char *, const char *));
extern void do_create(PROTO3(dbref , char *, char * ));

/* From game.c */
extern void do_dump(PROTO(dbref) );
extern void do_shutdown(PROTO(dbref ));

/* From help.c */
extern void spit_file(PROTO2(dbref , const char *));
extern void do_help(PROTO(dbref ));
extern void do_news(PROTO(dbref ));

/* From look.c */
extern void look_room(PROTO2(dbref , dbref ));
extern void do_look_around(PROTO(dbref ));
extern void do_look_at(PROTO2(dbref , char *));
extern void do_examine(PROTO2(dbref , const char *));
extern void do_inventory(PROTO(dbref) );
extern void do_find(PROTO2(dbref , const char *));
extern long ageof(PROTO(dbref ));
extern char *zone_name(PROTO(dbref));

/* From move.c */
extern void moveto(PROTO2(dbref , dbref ));
extern void enter_room(PROTO2(dbref , dbref ));
extern void send_home(PROTO(dbref ));
extern int can_move(PROTO2(dbref , const char *));
extern void do_move(PROTO2(dbref , const char *));
extern void do_get(PROTO3(dbref , const char *, const char *));
extern void do_drop(PROTO2(dbref , const char *));
#ifdef Player_flags
extern object_flag_type inheritance( PROTO(dbref) );
#endif

/* From player.c */
extern dbref lookup_player(PROTO(const char *));
extern void do_password(PROTO3(dbref , const char *, const char *));

/* From predicates.h */
extern int can_link_to( PROTO2(dbref , dbref ));
extern int could_doit( PROTO2(dbref , dbref ));
extern int can_doit( PROTO3(dbref , dbref , const char *));
extern int can_see( PROTO3(dbref , dbref , int ));
extern int controls( PROTO2(dbref , dbref ));
extern int payfor( PROTO2(dbref , money ));

/* From rob.c */
extern void do_kill( PROTO3(dbref , const char *, char * ));
extern void do_give( PROTO3(dbref , char *, char * ));
extern void do_rob( PROTO2(dbref , const char *));

/* From set.c */
extern void do_name( PROTO3(dbref , const char *, char *));
extern void do_describe( PROTO3(dbref , const char *, const char *));
extern void do_fail( PROTO3(dbref , const char *, const char *));
extern void do_success( PROTO3(dbref , const char *, const char *));
extern void do_osuccess( PROTO3(dbref , const char *, const char *));
extern void do_ofail( PROTO3(dbref , const char *, const char *));
extern void do_lock( PROTO3(dbref , const char *, const char *));
extern void do_unlock( PROTO2(dbref , const char *));
extern void do_unlink( PROTO2(dbref , const char *));
extern void do_chown( PROTO3(dbref , const char *, const char *));
extern void do_set( PROTO3(dbref , const char *, const char *));
extern char *prt_money( PROTO(money));

/* From speech.c */
extern void do_wall( PROTO3(dbref , const char *, const char *));
extern void do_gripe( PROTO3(dbref , const char *, const char *));
extern void do_say( PROTO3(dbref , const char *, const char *));
extern void do_page( PROTO2(dbref , const char *));

/* From notify.c */
extern void notify( PROTO2(dbref, const char *));
extern void notify2( PROTO3(dbref, const char *, const char *));
extern void notify_except( PROTO3(dbref , dbref , const char *));
extern void notify2_except( PROTO4(dbref , dbref , const char *, const char *));
extern void notify_except2( PROTO4(dbref , dbref , dbref , const char *));
extern void denied( PROTO(dbref));
extern void bsxnotify( PROTO2(dbref, const char *));
extern void bsxnotify_except( PROTO3(dbref , dbref , const char *));

/* From stringutil.c */
extern int string_compare( PROTO2(const char *, const char *));
extern int string_prefix( PROTO2(const char *, const char *));
extern int string_match( PROTO2(const char *, const char *));

/* From utils.c */
extern int member( PROTO2(dbref , dbref ));
extern dbref remove_first( PROTO2(dbref , dbref ));
extern dbref reverse(PROTO(dbref));

/* From wiz.c */
extern int is_god( PROTO(dbref));
extern int rank_of( PROTO(dbref));
extern int ArchWizard( PROTO(dbref));
extern int Wizard( PROTO(dbref));
extern int LocalWizard( PROTO(dbref));
extern int is_wiz_in_zone( PROTO2(dbref, dbref));
extern void do_teleport( PROTO3(dbref , const char *, const char *));
extern void do_force( PROTO3(dbref , const char *, char *));
extern void do_stats( PROTO2(dbref , const char *));
extern void do_toad( PROTO2(dbref, const char *));
extern void do_newpassword( PROTO3(dbref, const char *, const char *));
extern void at_zone( PROTO3(dbref, char *, char*));

/* From boolexp.c */
extern int eval_boolexp( PROTO3(dbref , BOOLEXP_PTR , dbref));
extern BOOLEXP_PTR parse_boolexp( PROTO2(dbref , const char *));
void xpand( PROTO4(dbref, char *, char *, int));
extern int last_result;

/* From unparse.c */
extern const char *unparse_object( PROTO2(dbref , dbref ));
extern const char *unparse_boolexp( PROTO2(dbref , BOOLEXP_PTR ));

/* From interfac.c */
extern char *actime();
extern TIME timenow;

/* From fight.c */
#ifdef WEAPON
extern const char *player_title( PROTO(dbref));
extern int player_level(PROTO(dbref));
#endif
extern char *reconstruct_message();

/* From zone.c */
extern dbref zoneof(PROTO(dbref));
extern dbref zone_owner(PROTO(dbref));
extern dbref world_of(PROTO(dbref));

#ifdef VM_DB
extern struct object *vm_dbf(PROTO(dbref));
extern struct object *vm_dbfw(PROTO(dbref));
extern void vm_grow(PROTO(dbref));
#endif

/* From text.c */

extern void show_text_stats(PROTO(dbref));
extern struct text_block *make_text_block( PROTO2(char *, int));
extern void free_text_block( PROTO(struct text_block *));
extern void add_to_queue( PROTO3(struct text_queue *,const char *, int));
extern void replace_text( PROTO5(struct text_queue *,struct text_block *, const char *, int, int));
extern int flush_queue(PROTO2(struct text_queue *, int));
extern int queue_write(PROTO3(struct descriptor_data *, const char *, int));
extern int queue_string( PROTO2(struct descriptor_data *, const char *));
extern char *ranmsg( PROTO2(char **, int));

/* match.c */
extern dbref receiver,sender;
extern int xargc;
extern char **xargv;
extern dbref wizauth();

void wprintf(PROTO3(dbref,const char *, ...));


