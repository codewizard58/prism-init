#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

extern int errno;

#define TMP "TEMP.c"

char buf[10000];
int cnt, n;
char *p, *p1;

int main(argc, argv)
char **argv;
{
	int outf;

	while(argc){

		if(argc > 1){
			close(0);
			outf = open(argv[1], O_RDONLY,0);
			if( outf < 0){
	fprintf(stderr,"Could not open %s\n", argv[1]);
				exit(1);
			}
			close(1);
			outf = open(TMP, O_RDWR|O_CREAT|O_TRUNC,0666);
			if( outf < 0){
	fprintf(stderr,"Could not open %s\n", TMP);
				exit(1);
			}
fprintf(stderr,"Converting %s\n", argv[1]);
	
		}
		while( (cnt = read(0,buf,10000))){
			p = buf;
			p1= buf;
			n = 0;
			while( p!= &buf[cnt]){
				if(*p == '\r' || *p == '\032'){
					if(n)
						write(1, p1, n);
					n = 0;
					p++;
					p1 = p;
				}else {
					p++;
					n++;
				}
			}
			if(n)
				write(1, p1, n);
		}
		if( argc > 1){
			close(0);
			close(1);
			unlink(argv[1]);
			if( 0 > link(TMP, argv[1]) ){
				fprintf(stderr,"link (TEST.c,%s ) failed!\n", argv[1]);
				perror("dos2unix");
				exit(1);
			}
			unlink(TMP);
		}
		if(argc == 2)
			argc--;
		argc--;
		argv++;
	}
	return 0;
}
