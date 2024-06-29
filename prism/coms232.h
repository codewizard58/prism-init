/* The following rename the .asm routines so that MSC 6.0 can call them
 * Lattice does not have any problems.
 */
#ifdef MSC
#define chaval  CHAVAL
#define serini	SERINI
#define serget  SERGET
#define serrst  SERRST
#define serput  SERPUT
#define setspd	SETSPD
#endif

extern int rs232_init(int);
extern int rs232_check();
extern int rs232_open(PROTO2(int, struct netinfo *));
extern int rs232_close(PROTO2(int, struct netinfo *));
extern int rs232_read(PROTO4(int, struct netinfo *, char *, int));
extern int rs232_write(PROTO4(int, struct netinfo *, char *, int));
extern int rs232_select(PROTO2( int *, int *));
extern int rs232_shut(void);
extern char *rs232_name(PROTO2(int, struct netinfo *));

