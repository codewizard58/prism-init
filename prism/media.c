/*  Multi-media extensions */
#include <stdlib.h>

#include "db.h"
#include "descript.h"

extern struct descriptor_data *descriptor_list;
static char *mbufp;
static int mdepth;
char media_dir[256];
extern char *Name0(), *Name1();


char *MediaName(obj)
dbref obj;
{	static char nbuf[12];
	int cnt;
	char *p, *q;

	cnt = 0;
	q = nbuf;
	p = Name0(obj);
	while(*p && cnt < 11){
		if( *p >= 'a' && *p <= 'z'){
			*q++ = *p;
			cnt++;
		}else if( *p >= 'A' && *p <= 'Z'){
			*q++ = *p;
			cnt++;
		}else if( *p >= '0' && *p <= '9'){
			*q++ = *p;
			cnt++;
		}
		p++;
	}
	*q = '\0';
	return nbuf;
}



int media_name_sub(obj, oname)
dbref obj;
char *oname;
{	dbref z;
	char *p;
	int cnt = 0;

	if( mdepth >= 5)
		return;
	mdepth++;
	z = Class(obj);

	if( !bad_dbref(z, 0)){
		media_name_sub(z, NULL);
	}
	*mbufp++ = '/';
	if( oname )
		p = oname;
	else
		p = Name0(obj);
	while(*p){
		int ch = *p;

		if( cnt > 7)
			break;
		if( ch >= 'a' && ch <= 'z'){
			cnt++;
			*mbufp++ = ch;
		}else if( ch >= 'A' && ch <= 'Z'){
			cnt++;
			*mbufp++ = ch+'a'-'A';
		}else if( ch >= '0' && ch <= '9'){
			cnt++;
			*mbufp++ = ch;
		}
		p++;
	}
	*mbufp = '\0';
	mdepth--;
	return 0;
}

char *media_name(object, oname)
dbref object;
char *oname;
{	static char *buf;
	char *p, *q;
	dbref n;

	if( buf == NULL){
		buf = malloc(257);
	}
	if( buf == NULL)
		return "";
	if( bad_dbref(object, 0)){
		return "";
	}
	*buf = '\0';
	mbufp = buf;
	media_name_sub(object, oname);
fprintf(logfile,"MEDIA NAME=%s\n", buf);
	return buf;
}

char mcurdir[257];
static char abuf[1024];

int find_file(fn, mode)
char *fn;
int mode;
{	static char fnbuf[12];
	int ret;
	char *p, *q;
	
	fnbuf[0] = 'B';
	p = &fn[1];
	q = &fnbuf[1];
	while(*q = *p++){
		if( *q == '.')
			break;
		q++;
	}
	*q = '\0';

	ret = open(fnbuf, mode);
	if( ret >= 0){
		return ret;
	}

	fnbuf[0] = 'b';
	ret = open(fnbuf, mode);
	if( ret >= 0){
		return ret;
	}

	*q++ = '.';
	*q++ = 'b';
	*q++ = 's';
	*q++ = 'x';
	*q = '\0';

	ret = open(fnbuf, mode);
	if( ret >= 0){
		return ret;
	}

	ret = open("Bdefault.bsx", mode);
	if( ret >= 0){
		return ret;
	}

	return ret;
}

/* spit back a bsx file */

int media_send_bsx(d, infd, cmd)
struct descriptor_data *d;
int infd;
char *cmd;
{	int cnt, i, cr;

	cr = 0;

	if( infd >= 0){
		queue_string(d, cmd);
		while( (cnt = read(infd, abuf, sizeof(abuf)) ) > 0){
			int pos = 0;
			for(i=0; i < cnt; i++){
				if( abuf[i] == 0x1a){
					cnt = i;
					continue;
				}
				if( abuf[i] == '\r')
					cr = 1;
				else if( abuf[i] == '\n'){
					if( cr != 1 ){
						if( i-pos > 0){
							queue_write(d, &abuf[pos], i-pos);
						}
						queue_write(d, "\r\n", 2);
						pos = i+1;
					}
					cr = 2;
				}else
					cr = 0;
			}
			if( pos < i){
				queue_write(d, &abuf[pos], cnt-pos);
			}
		}
		if( cr == 0){
			queue_write(d, "\r\n", 2);
		}
		queue_string(d, "@RFS\r\n");
		close(infd);
	}
	return 0;

}	


int media_send_file(d, ftyp, onam, fn, cmd)
struct descriptor_data *d;
char *ftyp, *onam, *fn, *cmd;
{	char buf[13];
	int cnt, infd;
	char *p = onam;
	char *q = fn, *r;
	int nodot = 0;
	int i;
	int cap = 0;

	if( media_dir[0] == '\0'){
		getcwd(media_dir, 255);
	}
	strcpy(mcurdir, media_dir);
	chdir(mcurdir);

	if( fn[0] == '\0'){
		if( chdir(ftyp)){
			fprintf(logfile,"Failed to chdir to %s\n", ftyp);
			return 1;
		}
		while(*q = *ftyp++)q++;
	
	/* now walk the database */
		while(1){
			cnt = 0;
			buf[0] = 'D';
			while(cnt < 8){
				cnt++;
				buf[cnt] = *p;
				if( *p == '\0' || *p == '/')
					break;
				p++;
			}
			while(*p != '\0' && *p != '/')p++;
			buf[cnt] = '\0';
	
			if( *p == '/'){
				if( chdir(buf)){
					buf[0] = 'd';
					if( chdir(buf)){
						strcpy(buf,"Bdefault");
						break;
					}
				}
				r = buf;
				*q++ = '/';
				while(*q = *r++)q++;
				cnt = 0;
				buf[0] = 'D';
			}else if( *p == '\0'){
				buf[cnt] = '\0';
				buf[0] = 'B';
				r = buf;
				break;
			}
			p++;
		}
	/* now find the file to send back */
		while(1){
			infd = find_file(buf, 0);
			if( infd >= 0){
				break;
			}
			while(q != fn && *q != '/')q--;
			if( q != fn ){
				*q = '\0';
				chdir(mcurdir);
				chdir(fn);
			}else break;
		}
		if( infd >= 0){
			r = buf;
			*q++ = '/';
			while(*q = *r++)q++;
		}
	}else{
		chdir(media_dir);
		infd = open(fn, 0);
		if( infd < 0){
			q = fn;
			while(*q){
				if( *q == '/')*q = '\\';
				q++;
			}
			while(q != fn && q[-1] != '\\')q--;
			if( q != fn){
				q[-1] = '\0';
				strncpy(buf, q, 8);
				buf[8] = '\0';
				chdir(fn);
				infd = find_file(buf, 0);
if( infd < 0){
fprintf(logfile,"Failed to re-open %s %s\n", fn, buf);
}
			}
		}
	}
	if( infd >= 0){
		cnt = read(infd, abuf, 4);
		while( cnt == 4 && abuf[0] == '#'){
			cap = 0;
			if( !strncmp(&abuf[1], "HIT", 3)){
				cap = CAP_HIT;
			}else if( !strncmp(&abuf[1], "URL", 3)){
				cap = CAP_MMEDIA;
			}else if( !strncmp(&abuf[1], "BSX", 3)){
				cap = CAP_BSX;
			}
				
			while(1){
				if( 1 != read(infd, abuf, 1))return 0;
				if( abuf[0] == '\n')break;
			}
			if( cap == CAP_BSX){
				abuf[0] = '#';
				break;
			}
			if(cap != 0){
				while(1){
					while(1){
						if( 1 != read(infd, abuf, 1))break;
						if( abuf[0] == '#')break;
						i = abuf[0];
						if( cap & CAP_HIT){
							strcpy(abuf,"@DHS");
						}else{
							strcpy(abuf,"@URL");
						}
						q = &abuf[4];
						p = &cmd[4];
						cnt = 4;
						while(*q = *p++){
							q++;
							cnt++;
						}
						abuf[cnt] = i;
						
						while(cnt < 1020){
							cnt++;
							if( 1 != read(infd, &abuf[cnt], 1))break;
							if( abuf[cnt] == '\n')break;
						}
						if( abuf[cnt-1] != '\r'){
							abuf[cnt++] = '\r';
							abuf[cnt] = '\n';
						}
						if( d->caps & cap){
							queue_write(d, abuf, cnt+1);
						}
					}
					if( abuf[0] == '#'){
						cnt = 1+read(infd, &abuf[1], 3);
						break;
					}
				}
			}else {
				abuf[4] = '\0';
				fprintf(logfile,"Unknown type '%s' in media file '%s'\n", abuf, buf);
				return 0;
			}
		}
		if( cnt != 4 || abuf[0] != '#'){
			close(infd);
			infd = find_file(buf, 0);
		}
	}
	media_send_bsx(d, infd, cmd);
	return 0;
}

char bfilename[257];
char mfilename[257];

static char showbuf[70];

int visualise( player, object, oname)
dbref player, object;
char *oname;
{	char *p;
	struct descriptor_data *d;
	char *root;

	if( bad_dbref(object, 0))
		return 1;

	p = media_name(object, oname);
		
	switch( Typeof(object)){
	case TYPE_PLAYER:
		sprintf(showbuf,"@DFO%.60s.", Name1(object));
		root = "d/dbsx/dplayer";
		break;
	case TYPE_THING:
		sprintf(showbuf,"@DFO%.60s.", Name1(object));
		root = "d/dbsx/dthing";
		break;
	default:
		sprintf(showbuf,"@DFS%.60s.", Name1(object));
		root = "d/dbsx/droom";
		break;
	}
	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0 && d->connected == 2){
			if( d->player == player && (d->caps & (CAP_BSX|CAP_HIT|CAP_MMEDIA))){
				media_send_file(d, root, p, bfilename, showbuf);
			}
		}
	}
	bfilename[0] = '\0';
	mfilename[0] = '\0';
	return 0;
}


int show(player,loc, object)
dbref loc, object, player;
{   struct descriptor_data *d;
	int t;
	char *p;
	long where;
	int x,y,z;
	int mflags = 0;
	char buf[64];
	
	showbuf[0] = '\0';
	if( !bad_dbref(object, 0)){
	
		p = showbuf;
		t = Typeof(object);
		if( t == TYPE_ROOM){
			sprintf(showbuf,"@SCE%.64s", Name1(object));
			while(*p){
				if( *p == '.')
					*p = '0';
				p++;
			}
			*p++ = '.';
			*p = '\0';
		}else if( t != TYPE_EXIT){
			sprintf(showbuf,"@VIO%.64s", Name1(object));
			while(*p){
				if( *p == '.')
					*p = '0';
				p++;
			}
			where = Position(object);
			x = where & 0x3ff;
			y = (where >> 10) & 0x3ff;
			z = (where >> 20) & 0x3ff;
			sprintf(p,".%02x%02x", 0xff&x, 0xff&y);
			while(*p){
				if(*p >= 'a' && *p <= 'f')
					*p -= 0x20;
				p++;
			}
		}
	}else if( object == -5){
	}else if( object == -4){
		sprintf(showbuf,"@SCEinventory.");
		p = showbuf;
		while(*p)p++;
	}
	if( showbuf[0]){
		*p++ = '\r';
		*p++ = '\n';
		*p = '\0';
	}else {
		return 0;
	}
	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0 && d->connected == 2){
			if( d->player != object && (d->caps & CAP_BSX) &&
				 (Location(d->player) == loc || d->player == player)){
				queue_string(d, showbuf);
				mflags |= 1;
			}
		}
	}
	return mflags;
}

int show_flush(player, loc, flags)
dbref loc, player;
int flags;
{   struct descriptor_data *d;

	if( flags == 0)
		return 0;

	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0){
			if( Location(d->player) == loc || d->player == player){
				if( (flags &1) && (d->caps & CAP_BSX)){
					queue_string(d,"@RFS\r\n");
				}
			}
		}
	}
	return 0;
}

int hide(player, loc, object)
dbref loc, object, player;
{	struct descriptor_data *d;
	int mflags = 0;

	sprintf(showbuf,"@RMO%.64s.\r\n", Name1(object));
	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0 && d->connected == 2){
			if( (d->caps & CAP_BSX) && (Location(d->player) == loc|| d->player == player )
					&& d->player != object){
				queue_string(d,showbuf);
				mflags |= 1;
			}
		}
	}
	return mflags;
}

define(player, object)
dbref player, object;
{
	return 0;
}

int media_remove(object, loc)
dbref object, loc;
{	struct descriptor_data *d;
	int t;
	char *p;
	int mflags = 0;

	if( Typeof(loc) != TYPE_ROOM)
		return 0;
	if( Typeof(object) == TYPE_EXIT )
		return 0;


	sprintf(showbuf,"@RMO%.64s.\r\n", Name1(object));
	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0 && d->connected == 2){
			if( (d->caps & CAP_BSX) &&
				Location(d->player) == loc){
				queue_string(d,showbuf);
				mflags |= 1;
			}
		}
	}
	return mflags;
}

int media_add(object, loc)
dbref object, loc;
{
	struct descriptor_data *d;
	int t;
	char *p;
	long where;
	int x,y,z;
	int mflags=0;

	if( Typeof(loc) != TYPE_ROOM)
		return 0;
	if( Typeof(object) == TYPE_EXIT)
		return 0;

	sprintf(showbuf,"@VIO%.64s", Name1(object));
	p = showbuf;
	while(*p){
		if( *p == '.')
			*p = '0';
		p++;
	}
	where = Position(object);
	x = where & 0x3ff;
	y = (where >> 10) & 0x3ff;
	z = (where >> 20) & 0x3ff;
	sprintf(p,".%02x%02x", 0xff&x, 0xff&y);
	while(*p){
		if(*p >= 'a' && *p <= 'f')
			*p -= 0x20;
		p++;
	}
	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	for (d = descriptor_list; d; d=d->next) {
		if(d->descriptor >= 0 && d->connected == 2){
			if( (d->caps & CAP_BSX) && d->player != object &&
				Location(d->player) == loc){
				queue_string(d,showbuf);
				mflags |= 1;
			}
		}
	}
	return mflags;
}


