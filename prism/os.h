/* os.h
 * By P.J.Churchyard
 */

#ifndef MSDOS
#ifdef OLD_DEF
#define mud_write(a,b,c) write(a,b,c)
#define mud_read(a,b,c) read(a,b,c)
#define mud_close(a) close(a)
#define mud_select(a,b,c,d,e) select(a, b, c, d, e)
#endif

#define noecho(x)
#define echo(x)

#ifdef HPUX
#define random(x) rand(x)
#define srandom(x) srand(x)
#endif

#endif

