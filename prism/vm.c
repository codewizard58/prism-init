/* db Virtualiser
 * by P.J.Churchyard
 */

#include "stdio.h"
#include "db.h"

static int vm_num;	/* count of the number of things in the store */

#ifdef VM_DB

#define MAX_VM 500

struct vm *vmhead, *vmfree, *vmtail;
FILE *objf;
char *object_name = "OBJECT.TMP";

long vm_found,vm_read,vm_write;	/* stats counters */

/* The virtual memory store keeps stuff in most recently used order */

struct vm *vm_free1()
{	long pos;
	struct vm *v,*v_last;

	if(vmtail && !vmtail->flags)
		return vmtail;

	for(v = vmtail; v ; v = v->prev){
		v_last = v;
		if( v->flags){
			pos = sizeof(struct object);
			pos = pos * v->obj;
fprintf(stderr,"vm_free1 %d\n", v->obj);
			fseek(objf,pos,0);
			fwrite((char *)v->object,sizeof(struct object),1,objf);
			v->flags = 0;
			vm_write++;
		}
	}
/*	db[v_last->obj] = NULL; */
	return v_last;
}

struct vm *vm_flush()
{	long pos;
	struct vm *v,*v_last;
	int n=0;

	for(v = vmhead; v ; v = v->next){
		v_last = v;
		if( v->flags){
			pos = sizeof(struct object);
			pos = pos * v->obj;
fprintf(stderr,"vm_flush %d\n", v->obj);
			fseek(objf,pos,0);
			fwrite((char *)v->object,sizeof(struct object),1,objf);
			v->flags = 0;
			vm_write++;
		}
	}
/*	db[v_last->obj] = NULL; */
	return v_last;
}

struct vm *vm_find( thing)
dbref thing;
{	struct vm *v;
	long pos;
	int found = 0;


	for(v=vmhead; v ; v = v->next){
		if( v->obj == thing){
			found++;
			vm_found++;
			break;
		}
	}

	if( !v){	/* not found */
		if( vmfree){
			v = vmfree;
			vmfree = v->next;
			v->prev = NULL;
			v->next = NULL;
			v->obj  = thing;
		}else if( vm_num < MAX_VM){	/* create a new one */
			vm_num++;
			v = (struct vm *)malloc( sizeof(struct vm));
			if(!v)
				abort_msg("No memory in vm_find!");
			v->object = (struct object *)malloc( sizeof(struct object));
			if(!v->object)
				abort_msg("Cant allocate object in vm_find!");
			v->flags = 1;
			v->obj   = thing;
			v->next  = NULL;
			v->prev  = NULL;
		}else {
			v = vm_free1();		/* write out the dirty ones */
			if( v){
				v->obj = thing;
				v->flags= 0;
			}
		}
	}
	if( !found && v){
		/* read it in */
		pos = sizeof(struct object);
		pos = thing * pos;
		fseek(objf, pos, 0);
		if( !fread(v->object, sizeof(struct object),1, objf)){
			fprintf(stderr,"thing = %d, pos= %ld\n",thing, pos);
			abort_msg("fread failed in vm_find!");
		}
		v->flags = 0;
		v->obj   = thing;
		vm_read++;
	}

	if( !v){
		abort_msg("Could not find object in vm_find!");
	}

	if( v != vmhead){	/* bring to top */
		if(vmtail == v)
			vmtail = v->prev;
		if( v->prev){
			v->prev->next = v->next;
		}
		if( v->next ){
			v->next->prev = v->prev;
		}
		v->prev = NULL;
		v->next = vmhead;
		if(v->next)
			v->next->prev = v;
		vmhead = v;
		if(!vmtail)
			vmtail = v;
	}
/*	db[thing] = v; */
	return v;
}

struct object *vm_dbf(x)
dbref x;
{
	return vm_find(x)->object;
}

struct object *vm_dbfw(x)
dbref x;
{	struct vm *v = vm_find(x);
	v->flags = 1;
	return v->object;
}

void vm_grow(x)
dbref x;
{	long pos = x, pos2;
	static struct object dum;
	struct object *o;

	if( pos < 0l)pos = 0l;

	pos = pos* sizeof(struct object);

	if(!objf){
		objf = fopen(object_name,"wb+");
		if(!objf)
			abort_msg("Could not open OBJECT.TMP");
	}

	fseek(objf, 0l, 2);	/* go to end */
	pos2 = ftell(objf);

	fwrite((char *) &dum, sizeof(struct object), 1, objf);

	if(pos2 != pos){
fprintf(stderr,"vm_grow: %ld != %ld\n", pos, pos2);
	}

	o = vm_dbfw(x);
}
#endif
