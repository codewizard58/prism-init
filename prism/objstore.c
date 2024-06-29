/* Object store *
 * By P,J,Churchyard
 * Copyright (c) 1991
 */

#include "db.h"
#include <fcntl.h>

#ifdef MSDOS
#define CACHE_SIZE 15
#else
#define CACHE_SIZE 31
#endif

/* The purpose of this module is to provide a means of using
 * disk to store both fixed length and variable length objects
 */
#include "boolexp.h"

char *desc_name = "mvax.dsc";
FILE *descf;

#ifdef MSDOS
long descpos;
long desc_got;	/* count the bytes read in */
typedef long filepos;
#else
int descpos;	/* current descriptor top */
int desc_got;
typedef int filepos;
#endif

struct vm_rec {
dbref me;
char data;
};

struct vm_cache {
struct vm_cache *next, *prev;
dbref	this;
dbref	indx;		/* Current index in the cache array */
struct vm_rec *main;
struct vm_rec *shadow;
};

struct vm_info_rec {
int Ncache;
long Size;		/* size of record */
struct vm_cache *Mru;	/* Most recently used list */
filepos Vmpos;
FILE *Vm_file;
char *Vm_name;
struct vm_cache *Cache[CACHE_SIZE];
} *vm_info;

#define cache (vm_info->Cache)
#define mru   (vm_info->Mru)
#define vmpos (vm_info->Vmpos)
#define vm_name (vm_info->Vm_name)
#define vm_file (vm_info->Vm_file)
#define vm_size (vm_info->Size)
#define ncache  (vm_info->Ncache)

#ifdef VM_DB
struct vm_info_rec vm_db_info =
{0, Sizeof(int)+Sizeof(union any_object),  NULL, 0l, NULL,"filename.dbm",{NULL,}};
#endif

#ifdef VM_BOOL
struct vm_info_rec vm_bool_info =
{0, Sizeof(int)+Sizeof(BOOLEXP), NULL, 0l, NULL,"filename.bol", {NULL,}};
#endif


void init_vm_names(nam)
char *nam;
{
	char nbuf[256];

	sprintf(nbuf,"%s.dsc",nam);
	desc_name = alloc_string(nbuf);

#ifdef VM_DB
	vm_info = &vm_db_info;
	sprintf(nbuf,"%s.dbm",nam);
	vm_name = alloc_string(nbuf);
#endif

#ifdef VM_BOOL
	vm_info = &vm_bool_info;
	sprintf(nbuf,"%s.bol",nam);
	vm_name = alloc_string(nbuf);
#endif

}

#ifdef VM_DESC
desctype alloc_desc(str)
char *str;
{
    char *s;
    int n;

    /* NULL, "" -> NULL */
    if(str == NULL || *str == '\0') return (desctype)NULL;

    n = strlen(str);

    if(!descf){	/* open the file */
#ifdef MSDOS
	descf = fopen(desc_name,"wb+");
#else
	descf = fopen(desc_name,"w+");
#endif
	if(!descf){
fprintf(logfile,"Cannot open descriptor temp file\n");
		abort();
	}
	fprintf(descf, "Descriptor Database\n");
    }

#ifdef MSDOS
    fseek(descf,0l,2);	/* seek to end */
#else
    fseek(descf,0 ,2);	/* seek to end */
#endif

    if( 0 > (descpos = ftell(descf))){
	fprintf(logfile,"alloc_desc: ftell failure\n");
	abort();
    }


    fwrite((char *)&n, Sizeof(int), 1, descf);
    fwrite(str, Sizeof(char), n+1, descf);
    return descpos;
}

desctype alloc_desc_2(s)
char *s;
{	desctype ret = alloc_desc(s);
	free_string(s);
	return ret;
}

static char *descbuf[4];
static desctype descarr[4] = {-1, -1, -1,-1};
static int descnext;

char *get_desc( pos)
desctype pos;
{
#ifdef MSDOS
	long dpos = pos;
#else
	int dpos = pos;
#endif
	int len= 1, cnt= 1;
	int n;

	if( !descbuf[0]){
		for(descnext=0; descnext < 4; descnext++){
			descbuf[descnext] = (char *)malloc(2096);
		}
	}

	if(!dpos)
		return NULL;	/* must be NULL! */

	for(n=0; n < 4; n++){
		if( descarr[n] == pos)
			return descbuf[n];
	}
	descnext++;
	if( descnext > 3)descnext = 0;

	descarr[descnext] = pos;

	if( fseek(descf, dpos, 0)){
#ifdef MSDOS
sprintf(descbuf,"get_desc: fseek(%ld) failed!\n", dpos);
#else
sprintf(descbuf,"get_desc: fseek(%ld) failed!\n", dpos);
#endif
		goto bad;
	}
	if( dpos != ftell(descf)){
#ifdef MSDOS
sprintf(descbuf,"Bad ftell: %ld, %ld, %ld",ftell(descf), dpos, descpos);
#else
sprintf(descbuf,"Bad ftell: %d, %d, %d",ftell(descf), dpos, descpos);
#endif
		goto bad;
	}

	if( 1 != (cnt = fread(&len, Sizeof(int), 1, descf) )){
#ifdef MSDOS
sprintf(descbuf,"get_desc: fread(len) failed. pos=%ld,cnt = %d, descpos = %ld", dpos, cnt, descpos);
#else
sprintf(descbuf,"get_desc: fread(len) failed. pos=%d,cnt = %d,descpos = %d", dpos, cnt, descpos);
#endif
		goto bad;
	}
	if(len > 2047){
		len = 2047;
	}
	if( len < 0){
		sprintf(descbuf[descnext],"Bad len: %d", len);
		goto bad;
	}

	if( len+1 != (cnt = fread(descbuf[descnext], Sizeof(char), len+1, descf))){
#ifdef MSDOS
sprintf(descbuf,"fread problem wanted %d, got %d, pos %ld, size %ld", len+1, cnt, dpos, descpos);
#else
sprintf(descbuf,"fread problem wanted %d, got %d, pos %d, size %d", len+1, cnt, dpos, descpos);
#endif
		goto bad;
	}
	desc_got += len+1;

	descbuf[descnext][len] = '\0';
	return descbuf[descnext];
bad:
	fprintf(logfile,"%s\n",descbuf[descnext]);
	return descbuf[descnext];
}
#endif

/* Here are the caching routines */

static char any[256];		/* Just a block of memory */

void print_mru_sub()
{	struct vm_cache *t;

	fprintf(logfile,"MRU(%d)->", (int)vm_size);
	t = mru;
	while(t){
		fprintf(logfile,"%d(%d)->", t->this, t->indx);
		t = t->next;
	}
	fprintf(logfile,"\n");
}

void print_mru()
{	struct vm_info_rec *saved = vm_info;
#ifdef VM_DB
	vm_info = &vm_db_info;
	print_mru_sub();
#endif

#ifdef VM_BOOL
	vm_info = &vm_bool_info;
	print_mru_sub();
#endif
	vm_info = saved;
}

void cache_insert(indx)
int indx;
{	struct vm_cache **p, *t;

	p = &(cache[indx]);
	while(1){
		if( indx < (ncache-1) && (*p)->this > p[1]->this){
			t = p[1];
			p[1] = *p;
			*p = t;
			t->indx = indx;
			p++;
			indx++;
			(*p)->indx = indx;
			continue;
		}
		if( indx && (*p)->this < p[-1]->this){
			t = p[-1];
			p[-1] = *p;
			*p = t;
			t->indx = indx;
			p--;
			indx--;
			(*p)->indx = indx;
			continue;
		}
		break;
	}
}

void vm_free( t)
struct vm_cache *t;
{
	free( (FREE_TYPE) t->main);
	free( (FREE_TYPE) t->shadow);
	free( (FREE_TYPE) t);
}


void vm_read(i, p)
dbref i;
struct vm_cache *p;
{	filepos pos;
	char *f, *t;
	int cnt;

	if(!vm_file){	/* open the file */
fprintf(logfile,"vm_read: open %s\n", vm_name);
#ifdef MSDOS
		vm_file = fopen(vm_name,"wb+");
#else
		vm_file = fopen(vm_name,"w+");
#endif
		if(!vm_file){
fprintf(logfile,"Cannot open vm temp file\n");
			abort();
		}
	}

	pos = (i+1) * vm_size;
	if( pos >= vmpos){
		memcpy(p->main, any, (int)vm_size);
		p->main->me= i;
	}else {
		if( fseek(vm_file, pos, 0) ){
fprintf(logfile,"fseek on vm_file to %ld failed\n", pos);
			return;
		}
		if( fread( p->main, (int)vm_size, 1, vm_file) <= 0){
fprintf(logfile,"Fread failed for object %d\n", i);
			perror("vm_read");
			return;
		}
	}
	p->main->me = i;

	f = (char *) (p->main);
	t = (char *)(p->shadow);
	memcpy(t, f, (int)vm_size);
	p->this = i;
}


void vm_write(i, p)
dbref i;
struct vm_cache *p;
{	filepos pos;

	if(!vm_file){	/* open the file */
fprintf(logfile,"vm_write: open %s\n", vm_name);
#ifdef MSDOS
		vm_file = fopen(vm_name,"wb+");
#else
		vm_file = fopen(vm_name,"w+");
#endif
		if(!vm_file){
fprintf(logfile,"Cannot open vm temp file\n");
			abort();
		}
	}
	pos = (i+1) * vm_size;
	while( pos > vmpos){
#ifdef MSDOS
		fseek(vm_file,vmpos,0);
#else
		fseek(vm_file,vmpos ,0);
#endif
		fwrite(any, (int)vm_size, 1, vm_file);
		vmpos = ftell(vm_file);
	}
#ifdef MSDOS
		fseek(vm_file,pos,0);
#else
		fseek(vm_file,pos ,0);
#endif
	if( p){
		p->main->me = i;
		fwrite( p->main, (int)vm_size, 1, vm_file);
	}else{
		fwrite( any, (int)vm_size, 1, vm_file);
	}
	pos = ftell(vm_file);
	if( pos > vmpos)
		vmpos = pos;
}

int vm_dirty(t)
struct vm_cache *t;
{	char *p, *p1;
	int cnt = vm_size;

	p = (char *) (t->main );
	p1= (char *) (t->shadow );
	while(cnt--){
		if( *p++ != *p1++)
			return 1;
	}
	return 0;
}

struct vm_cache *vm_fill( obj)
dbref obj;
{	struct vm_cache *t, **p, **p2;
	int cnt;

	if( ncache < CACHE_SIZE){
		cnt = ncache;
		t = (struct vm_cache *)malloc(Sizeof(struct vm_cache));
		if( !t){
fprintf(logfile,"\nNo memory in vm_fill( %d)\n", obj);
			abort();
		}
		t->next = NULL;
		t->prev = NULL;
		t->main = (struct vm_rec *) malloc((int)vm_size);
		t->shadow = (struct vm_rec *) malloc((int)vm_size);
		if( !t->main || !t->shadow){
fprintf(logfile,"\nNo memory in vm_fill2(%d)\n", obj);
			abort();
		}
		t->main->me = obj;
		t->shadow->me = obj;
		cache[cnt] = t;
		t->this = obj;
		t->indx = cnt;

		ncache++;
	}else{
		t = mru;
		while(t && t->next)
			t = t->next;
		cnt = t->indx;

		if( vm_dirty(t) ){
			vm_write(t->this, t);
		}
	}
	vm_read(obj, t);
	t->indx = cnt;
	t->this = obj;
	cache_insert(cnt);
	return t;
}

struct vm_cache *find_cache( obj)
dbref obj;
{	struct vm_cache *t;
	int i, j, k, n;

	if( obj < 0){
fprintf(logfile,"find_cache(%d)\n", obj);
		t = (struct vm_cache *) -4;
		t->this = 9999999;
	}

   if( ncache){
	for(j = 1; j <= ncache; j = j+j);

	j = j/2;
	i = j-1;

	while(j){	/* some to try */
		j = j/2;
		t = cache[i];
		if( t->this == obj){	/* Found it*/
			goto done;
		}else if(t->this > obj){ i = i-j;
		}else {
			i = i+j;
			while( i >= ncache && j){
				i = i-j;
				j = j/2;
				i = i+j;
			}
		}
	}
   }

	t = vm_fill(obj);
done:
	if( t != mru){
		if( t->next) t->next->prev = t->prev;
		if( t->prev) t->prev->next = t->next;
		t->next = mru;
		t->prev = NULL;
		if( mru) mru->prev = t;
		mru=t;
	}
	return t;
}

int check_cache()
{	struct vm_cache **p;
	int cnt = 0, pos, res = 1;

	p = &cache[0];
	pos = 0;
	while( p < &cache[ncache]){
		if( (*p)->this < cnt){
fprintf(logfile,"check_cache(%d less than %d)\n", (*p)->this, cnt);
			res = 0;
		}
		cnt = (*p)->this;
		if( (*p)->indx != pos){
fprintf(logfile,"check_cache(%d not in postion %d)\n", (*p)->indx, pos);
			res = 0;
		}
		p++;
		pos++;
	}
	if( !res){
		for(cnt=0; cnt < ncache; cnt++){
fprintf(logfile,"%d(%d,%d) ",cnt, cache[cnt]->this, cache[cnt]->indx);
		}
	}
	return res;
}



#ifdef VM_DB
void DbFree(i)
dbref i;
{
	vm_info = &vm_db_info;
}

void db_grow(i)
dbref i;
{	struct vm_cache **t;

	if( i > db_top)
		db_top = i;

	vm_info = &vm_db_info;

	if( ncache == CACHE_SIZE){
fprintf(logfile,"db_grow(%d)\n", i);
		for(t = &cache[0]; t != &cache[CACHE_SIZE]; t++){
			if( vm_dirty(*t))
				vm_write((*t)->this, *t);
			vm_free(*t);
			*t = NULL;
		}
		ncache = 0;
	}
	vm_write(i, NULL);	/* Create the new entry */
}

struct object *vm_db(i)
dbref i;
{	struct vm_cache *t;

	vm_info = &vm_db_info;
	t = find_cache(i);
check_cache();
	return (struct object *)&( t->main->data);
}

void vm_dbw(i, j)
dbref i;
union any_object *j;
{	struct vm_cache *t;

	vm_info = &vm_db_info;
	t = find_cache(i);

	memcpy((char *) &(t->main->data), (char *)j, (unsigned)Sizeof(union any_object) );
	t->main->me = i;
}
#endif

#ifdef VM_BOOL

BOOLEXP *vm_bool(i)
BOOLEXP_PTR i;
{	struct vm_cache *t;
	vm_info = &vm_bool_info;
	t = find_cache(i);
check_cache();
	return (BOOLEXP *)&( t->main->data);
}

#endif

vm_reopen_sub()
{
#ifdef VM_DB
	fclose(vm_file);
#ifdef MSDOS
	vm_file = fopen(vm_name, "rb+");
#else
	vm_file = fopen(vm_name, "r+");
#endif
	if(!vm_file){
fprintf(logfile,"Could not re-open %s\n", vm_name);
		abort();
	}else
fprintf(logfile,"Re-opened %s\n",vm_name);
#endif
}

vm_reopen()
{
#ifdef VM_DB
	vm_info = &vm_db_info;
	vm_reopen_sub();
#endif

#ifdef VM_BOOL
	vm_info = &vm_bool_info;
	vm_reopen_sub();
#endif
}
