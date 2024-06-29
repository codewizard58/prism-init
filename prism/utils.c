#include "copyrite.h"

#include "db.h"


/* remove the first occurence of what in list headed by first */
dbref remove_first( first, what)
dbref first, what;
{
    dbref prev;
    int depth = 0;

    /* special case if it's the first one */
    if( bad_dbref(first, 0)){
	return NOTHING;
    }
    if(first == what) {
	return Next(first);
    } else {
	/* have to find it */
	DOLIST(prev, first) {
	    if(bad_dbref(Next(prev), 0)){
		Next(prev) = NOTHING;
		return first;
	    }
	    if( depth++ > 10000){
		fprintf(stderr,"Depth loop in remove_first(%d,%d)\n", first, what);
		Next(prev) = NOTHING;	/* break the chain */
		return NOTHING;
	    }
	    if(Next(prev) == what) {
		Next(prev) = Next(what);
		return first;
	    }
	}
	return first;
    }
}

int member( thing, list)
dbref thing, list;
{   int depth = 0;

    if( list < 0 || list >= db_top)
	return 0;

    for(; list >= 0 && list < db_top; list = Next(list)) {
	if(list == thing) return 1;
	if( depth++ > 10000){
fprintf(stderr,"Depth loop in member(%d, %d)\n", thing, list);
		break;
	}
    }

    return 0;
}

dbref reverse( list)
dbref list;
{
    dbref newlist;
    dbref rest;

    if( list < 0 || list >= db_top)
	return NOTHING;

    newlist = NOTHING;
    while(list != NOTHING) {
	rest = Next(list);
	PUSH(list, newlist);
	list = rest;
    }
    return newlist;
}
