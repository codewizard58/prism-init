/* Boolexp specific stuff */

/* #define VM_MSGS */

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_GROUP 4
#define BOOLEXP_FUNC  5
/* And named functions */
#define BOOLEXP_AGE	6
#define BOOLEXP_LEVEL	7
#define BOOLEXP_HAS	8
#define BOOLEXP_DROP	9
#define BOOLEXP_TEL	10
#define BOOLEXP_FLAGS	11
#define BOOLEXP_EXEC	12
#define BOOLEXP_PAY	13
#define BOOLEXP_HEAL	14
#define BOOLEXP_EXECL	15
#define BOOLEXP_SUCC	16
#define BOOLEXP_OSUCC	17
/* following added 30-5-91 */
#define BOOLEXP_PROMOTE 18
#define BOOLEXP_EXP	19
#define BOOLEXP_STR	20
#define BOOLEXP_INT	21
#define BOOLEXP_CHR	22
#define BOOLEXP_CLASS   23
/* added 3-8-91 */
#define BOOLEXP_FORTH	24
#define BOOLEXP_FALSE	25
#define BOOLEXP_PUSH	26	/* 23/9/91 */
/* 4-1-92 */
#define BOOLEXP_FAIL	27
#define BOOLEXP_EMPTY	28
#define BOOLEXP_SEND	29
#define BOOLEXP_SSEND	30
/* 11-1-92 */
#define BOOLEXP_TRUE	31
/* 24-1-92 */
#define BOOLEXP_REWARD  32
#define BOOLEXP_WOUND   33
/* 9-2-92 */
#define BOOLEXP_NRSEND	34
#define BOOLEXP_RND	35
#define BOOLEXP_OFAIL   36
/* BSX and multimedia extensions */
#define BOOLEXP_BSUCC	37
#define BOOLEXP_BOSUCC	38
#define BOOLEXP_SHOW	39
#define BOOLEXP_HIDE	40
#define BOOLEXP_DEF		41

#define BOOLEXP_POS		42	/* 19/10/95 */
#define BOOLEXP_WITHIN	43

struct bool_rec {
const char *name;
int   type;
int   fmt;		/* print type, see unparse.c */
const char *help;
};

#ifdef BTABLE

struct bool_rec booltable[] =
{	{"&",		BOOLEXP_AND,	0,	"exp1 & exp2       . True if exp1 and exp2 is true."},
	{"|",		BOOLEXP_OR,	0,	"exp1 | exp2       . True if exp1 or exp2 is true."},
	{"!",		BOOLEXP_NOT,	0,	"! exp             . True if exp is FALSE."},
	{"@",		BOOLEXP_GROUP,	0,	"@obj              . True if lock on obj is true."},
	{"AGE",		BOOLEXP_AGE,	0,	"AGE{secs,obj}     . True if obj is less than secs old."},
	{"EXEC",	BOOLEXP_EXEC,	2,	"EXEC{\"cmds\",auth} . Execute cmds quietly with euid set to auth."},
	{"EXECL",	BOOLEXP_EXECL,	2,	"EXECL{\"cmds\",auth}. Execute cmds with euid set to auth."},
	{"FAIL",	BOOLEXP_FAIL,	2,	"FAIL{\"msg\",target}. Like SUCC but returns false."},	/* fail msg{fail, ofail} */
	{"FALSE",	BOOLEXP_FALSE,	1,	"FALSE{0}          . Always false."},
	{"FLAGS",	BOOLEXP_FLAGS,	0,	"FLAGS{flag,obj}   . True if bit flag is set in obj's flags"},
	{"HAS",		BOOLEXP_HAS,	0,	"HAS{loc,obj}      . True if loc contains obj"},
	{"HEAL",	BOOLEXP_HEAL,	0,	"HEAL{amount}      . Heal player, kill a bit if amount is negative."},
	{"LEVEL",	BOOLEXP_LEVEL,	1,	"LEVEL{lev}        . True if players level is <= lev."},
	{"MOVE",	BOOLEXP_TEL,	0,	"MOVE{obj,loc}     . Move obj to loc."},
	{"OFAIL",	BOOLEXP_OFAIL,	2,	"OFAIL{\"omsg\"}     . Tell others in room omsg, return false."},
	{"OSUCC",	BOOLEXP_OSUCC,	2,	"OSUCC{\"omsg\"}     . Tell others in room omsg, return true."},
	{"PAY",		BOOLEXP_PAY,	0,	"PAY{amount}       . True if player can pay amount"},
	{"PROMOTE",	BOOLEXP_PROMOTE,1,	"PROMOTE{n}        . Promote player to level n+1 if they are at level n"},
	{"PUSH",	BOOLEXP_PUSH,	2,	"PUSH{\"cmd\"}       . Push cmd onto players input queue."},
	{"REWARD",	BOOLEXP_REWARD,	0,	"REWARD{amount}    . Give player amount."},
	{"RND",		BOOLEXP_RND,	0,	"RND{i,j}          . True i in j times."},
	{"SEND",	BOOLEXP_SEND,	2,	"SEND{\"msg\",auth}  . Send msg to receiver (self)."},	/* Send message to object */
	{"SSEND",	BOOLEXP_SSEND,	2,	"SSEND{\"msg\",auth} . Send to super class of receiver (super)."},	/* Super send */
	{"SUCC",	BOOLEXP_SUCC,	2,	"SUCC{\"msg\",target}. Tell target msg, player if target not specified. Always true."},
	{"TEL",		BOOLEXP_TEL,	0,	"TEL{obj,loc}      . As for move."},
	{"TRUE",	BOOLEXP_TRUE,	0,	"TRUE{ }           . Always true."},
	{"WOUND",	BOOLEXP_WOUND,	0,	"WOUND{amount}     . Opposite of HEAL."},
	{"SHOW",	BOOLEXP_SHOW,	2,	"SHOW{#obj,arg}    . Show multi-media object"},
	{"HIDE",	BOOLEXP_HIDE,	2,	"HIDE{#obj,arg}    . Hide multi-media object"},
	{"DEF",		BOOLEXP_DEF,	2,	"DEF{#obj,arg}     . Define multi-media object"},
#ifdef BOOLEXP_FUNC
	{"$",		BOOLEXP_FUNC,	0,	"$obj              . Obsolete version of EXEC{}."},
#endif
	{"DROP",	BOOLEXP_DROP,	1,	"DROP{0}           . Obsolete."},	/* Obsoleted */
#ifdef THREAD
	{"FORTH",	BOOLEXP_FORTH,	2,	""},
#endif
	{"BSUCC",	BOOLEXP_BSUCC,	2,	"BSUCC{\"msg\",target} . Tell target msg if BSX capable. Always true."},
	{"BOSUCC",	BOOLEXP_BOSUCC,	2,	"BOSUCC{\"msg\",target}. Tell others msg if BSX capable. Always true."},
	{"CLASS",	BOOLEXP_CLASS,	0,	"CLASS{class,obj}  ."},
	{"POS",		BOOLEXP_POS,	2,	"POS{sub,add}      . Change position, x=x-sub+add"},
	{"WITHIN",	BOOLEXP_WITHIN,	2,	"WITHIN{x,y}       . True if receiver within box, x y define corners."},
	{"EMPTY",	BOOLEXP_EMPTY,	1,	"EMPTY{obj}        . Not Implemented."},
	{"",	0,	-1}
};
#endif

/* New boolexp record structure */

union new_boolobj {
struct n_boolexp *p;
long l;
dbref d;
int i;
char *s;
};

struct n_boolexp {
union new_boolobj u1,u2;
char n_func;
unsigned char n_type;
};

#define NT_UNKN 0
#define NT_ARG 1
#define NT_DBREF 2
#define NT_INT 3
#define NT_STR 4
#define NT_ME 5
#define NT_HERE 6
#define NT_SENDER 7
#define NT_PTR 8
#define NT_RECEIVER 9
#define NT_LOC 10

#define NT_T1(x) ( (vm_bool(x)->n_type)&0xf)
#define NT_T2(x) ( ( (vm_bool(x)->n_type)>>4)&0xf)

extern BOOLEXP_PTR PTR1( PROTO(BOOLEXP_PTR) );
extern BOOLEXP_PTR PTR2( PROTO(BOOLEXP_PTR) );
extern dbref OBJ1( PROTO(BOOLEXP_PTR) );
extern dbref OBJ2( PROTO(BOOLEXP_PTR) );
extern int ARG1( PROTO(BOOLEXP_PTR) );
extern int ARG2( PROTO(BOOLEXP_PTR) );
extern long VAL1( PROTO(BOOLEXP_PTR) );
extern long VAL2( PROTO(BOOLEXP_PTR) );
extern char * STR1( PROTO(BOOLEXP_PTR) );
extern char * STR2( PROTO(BOOLEXP_PTR) );


extern BOOLEXP_PTR * W_PTR1( PROTO(BOOLEXP_PTR ));
extern BOOLEXP_PTR * W_PTR2( PROTO(BOOLEXP_PTR ));
extern dbref *W_OBJ1( PROTO(BOOLEXP_PTR ));
extern dbref *W_OBJ2( PROTO(BOOLEXP_PTR ));
extern long *W_VAL1( PROTO(BOOLEXP_PTR ));
extern long *W_VAL2( PROTO(BOOLEXP_PTR ));

extern void W_STR1( PROTO2(BOOLEXP_PTR , char *));
extern void W_STR2( PROTO2(BOOLEXP_PTR , char *));
extern void W_STR( PROTO3(BOOLEXP_PTR , int, char *));

extern int *W_ARG1( PROTO(BOOLEXP_PTR) );
extern int *W_ARG2( PROTO(BOOLEXP_PTR ));

extern BOOLEXP_PTR * W_PTR( PROTO2(BOOLEXP_PTR , int));
extern dbref *W_OBJ( PROTO2(BOOLEXP_PTR , int));
extern long *W_VAL( PROTO2(BOOLEXP_PTR , int));
extern int *W_ARG( PROTO2(BOOLEXP_PTR , int));

extern succ_fail_form( PROTO3(BOOLEXP_PTR , BOOLEXP_PTR *, BOOLEXP_PTR *));
extern BOOLEXP_PTR new_boolexp();
extern void bool_free( PROTO(BOOLEXP_PTR));
extern int last_result;

