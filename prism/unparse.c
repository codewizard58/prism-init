#include "db.h"

#include "config.h"
#include "interfac.h"
#include "boolexp.h"
#include "externs.h"
#include "match.h"

#ifdef EXTRACT
extern int extracting;
extern dbref *extract_list;
#endif

extern char *requote();
static int db_flag = 0;		/* set if outputting for db.c */
extern char *Name0( PROTO(dbref));

static const char *unparse_flags( thing)
dbref thing;
{
    static char buf[32];
    char *p;
    const char *type_codes = "R-EP";
    int flg;

    p = buf;
    if(Typeof(thing) != TYPE_THING) *p++ = type_codes[Typeof(thing)];
    if((flg = Flags(thing)) & ~TYPE_MASK) {
	/* print flags */
	if(flg & WIZARD) *p++ = 'W';
	if(flg & STICKY) *p++ = 'S';
	if(flg & DARK) *p++ = 'D';
	if(flg & LINK_OK) *p++ = 'L';
	if(flg & TEMPLE) *p++ = 'T';
#ifdef RESTRICTED_BUILDING
	if(flg & BUILDER) *p++ = 'B';
#endif /* RESTRICTED_BUILDING */
	if(flg & FEMALE) *p++ = 'F';
#ifdef NO_QUIT
	if(flg & NO_QUIT)*p++ = 'Q';
	if(flg & INHERIT)*p++ = 'I';
#endif
#ifdef WEAPON
	if(flg & WEAPON)*p++ = 'A';		/* Armourment */
#endif
#ifdef ROBOT
	if(flg & ROBOT)*p++ = 'O';
#endif
#ifdef CONTAINER
	if(flg & CONTAINER)*p++='C';
#endif
    }
    *p = '\0';
    return buf;
}

char *def_names[] = {
	"DEF_THING(#-4)",
	"DEF_PLAYER(#-3)",
	"DEF_ROOM(#-2)"
};


const char *unparse_object( player, loc)
dbref player, loc;
{

	static char *buf;

    if( !buf){
	buf = (char *)malloc(BUFFER_LEN+128);
    }

    buf[0] = '\0';
    if( loc < -4 || loc >= db_top){
	strcpy(buf, "*Unknown*");
	return buf;
    }

    switch(loc) {
      case NOTHING:
	if(db_flag)
		strcpy(buf, "-1");
	else
		strcpy(buf,"*NOTHING*");
	break;
      case DEF_PLAYER:
      case DEF_ROOM:
      case DEF_THING:
	if( db_flag){
		sprintf(buf,"%d", loc);
		break;
	}
	strcpy(buf, def_names[loc+4]);
	break;

      default:
	if(db_flag){
#ifdef EXTRACT
		if(extracting){
			loc = extract_list[loc];
			if( loc == NOTHING)
				loc = -5;
		}
#endif
		sprintf(buf,"#%d", loc);
	}else	if(controls(player, loc) || can_link_to(player, loc)) {
	    /* show everything */
	    sprintf(buf, "%.60s(#%d %s)", Name(loc), loc, unparse_flags(loc));
	} else {
	    /* show only the name */
	    sprintf(buf, "%.60s", Name0(loc) );
	}
    }
    return buf;
}

static char *boolexp_buf;
static char *buftop;

void bstrcpy(buf, msg)
char *buf, *msg;
{	char *limit = &boolexp_buf[BUFFER_LEN+120];

	if( !buf || !msg){
fprintf(stderr,"NULL argument in bstrcpy\n");
		return;
	}
	while( buf != limit && (*buf++ = *msg++)); 

	return;
}


void unparse_bool_type(b, which)
BOOLEXP_PTR b;
int which;
{	int t,i;
	char *s;
	dbref d;
	long l;

	if(which)
		t = NT_T2(b);
	else
		t = NT_T1(b);

	switch(t){
	case NT_ME:
		bstrcpy(buftop,"ME");
		break;

	case NT_HERE:
		bstrcpy(buftop,"HERE");
		break;

	case NT_SENDER:
		bstrcpy(buftop,"SENDER");
		break;

	case NT_RECEIVER:
		bstrcpy(buftop,"SELF");
		break;

	case NT_STR:
		if(which)
			s = STR2(b);
		else
			s = STR1(b);
		*buftop++ = '"';
		bstrcpy(buftop, requote(s));
		while(*buftop)buftop++;
		*buftop++ = '"';
		*buftop = '\0';
		break;

	case NT_ARG:
		if(which)
			i = ARG2(b);
		else
			i = ARG1(b);
		sprintf(buftop,"$%d", i);
		break;

	case NT_INT:
		if(which)
			l = VAL2(b);
		else
			l = VAL1(b);
		sprintf(buftop,"%ld", l);
		break;

	case NT_DBREF:
		if(which)
			d = OBJ2(b);
		else
			d = OBJ1(b);
#ifdef EXTRACT
		if(extracting){
			d = extract_list[d];
			if( d == NOTHING)
				d = -4;
		}
#endif
		sprintf(buftop,"#%d", d);
		break;

	case NT_UNKN:
		sprintf(buftop," ");
		break;

	case NT_PTR:
		sprintf(buftop,"*PTR*");
		break;

	case NT_LOC:
		sprintf(buftop, "LOC");
		break;


	default:
		sprintf(buftop,"UNKN TYPE %d", t);
		break;
	}
	while(*buftop)buftop++;
}

int binary_op(t)
int t;
{
	if( t == BOOLEXP_AND || t == BOOLEXP_OR)
		return 1;
	else
		return 0;
}

void unparse_boolexp1(player, b, outer_type)
dbref player;
BOOLEXP_PTR b;
int outer_type;
{   char *s;
    int fmt;
    BOOLEXP_PTR b1;

    if(b == TRUE_BOOLEXP) {
if(db_flag){
fprintf(stderr,"unparse(%d, %lx, %d)\n", player, b, outer_type);
	return;
}
	bstrcpy(buftop, "*UNLOCKED*");
	buftop += strlen(buftop);
    } else {
	switch(vm_bool(b)->n_func) {
	  case BOOLEXP_AND:
	  case BOOLEXP_OR:
		b1 = PTR1(b);
if( b1 == TRUE_BOOLEXP){
fprintf(stderr,"b1 == TRUE in unparse boolexp.\n");
	bstrcpy(buftop, "TRUE{}");
	buftop += strlen(buftop);
	break;
}
		if(binary_op(vm_bool(b1)->n_func) && vm_bool(b1)->n_func != vm_bool(b)->n_func){
			*buftop++ = '(';
			unparse_boolexp1(player, b1, vm_bool(b)->n_func);
			*buftop++ = ')';
		}else
			unparse_boolexp1(player, b1, vm_bool(b)->n_func);

		if( vm_bool(b)->n_func == BOOLEXP_AND)
			*buftop++ = '&';
		else 	*buftop++ = '|';
		unparse_boolexp1(player, PTR2(b), vm_bool(b)->n_func);
		break;

	  case BOOLEXP_NOT:
	    *buftop++ = '!';
	    b1 = PTR1(b);
	    if( binary_op(vm_bool(b1)->n_func)){
		*buftop++ = '(';
		unparse_boolexp1(player, b1, vm_bool(b)->n_func);
		*buftop++ = ')';
	    }else
		unparse_boolexp1(player, b1, vm_bool(b)->n_func);
	    break;

	  case BOOLEXP_CONST:
		switch(NT_T1(b)){
		case NT_DBREF:
			bstrcpy(buftop, unparse_object(player, OBJ1(b)));
			buftop += strlen(buftop);
			break;
		default:
			unparse_bool_type(b, 0);
			break;
		}
	    	break;

	  case BOOLEXP_GROUP:
		*buftop++ = ' ';
		*buftop++ = '@';
		bstrcpy(buftop, unparse_object(player, OBJ1(b)));
		buftop += strlen(buftop);
		break;
#ifdef Notdef
	  case BOOLEXP_FUNC:
		*buftop++ = '$';
		*buftop++ = ' ';
		bstrcpy(buftop, unparse_object(player, OBJ1(b)));
		buftop += strlen(buftop);
		break;
#endif

	  default:
		boolexp_info(b, &s, &fmt);
		sprintf(buftop, "%s{", s);
		while(*buftop)buftop++;

		unparse_bool_type(b, 0);
#ifdef Notdef
		if( !fmt || fmt == 2){
#else
		{
#endif
			*buftop++ = ',';
			*buftop = '\0';
			unparse_bool_type(b, 1);
		}
		*buftop++ = '}';
		*buftop = '\0';
	}
    }
}
	    
const char *unparse_boolexp( player, b)
dbref player;
BOOLEXP_PTR b;
{
	if( !boolexp_buf)
		boolexp_buf = (char *)malloc(BUFFER_LEN+128);

    db_flag = 0;
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST);	/* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}
	    
const char *db_unparse_boolexp( player, b)
dbref player;
BOOLEXP_PTR b;
{
	if( !boolexp_buf)
		boolexp_buf = (char *)malloc(BUFFER_LEN+128);
	if( !boolexp_buf){
fprintf(stderr,"Help, no memory in db_unparse_boolexp\n");
		return "No memory";
	}

    db_flag = 1;
    buftop = boolexp_buf;
    if( b != TRUE_BOOLEXP){
	*buftop++ = '(';
    	*buftop = '\0';
    	unparse_boolexp1(player, b, BOOLEXP_CONST);	/* no outer type */
    	*buftop++ = ')';
    }
    *buftop++ = '\0';

    db_flag = 0;
    return boolexp_buf;
}

void putboolexp(f, b, i)
FILE *f;
BOOLEXP_PTR b;
dbref i;
{	char *s;

	s = db_unparse_boolexp(1, b);
	fprintf(f, "%s\n", s);
}
	
