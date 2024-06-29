#include "copyrite.h"

#include <stdio.h>
#include <ctype.h>

#include "db.h"
#include "config.h"
#include "interfac.h"
#include "fight.h"


extern dbref euid;
#define DB_MSGLEN (MAX_COMMAND_LEN)

extern dbref euid;

/* 
   zoneof(a)
	returns the zone of the object 
 */

dbref zoneof(obj2)
dbref obj2;
{	dbref obj = obj2;
	int depth = 0;

	if( bad_dbref(obj, 0))
		return 0;

	while(Typeof(obj) != TYPE_ROOM){
		if(depth++ > 20){
fprintf(logfile,"Zoneof: loop obj %d\n", obj);
			return -1;
		}
		switch(Typeof(obj)){
		case TYPE_EXIT:
			return -1;
		case TYPE_PLAYER:
		case TYPE_THING:
			obj = Location(obj);
			if( bad_dbref(obj, 0))
				return -1;
			continue;
		default:
fprintf(logfile,"Bad in zoneof\n");
			return -1;
		}
	}
	return Zone(obj);
}

