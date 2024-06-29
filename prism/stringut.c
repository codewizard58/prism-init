#include "copyrite.h"
#include "proto.h"

/* String utilities */

#include <ctype.h>

char translate_table[256];

void init_translate_table()
{	int ch;

	for(ch = 0; ch < 256 ; ch++)
		translate_table[ch] = ch & 0x7f;

	for(ch = 'A'; ch <= 'Z'; ch++){
		translate_table[ch] = ch+0x20;
		translate_table[ch+128] = ch+0x20;
	}
}

#define DOWNCASE(x) (translate_table[ x & 0xff])

int string_compare(s1, s2)
const char *s1,*s2;
{
    while(*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2)) s1++, s2++;

    return(DOWNCASE(*s1) - DOWNCASE(*s2));
}

int string_prefix(string, prefix)
const char *string, *prefix;
{
    while(*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
	string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
int string_match(src, sub)
const char *src, *sub;
{	char *s_sub = sub;

    if(*sub != '\0') {
	while(*src) {
	    while(*src && *sub && DOWNCASE(*src) == DOWNCASE(*sub)){
		src++;
		sub++;
	    }
	    if( !*sub)
		return 1;	/* found it */
	    sub = s_sub;
	    /* else scan to beginning of next word */
	    while(*src && isalnum(*src)) src++;
	    while(*src && !isalnum(*src)) src++;
	}
    }

    return 0;
}

