#include <stdio.h>
#include <dos.h>

char buf[640];

#undef FP_SEG
#undef FP_OFF

unsigned FP_SEG(x)
char *x;
{	long y;

	y = (long)x;

	return (unsigned) (y >> 16);
}

unsigned FP_OFF(x)
char *x;
{	long y;

	y = (long)x;
	return (unsigned) (y & 0xffff);
}

main(argc, argv)
char **argv;
{
	fprintf(stderr,"%x %x\n", FP_SEG(buf), FP_OFF(buf));
}
