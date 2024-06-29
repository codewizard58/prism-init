/* Prism-BSX interface. */
#include <stdio.h>
#include <stdlib.h>

int done;

char *bsxbuf;
char *bsxptr;
int   bsxcnt;
extern char eat_char();

int bsx_write(buf, cnt)
char *buf;
int cnt;
{	int n = cnt;
	char comm[4];

	if( !bsxbuf){
		bsxbuf = (char *)malloc(4096);
		if( !bsxbuf){
fprintf(stderr,"No memory for bsxbuf!\n");
			exit(1);
		}
	}
	while( cnt > 0){
		if( !bsxptr){
			bsxptr = bsxbuf;
			bsxcnt = 0;
		}
		switch(*buf){
		case '\n':
			buf++;
			*bsxptr++ = '\n';
			cnt--;
			*bsxptr = '\0';

			bsxptr = bsxbuf;

			if( bsxbuf[0] == '@'){
				comm[0] = bsxbuf[1];
				comm[1] = bsxbuf[2];
				comm[2] = bsxbuf[3];
				comm[3] = '\0';
				bsxptr = &bsxbuf[4];

				if( process_bsx_command(comm)){
					bsxptr = NULL;
					continue;
				}
			}
			add_string_to_text_window(bsxbuf);
			bsxptr = NULL;
			break;
		default:
			*bsxptr++ = *buf++;
			cnt--;
			break;
		}
	}
	return n;
}

int eat_hex()
{	int n = 0;
	char ch;

	ch = eat_char();
	if( ch == EOF){
fprintf(stderr,"eof in eat hex(1)\r");
		return 0;
	}
	if( ch >= '0' && ch <='9')
		n = ch & 0xf;
	else n = (ch+9 & 0xf);

	n = n << 4;
	ch = eat_char();
	if(ch == EOF){
fprintf(stderr,"eof in eat hex(2)\r");
		return 0;
	}
	if( ch >= '0' && ch <='9')
		n |= ch & 0xf;
	else n |= (ch+9 & 0xf);

	return n;
}

char eat_char()
{
	if( !bsxptr || !*bsxptr){
		return EOF;
	}
	return *bsxptr++;
}

char eat_raw_char()
{
	return eat_char();
}
