/*	forth.h by P.J.Churchyard
	Copyright (c) july 1991
	All rights reserved
*/
#ifdef MSDOS
#ifndef const
#define const /* */
#endif
#else
#endif

struct dict {
unsigned char	nfa;		/* flag and length */
const char	*name;
void		(*cfa)();
struct dict 	*next;
struct dict 	**pfa;
};

typedef union datum
{	char ch;
	int  i;
	long l;
	char *ptr;
}DATUM;

#define FIRST *dp
#define SECOND dp[1]
#define THIRD dp[2]

#define PUSH_PTR(x) (--dp)->ptr = x
#define PUSH_INT(x) (--dp)->l = x
#define PUSH_CHAR(x) (--dp)->l = x
#define PUSH_LONG(x) (--dp)->l = x
#define PUSH_SHORT(x) (--dp)->l = x

/* Common used dict entries */
extern struct dict d_drop,d_dot,d_dotx,d_dotlx,d_dotch;
extern struct dict d_lit,d_dup,d_swap,d_over,d_rot;
extern struct dict d_plus,d_minus,d_times,d_divide;
extern struct dict d_and,d_or,d_xor,d_not;
extern struct dict d_at,d_atb,d_atp,d_store,d_storep,d_storeb;
extern struct dict d_if,d_goto,d_skip;
extern struct dict d_query,d_word,d_find,d_scan;
extern struct dict d_qwf,d_state,d_error,d_interp;
extern struct dict d_number,d_execute;
extern struct dict d_comma,d_commab,d_commap;
extern struct dict d_stkchk;
extern struct dict d_d_name,d_d_func,d_d_next,d_d_flag,d_d_thread;
extern struct dict d_fopen,d_fclose,d_putc,d_getc,d_fprintf,d_sprintf;
extern struct dict d_inptr;
extern struct dict d_infile,d_stdout,d_stdin,d_infunc,d_outfunc;
extern struct dict d_malloc,d_free,d_dots;
extern struct dict d_call,d_doit;

#define DICT(a,b,c,d,e,f) extern void b(); \
struct dict a = {e,f,b,c,d};

#define DICTT(a,b,c,d,e,f) struct dict a = {e,f,b,c,d};

