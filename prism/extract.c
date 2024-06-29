/* New extract.c */

#include "db.h"
#include "boolexp.h"

int extracting;
dbref *extract_list;
int extract_top;

extern char *saved_dumpfile,*dumpfile;
extern int alarm_triggered;

void extract_renumber()
{	dbref i,j;

	j = 0;
	for(i = 0; i < extract_top; i++){
		if( extract_list[i] != NOTHING)
			extract_list[i] = j++;
	}
}


int is_in_zone(obj, zone)
dbref obj,zone;
{	int cnt = 0;
	dbref orig = obj;

	if(obj == zone)return 1;

	obj = zoneof(obj);
	while( obj >= 0 && obj < db_top && 
		Typeof(obj) == TYPE_ROOM &&
		(Flags(obj) & INHERIT) && cnt++ < 20){
		if( obj == zone){
			return 1;
		}
		obj = Zone(obj);
	}
	return 0;
}

int is_in_class(obj, zone)
dbref obj,zone;
{	int cnt = 0;
	dbref orig = obj;

	if(obj == zone)return 1;

	obj = Class(obj);
	while( !bad_dbref(obj, 0) && Typeof(obj) == TYPE_ROOM &&
		(Flags(obj) & INHERIT) && cnt++ < 20){
		if( obj == zone){
			return 1;
		}
		obj = Class(obj);
	}
	return 0;
}

void mark(x)
dbref x;
{
	if( x >= 0 && x < extract_top)
		extract_list[x] = x;
}

void mark_list(i)
dbref i;
{	dbref j,k;

	while( i != NOTHING){
		mark(i);
		i = Next(i);
	}
}

void mark_all_in_zone(zone)
dbref zone;
{
	dbref i,j;

/* Mark all rooms in that zone */
	for(i=2; i < db_top;i++){
		if( Typeof(i) == TYPE_ROOM && is_in_zone(i, zone)){
fprintf(logfile,"Marking %d in zone %d\n", i, zone);
			mark(i);
			mark(Owner(i));
		}
	}

}

void mark_all_in_class(mclass)
dbref mclass;
{
	dbref i,j;

/* Mark all rooms in that class */
	for(i=2; i < db_top;i++){
		if( Typeof(i) == TYPE_ROOM && is_in_class(i, mclass)){
fprintf(logfile,"Marking %d in class %d\n", i, mclass);
			mark(i);
			mark(Owner(i));
		}
	}

}

/* Move all the marked objects in the list to the front and put the
 * unmarked ones at the end.
 */

dbref sort_extract_list(x)
dbref x;
{	dbref marked, unmarked, i, j;

	marked = NOTHING;
	unmarked=NOTHING;

	i = x;

	while( i != NOTHING){
		j = Next(i);
		if( i < extract_top && extract_list[i]!= NOTHING){
			Next(i) = marked;
			marked = i;
		}else {
			Next(i) = unmarked;
			unmarked = i;
		}
		i = j;
	}

/* return the new head of the list */
	if( marked != NOTHING){
/* Join the unmarked list on end of marked one. */
		i = marked;
		while( Next(i) != NOTHING)
			i = Next(i);
		Next(i) = unmarked;

		x = marked;
	}else{
		x = unmarked;
	}
	return x;
}

void mark_classes_of(x)
dbref x;
{	int depth = 0;

	while(x >= 0 && x < extract_top && depth++ < 20){
		x = Class(x);

		if( x < 0 || x >= extract_top)
			break;
		if( Typeof(x) != TYPE_ROOM || !(Flags(x) & INHERIT))
			break;	/* Not a class */
		extract_list[x] = x;
	}
}

void mark_lock();

void mark_exits_of(i)
dbref i;
{	dbref j,k;

	i = Contents(i);

	while( i != NOTHING){
		if( Typeof(i) == TYPE_EXIT && Contents(i) != NOTHING){ /* don't mark unlinked exits */
			mark(i);
			mark_lock(Key(i));
		}
		i = Next(i);
	}
}

void mark_all_players(player)
dbref player;
{	dbref i;

	for(i = 0 ; i < extract_top; i++){
		if( Typeof(i) == TYPE_PLAYER)
			mark(i);
	}
}

static dbref has_marked_flag;

void has_marked_sub(b)
BOOLEXP_PTR b;
{	dbref i;

	if( vm_bool(b)->n_func == BOOLEXP_TEL){
		if( NT_T2(b) == NT_DBREF){
			i = vm_bool(b)->u2.d;
			if( i > 1 && i < extract_top && extract_list[i] != NOTHING){
				has_marked_flag = i;
			}
		}
	}
}

dbref has_marked_in_lock(b)
BOOLEXP_PTR b;
{
	if( b == TRUE_BOOLEXP)
		return 0;

	has_marked_flag = NOTHING;

	apply_func(b, has_marked_sub);
	
	return has_marked_flag;
}

void mark_lock_sub(b)
BOOLEXP_PTR b;
{

    if(b == TRUE_BOOLEXP) {
	return;
    } else {
	switch(vm_bool(b)->n_func) {
	  case BOOLEXP_AND:
	  case BOOLEXP_OR:
	  case BOOLEXP_NOT:
		return;

	  case BOOLEXP_CONST:
	    if( NT_T1(b) == NT_DBREF)
	    	mark(OBJ1(b));
	    break;

	  case BOOLEXP_GROUP:
		if( NT_T1(b) == NT_DBREF ){
			mark( OBJ1(b));
		}
		return;

	  case BOOLEXP_FUNC:
	  case BOOLEXP_DROP:
	  case BOOLEXP_TRUE:
	  case BOOLEXP_FALSE:
		return;

	  default: 
		if( NT_T1(b) == NT_DBREF ){
			mark(OBJ1(b));
		}
		if( NT_T2(b) == NT_DBREF ){
			mark( OBJ2(b));
		}
		return;

	}
    }
}

void mark_lock(b)
BOOLEXP_PTR b;
{
	if( b == TRUE_BOOLEXP)
		return;

	has_marked_flag = NOTHING;

	apply_func(b, mark_lock_sub);
	
	return;
}

/* Mark all objects that have a home in a marked room */
/* This is done by scanning the commands all objects for MOVE{..} */
/* locks that point to marked rooms */

void mark_all_homes()
{	dbref i,j;
	
	for(i=2; i < extract_top; i++){
		switch(Typeof(i)){
		default:
			break;
		case TYPE_PLAYER:
		case TYPE_THING:
			if( extract_list[i] != NOTHING)
				break; 	/* Already marked */
			for(j=Contents(i); j != NOTHING; j=Next(j)){
				if( Typeof(j) == TYPE_EXIT && Contents(j) != NOTHING){
					has_marked_in_lock(Key(j));
					if( has_marked_flag != NOTHING){
						extract_list[i] = i;
fprintf(logfile,"Found object %d has a home in %d\n", i, has_marked_flag);
						break;
					}
				}
			}
		}
	}
}

int mark_default_stuff(player)
dbref player;
{
	if( !bad_dbref(default_room_class, 0))
		mark(default_room_class);
	if( !bad_dbref(default_thing_class, 0))
		mark(default_thing_class);
	if( !bad_dbref(default_player_class, 0))
		mark(default_player_class);
	if( !bad_dbref(default_wizard, 0)){
		mark(default_wizard);
		mark_classes_of(default_wizard);
	}
	if( !bad_dbref(default_beaker, 0)){
		mark(default_beaker);
		mark_classes_of(default_beaker);
	}
	if( !bad_dbref(default_player_start, 0)){
		mark(default_player_start);
	}
	if( !bad_dbref(default_temple_class, 0)){
		mark(default_temple_class);
	}
	return 0;
}

/* This routine does all the things that are needed before the dump can
 * be done. */

void extract_mark_needed(player)
dbref player;
{	dbref i,j;
	int cnt;

fprintf(logfile,"Mark default things\n");
	mark_default_stuff(player);

fprintf(logfile,"Mark all homes\n");
	mark_all_homes();

/* Mark classes of extracted objects */
fprintf(logfile,"Mark classes\n");
	for(i=0; i < extract_top; i++){
		if( extract_list[i] != NOTHING && Typeof(i) != TYPE_EXIT){
			mark_classes_of(i);
		}
	}

/* Mark commands/exits of all marked objects */
	for(i=0; i <extract_top; i++){
		if(extract_list[i] != NOTHING && Typeof(i) != TYPE_EXIT){
			mark_exits_of(i);
		}
	}

/* Sort the contents lists of marked objects so that the extracted objects
 * will have valid contents lists */
	for(i=0 ; i < extract_top; i++){
		if( extract_list[i] != NOTHING && Typeof(i) != TYPE_EXIT){
			Contents(i) = sort_extract_list(Contents(i));
		}
	}
}

void do_extract(player, arg1, arg2)
dbref player;
char *arg1, *arg2;
{	int i, cnt;
	char combuf[20],*p;
	dbref thing;

	if( !arg1 || !*arg1){
		notify(player, "Extract commands");
		notify(player, "----------------------------------------------------");
		notify(player, "/class class ...... Mark all in class");
		notify(player, "/def .............. Mark the Default classes");
		notify(player, "/done ............. Finished extracting");
		notify(player, "/dump file ........ Do the extract");
		notify(player, "/list ............. List current marked");
		notify(player, "/negate ........... Swap marked and unmarked");
		notify(player, "/owned owner ...... Mark all owned by");
		notify(player, "/players .......... Mark all players");
		notify(player, "/unmark #obj ...... Unmark a specific object");
		notify(player, "/zone zone ........ Mark all in zone");
		notify(player, "#obj .............. Mark a specific object");
		notify(player, "-----------------------------------------------------");
		notify(player, "Note: Gods only!");
		if( is_god(player)){
			sprintf(msgbuf,"Extract_top = %d", extract_top);
			notify(player, msgbuf);
			sprintf(msgbuf,"Extracting: %s", extracting ? "Yes":"no");
			notify(player, msgbuf);
			cnt = 0;
			for(i=0; i < extract_top; i++){
				if( extract_list && extract_list[i] != NOTHING)
					cnt++;
			}
			sprintf(msgbuf,"%d objects marked", cnt);
			notify(player, msgbuf);
		}
		return;
	}


	if( !is_god(player)){
		denied(player);
		return;
	}

	if( extract_list == NULL){
		extract_list = (int *)malloc( sizeof(dbref) * db_top);
		for(i=0; i < db_top;i++)
			extract_list[i] = NOTHING;
		extract_list[0] = 0;
 		extract_list[1] = 1;
		extract_top = db_top;
	}

	if( *arg1 == '/'){
		p = combuf;
		arg1++;
		cnt = 0;
		while(cnt++ < 19 && (*p = *arg1++) && *p != ' '){
			p++;
		}
		*p = '\0';
		while(*arg1 == ' ')arg1++;	/* skip spaces */
		
		if( !strcmp(combuf, "done")){
			free( extract_list);
			extract_list = NULL;
			extracting = 0;
			extract_top= 0;
			notify(player,"Extract resources returned");
			return;
		}else if( !strcmp(combuf, "dump")){
			do_noquit_scan();			/* Make sure everything is at its starting point */
			extract_mark_needed(player);		/* Mark all that need to be marked */
			fnotify(player,"Extract dump done");
			extracting = 1;
			extract_renumber();
			alarm_triggered = 1;
			saved_dumpfile = dumpfile;
			if( *arg1)
				dumpfile = arg1;
			return;
		}else if( !strcmp(combuf, "list")){
			extract_renumber();
			notify(player,"Extracting the following:");
			for(i=0; i < extract_top; i++){
				if( extract_list[i] != NOTHING && Typeof(i) != TYPE_EXIT){
					sprintf(msgbuf,"%s(%d) as %d", Name(i), i, extract_list[i]);
					notify(player, msgbuf);
				}
			}
			notify(player,"------------");
			return;
		}else if( !strcmp(combuf, "negate")){
			mark_all_homes();

/* Mark commands/exits of all marked objects */
			for(i=0; i <extract_top; i++){
				if(extract_list[i] != NOTHING && Typeof(i) != TYPE_EXIT){
					mark_exits_of(i);
				}
			}

			for(i=2; i < extract_top; i++){
				if( extract_list[i] == NOTHING)
					extract_list[i] = i;
				else
					extract_list[i] = NOTHING;
			}
			notify(player,"Mark list negated");
			return;
		}else if( !strcmp(combuf, "owned")){
			init_match(player, arg1, NOTYPE);
			match_everything();
			thing = match_result();
			if( !bad_dbref(thing, 0) && Typeof(thing) == TYPE_PLAYER){
				for(i=0; i < extract_top; i++){
					if( Typeof(i) != TYPE_EXIT && Owner(i) == thing){
						mark(i);
					}
				}
				sprintf(msgbuf,"Marked all owned by %s(%d)", Name(thing), thing);
				notify(player, msgbuf);
			}
			return;
		}else if( !strcmp(combuf, "unmark")){
		    	init_match(player, arg1, NOTYPE);
		    	match_everything();
		    	thing = match_result();

			if( thing >= 0 && thing < extract_top){
				if( extract_list[thing]== NOTHING){
					notify(player,"Not marked!");
					return;
				}
				extract_list[thing] = NOTHING;
				sprintf(msgbuf,"unmarked %d", thing);
				fnotify(player, msgbuf);
			}else {
				notify(player,"unmark what?");
			}
			return;
		}else if( !strcmp(combuf, "players")){
			mark_all_players(player);
			return;
		}else if( !strcmp(combuf, "def")){
			mark_default_stuff(player);
			return;
		}else if( !strcmp(combuf, "class")){
			init_match(player, arg1, NOTYPE);
			match_everything();
			thing = match_result();
			if( !bad_dbref(thing, 0) && Typeof(thing) == TYPE_ROOM && (Flags(thing)&INHERIT)){
				mark(thing);
				mark_all_in_class(thing);
			}
			return;
		}else if( !strcmp(combuf, "zone")){
			init_match(player, arg1, NOTYPE);
			match_everything();
			thing = match_result();
			if( !bad_dbref(thing, 0) && Typeof(thing) == TYPE_ROOM && (Flags(thing)&INHERIT)){
				mark(thing);
				mark_all_in_zone(thing);
			}
			return;
		}else {
			sprintf(msgbuf,"Unknown @extract command '%s'", combuf);
			notify(player, msgbuf);
		}
	}else {
	    	init_match(player, arg1, NOTYPE);
	    	match_everything();
	    	thing = match_result();

		if( !bad_dbref(thing, 0)){
			extract_list[thing] = thing;
			sprintf(msgbuf,"Marked %d for extraction", thing);
			fnotify(player, msgbuf);
		}else {
			notify(player,"extract what?");
		}
	}
}


