#include <stdio.h>

int main(argc, argv)
char **argv;
int argc;
{	FILE *ver;
	char verbuf[80], *p;
	char ch;
	char minor[80];

	strcpy(minor,"1");

	ver = fopen("version.h","r");
	if( !ver){
		goto output;
	}

	verbuf[0] = '\0';
	minor[0] = '\0';
	p = verbuf;
	while( fread(&ch, 1, 1, ver)){
		if( ch == '"')
			break;
	}
	if( ch == '"'){
	}else
		exit(1);
	while( fread(&ch, 1, 1, ver)){
		if( ch == ' ')
			break;
		*p++ = ch;
	}
	*p = '\0';
	p = minor;
	while( fread(&ch, 1, 1, ver)){
		*p++ = ch;
		if( ch == ' ')
			break;
	}
	*p = '\0';
	fclose(ver);

output:
	ver = fopen("version.h", "w");
	if( !ver)
		exit(1);
	if( argc > 1){
		fprintf(ver,"#define VERSION \"%s %03d %s\"\n", verbuf, atoi(minor)+1, argv[1]);
	}else {
		fprintf(ver,"#define VERSION \"%s %03d\"\n", verbuf, atoi(minor)+1);
	}
	fclose(ver);
	exit(0);
}
