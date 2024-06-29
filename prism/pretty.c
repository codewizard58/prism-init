#include <stdio.h>
#ifdef MSDOS
#include <stdlib.h>
#endif

/* New prism pretty print for the DB */
/* Printf out each record on a line */

char buf[1024];
int cnt;
char *p;

char obuf[1024];
char *optr;

char msgbuf[2048];

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
	if( p[-1] == '\n')
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
char descbuf[1024];

main(argc, argv)
char **argv;
int argc;
{	FILE *inf;
	int i=0, j;
	int loc,next,contents,owner,flags, class;
	long pennies, age;
	int commands, exits;
	static char lockbuf[1024];
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

	while( fgets(buf, 1023, inf)){

		class = 0;
		age = now;

		p = buf;
		if( *p != '#'){
			continue;
		}
		p++;
		if( *p < '0' || *p > '9'){
			if( *p == 'v'){
				version++;
				continue;
			}
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
		i++;
		optr = obuf;

		if( !sfgets(namebuf, 1023, inf))
			break;
		if( !sfgets(descbuf, 1023, inf))
			break;

		loc = getint(inf);
		contents = getint(inf);
		exits=getint(inf);
		next= getint(inf);

		if( !sfgets(lockbuf, 1023, inf))
			break;

		if( !version){
			if( !fgets(buf, 1023, inf))
				break;
			if( !fgets(buf, 1023, inf))
				break;
			if( !fgets(buf, 1023, inf))
				break;
			if( !fgets(buf, 1023, inf))
				break;
		}
		owner=getint(inf);
		pennies=getlong(inf);
		flags=getint(inf);

		if( !fgets(buf, 1023, inf))
			break;

		ch = getc(inf);
		if( ch != '!' && ch != '#' && ch != '*'){
			ungetc(ch, inf);
			commands = getint(inf);
			ch = getc(inf);
		}

		if( ch != '!' && ch != '#' && ch != '*'){
			ungetc(ch, inf);
			class = getint(inf);
			ch = getc(inf);
		}
		if( ch == '!'){
			age = getlong(inf);
		}else
			ungetc(ch, inf);

/* put out special flags */
		sprintf(optr, "%5d ", j);
		while(*optr)optr++;

		sprintf(optr, "%-15.15s ", namebuf);
		while(*optr)optr++;

		if( !version && (flags&7)==2)
			contents = loc;
		if( (flags&7)==2 && contents == -1){
			*optr++ = 'U';
		}else if(ageof(age)>5 && (flags&7)==2){
			*optr++ = '!';
		}else *optr++ = ' ';

		fcnt = 0;
		switch( flags & 7){
		case 0: *optr++ = 'R'; break;
		case 1: *optr++ = ' '; break;
		case 2: *optr++ = 'E'; break;
		case 3: *optr++ = 'P'; break;
		default:*optr++ = 'X';
		}
		if( flags & 0x10){*optr++ = 'W'; fcnt++; }
		if( flags & 0x20){*optr++ = 'L'; fcnt++; }
		if( flags & 0x40){*optr++ = 'D'; fcnt++; }
		if( flags & 0x80){*optr++ = 'T'; fcnt++; }
		if( flags & 0x100){*optr++ = 'S'; fcnt++; }
		if( flags & 0x200){*optr++ = 'B'; fcnt++; }
		if( flags & 0x400){*optr++ = 'F'; fcnt++; }
		if( flags & 0x800){*optr++ = 'Q'; fcnt++; }
		if( flags & 0x1000){*optr++ = 'I'; fcnt++; }
		if( flags & 0x2000){*optr++ = 'A'; fcnt++; }
		if( flags & 0x4000){*optr++ = 'R'; fcnt++; }
		if( flags & 0x8000){*optr++ = 'C'; fcnt++; }
		if( flags & 0x8){*optr++ = '!'; fcnt++; }
		while(fcnt++ < 4)*optr++ = ' ';

		sprintf(optr, " %5d %5d  %5d %5d %5d :", class, owner, loc, contents, next);
		while( *optr)optr++;

		if(( flags&7 ) == 2)
			sprintf(optr, "%s", lockbuf);
		else
			sprintf(optr, "%s", descbuf);

		if( namebuf[0] == ' '&& namebuf[1] == ' '&&namebuf[2] == 'R'){
			if( lastrubbish)
				continue;
			lastrubbish = j;
		}else if( lastrubbish){
			if( lastrubbish+1 != j)
				fprintf(stdout,"  .....\n");
			lastrubbish = 0;
		}
		fprintf(stdout, "%s\n", obuf);
	}
}
