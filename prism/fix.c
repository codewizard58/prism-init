#include <stdio.h>
#ifdef MSDOS
#include <stdlib.h>
#endif

/* New rism pretty print for the DB */
/* Printf out each record on a line */

char buf[4096];
int cnt;
char *p;

char obuf[4096];
char *optr;

char msgbuf[4096];

char *sfgets(buf, cnt, fd)
char *buf;
int cnt;
FILE *fd;
{	char *p;

	p = fgets(buf,cnt,fd);
	if( !p)
		return p;
	
	p = buf;

	while(*p)p++;
	if( p[-1] == '\n'){
		p[-1] = '\0';
		p--;
	}
	if( p[-1] == '\015')
		p[-1] = '\0';
	return buf;
}

static char *ptr;
	    
int getint(f)
FILE *f;
{	char *p, *p1;

	if( ptr == NULL){
		fgets(msgbuf, 2048, f);
		ptr = msgbuf;
	}
/* skip white space */
	p = ptr;
	while( *p == ' ' || *p == '\011')p++;
/* Look for ; seperator */
	p1 = p;
	while( *p1 && *p1 != ';')p1++;
	if( *p1 == ';'){	/* found it */
		*p1++ = '\0';
		ptr = p1;
	}else{
		ptr = NULL;	/* Not found, so need a new buffer next time*/
	}

	return((int )atoi(p));
}

long getlong(f)
FILE *f;
{	char *p, *p1;

	if( ptr == NULL){
		fgets(msgbuf, 2048, f);
		ptr = msgbuf;
	}
/* skip white space */
	p = ptr;
	while( *p == ' ' || *p == '\011')p++;
/* Look for ; seperator */
	p1 = p;
	while( *p1 && *p1 != ';')p1++;
	if( *p1 == ';'){	/* found it */
		*p1++ = '\0';
		ptr = p1;
	}else{
		ptr = NULL;	/* Not found, so need a new buffer next time*/
	}

    return(atol(p));
}

long now = 0l;

int ageof(age)
long age;
{	long t;

	if( age == 0l)
		return 12;
	t = now-age;
	t = t /3600l;
	t = t / 24;	/* days */
	t = t / 30;	/* months */
	return (int)t;
}

char namebuf[1024];
char descbuf[4096];
char lockbuf[4096];

main(argc, argv)
char **argv;
int argc;
{	FILE *inf;
	int i=0, j;
	int loc,next,contents,owner,flags, class;
	long pennies, age;
	int commands, exits;
	int version = 0, ch;
	char *p;
	int lastrubbish=0;
	int fcnt;

	time(&now);


	if( argc > 1){
		inf = fopen(argv[1], "r");
		if( !inf){
			perror("fopen");
			exit(1);
		}
	}else
		inf = stdin;

	while( sfgets(buf, 1023, inf)){

		class = 0;
		age = now;

		p = buf;
		if( *p != '#'){
fprintf(stderr,"Skipping1 %s\n", buf);
			continue;
		}
		p++;
		if( *p < '0' || *p > '9'){
printf("%s\n", buf);
			if( *p == 'v'){
fprintf(stderr,"Has a version number!\n");
				version++;
				continue;
			}
fprintf(stderr,"Skipping2 %s\n", buf);
			continue;
		}

		j = 0;
		while( *p >= '0' && *p <= '9'){
			j = j*10;
			j = j + *p -'0';
			p++;
		}	

		if( i != j){
fprintf(stderr, "Expecting record %d found %d\n", i, j);
			continue;
		}
printf("#%d\n",j);
		i++;
		optr = obuf;

		if( !sfgets(namebuf, 1023, inf))
			break;
		if( !sfgets(descbuf, 4090, inf))
			break;
printf("%s\n", namebuf);
printf("%s\n", descbuf);

		do {
			ch = getc(inf);
		}while (ch != EOF && ch != '-' && !(ch >= '0' && ch <= '9'));
		ungetc(ch,inf);

		ptr = NULL;		/* Make sure its clean */

		loc = getint(inf);
		contents = getint(inf);
		exits=getint(inf);
		next= getint(inf);
printf("%d;%d;%d;%d\n",loc, contents,exits,next);

		if( !sfgets(lockbuf, 4090, inf))
			break;
printf("%s\n", lockbuf);

		if( !version){
			if( !sfgets(buf, 1023, inf))
				break;
printf("%s\n", buf);
			if( !sfgets(buf, 1023, inf))
				break;
printf("%s\n", buf);
			if( !sfgets(buf, 1023, inf))
				break;
printf("%s\n", buf);
			if( !sfgets(buf, 1023, inf))
				break;
printf("%s\n", buf);
		}
		owner=getint(inf);
		pennies=getlong(inf);
		flags=getint(inf);
printf("%d;%ld;%d\n",owner, pennies, flags);

		if( !sfgets(buf, 1023, inf))
			break;
printf("%s\n", buf);

		ch = getc(inf);
		while(ch != EOF && ch != '#'){
			ungetc(ch,inf);
			if( !sfgets(buf, 1023, inf))
				break;
			printf("%s\n", buf);
			ch = getc(inf);
		}
		ungetc(ch,inf);
	}
}
