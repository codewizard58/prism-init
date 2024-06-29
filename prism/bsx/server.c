#include <stdio.h>
#include <fcntl.h>
#include "config.h"

typedef struct {        /* Structure describing a scene of several polys */
  char id[MAXIDLEN];
  struct polygon poly[MAXDESCSIZE];
  char descsize;
} desc;

desc dbase[DBASESIZE];                      /* Last recently used scenes */

/* Interfase to text.c code */
extern void download_counter();

/* Interface to internet.c code */
extern unsigned char eat_hex();
extern unsigned char eat_char();
extern unsigned char eat_raw_char();
extern void tell_server();

/* Interface to scenery.c code */
extern void introduce_new_scene();
extern void add_object_to_scene();
extern void remove_object_from_scene();
extern void redraw_entire_scene();
extern void list_scene_contents();

extern int done;

char logging=1;

/*-------------------------------------------------------------------------*/
/*      A U X I L A R Y     S T U F F                                      */
/*-------------------------------------------------------------------------*/

unsigned char get_slot() {      /* Find slot for a scene */
  static int slot = 0;
  if (++slot==DBASESIZE)
    slot = 0;
  return(slot);
}

/*-------------------------------------------------------------------------*/

int find_id(id)         /* Check wether desc is already known */
char *id;
{
  char found;
  int s, retval;

  found = 0;
  for (s=0; ((s<DBASESIZE) && (!found)); s++)
    if (strcmp(id,dbase[s].id)==0) {
      retval = s;
      found = 1;
    }
  if (found)
    return(retval);
  else
    return(-1);
}

void depict_scene(scene,centrex,centrey,scale)
desc scene;
unsigned char centrex, centrey;
int scale;
{
  unsigned char p, size, v;
  struct polygon translated_poly;

  for (p=0; p<scene.descsize; p++) {
    size = scene.poly[p].size;
    translated_poly.size = size;
    translated_poly.color= scene.poly[p].color;
    for (v=0; v<size; v++) {
      int tmpx, tmpy;

      tmpx = (((scene.poly[p].x[v]-127) /* * scale/100 */ )) + centrex;
      tmpy = (((scene.poly[p].y[v]-127) /* * scale/100 */ )) + centrey;
      if (tmpx>255) tmpx = 255;
      if (tmpy>255) tmpy = 255;
      if (tmpx<  0) tmpx =   0;
      if (tmpy<  0) tmpy =   0;
      translated_poly.x[v] = tmpx;
      translated_poly.y[v] = tmpy;
    }
    visualise_poly(translated_poly); 
  }
}

void lookup_and_depict_scene(entry,centrex,centrey,scale)
unsigned int entry;
unsigned char centrex;
unsigned char centrey;
int scale;
{
  if (entry<DBASESIZE)
    depict_scene(dbase[entry],centrex,centrey,scale);
}

/*-------------------------------------------------------------------------*/
/*      S E R V E R    P R O C E D U R E S                                 */
/*-------------------------------------------------------------------------*/

void serve_dfs() {
  unsigned char c, p, i, size, v;
  unsigned char id[MAXIDLEN];
  int s, cnt;

  /* define scene */
  for (i=0; ((c=eat_char()) != '.')&&(i<MAXIDLEN-1) ; i++)
    id[i] = c;
  id[i] = 0;
  s = find_id(id);
  if (s==-1)
    s = get_slot();
  for (i=0; i<MAXIDLEN; i++)
    dbase[s].id[i] = id[i];
  /* printf("DEFINING SCENE/OBJ : %s\n",dbase[s].id); */
  cnt = dbase[s].descsize = eat_hex();
  for (p=0; p<dbase[s].descsize; p++) {
    /* download_counter(dbase[s].descsize-p-1); */
    size = eat_hex();
    if( size > 32){
fprintf(stderr,"poly %d,size=%d skipping\r", size);
	while( (c=eat_char()) != '\r' && c != EOF && c != '\n');
	while( p < dbase[s].descsize){
		dbase[s].poly[p].size = 0;
		dbase[s].poly[p].color= 0;
		p++;
	}
	return;
    }
    dbase[s].poly[p].size = size;
    dbase[s].poly[p].color = eat_hex();
    for (v = 0; v < size; v++) {
      dbase[s].poly[p].x[v] = eat_hex();
      dbase[s].poly[p].y[v] = eat_hex();
    }
  }
}

void serve_sce() {
  unsigned char i,c,p;
  unsigned char id[MAXIDLEN];
  unsigned char mesg[MAXIDLEN+8];
  int s;

  /* SCEne */
  for (i=0; ((c=eat_char()) != '.')&&(i<MAXIDLEN-1) ; i++)
    id[i] = c;
  id[i] = 0;
  s = find_id(id);
  if (s==-1) {
    tell_server("#RQS ");
    tell_server(id);
    tell_server("\n");
  } 
  introduce_new_scene(id);          /* Let the scene-admin know about this */
}

void serve_rfs() {
  redraw_entire_scene();
}

void serve_vio() {
  unsigned char c,i,position,depth;
  unsigned char id[MAXIDLEN];
  int s;

  for (i=0; ((c=eat_char()) != '.')&&(i<MAXIDLEN-1) ; i++)
    id[i] = c;
  id[i] = 0;
  s = find_id(id);
  if (s==-1) {
    tell_server("#RQO ");
    tell_server(id);
    tell_server("\n");
  } 
  position = eat_hex();
  if (position>15)
    position = 0;
  depth    = eat_hex();
  if (depth>7)
    depth = 7;
  add_object_to_scene(id,position,depth);
}

void serve_rmo() {
  unsigned char i,c,id[64];

  for (i=0; ((c=eat_char()) != '.')&&(i<MAXIDLEN-1) ; i++)
    id[i] = c;
  id[i] = 0;
  remove_object_from_scene(id);
}

void serve_txt() {
  char mesg[800];
  unsigned char c;
  int i;

  for (i=0; ((c=eat_raw_char()) != '$')&&(i<800-1) ; i++)
    mesg[i] = c;
  mesg[i] = 0;
  do_text(mesg);
}

void serve_tms() {
  done = 1;
}

void serve_lof() {
  logging = 0;
}

void serve_lon() {
  logging = 1;
}

void serve_pur() {
  int slot;
  unsigned char i,c,id[64];

  for (i=0; ((c=eat_char()) != '.')&&(i<MAXIDLEN-1) ; i++)
    id[i] = c;
  id[i] = 0;
  slot = find_id(id);
  if (slot != -1)
    strcpy(dbase[slot].id, "** PURGED **");
}

void serve_rqv() {
  tell_server("#VER DOS  ");
  tell_server(VERSION);
  tell_server("\n");
}

/*
 * Module entry point
 *
 */

int process_bsx_command(command)
char *command;
{
  if ((strcmp(command,"DFS")==0) || (strcmp(command,"DFO")==0)){
    /* define scene , or: define object */
    serve_dfs();
    return 1;
  }
    
  if (strcmp(command,"SCE")==0){
    /* SCEne */
    serve_sce();
    return 1;
  }
  if (strcmp(command,"VIO")==0){
    /* VIsualise Object */
    serve_vio();
    return 1;
  }
  if (strcmp(command,"RMO")==0){
    /* ReMove Object */
    serve_rmo();
    return 1;
  }
  if (strcmp(command,"RFS")==0){
    /* ReFreSh display */
    serve_rfs();
    return 1;
  }
  if (strcmp(command,"TXT")==0){
    /* TeXT */
    serve_txt();
    return 1;
  }
  if (strcmp(command,"TMS")==0){
    /* TerMinate Session */
    serve_tms();
    return 1;
  }
  if (strcmp(command,"LOF")==0){
    /* Logging OFf */
    serve_lof();
    return 1;
  }
  if (strcmp(command,"LON")==0){
    /* Logging ON */
    serve_lon();
    return 1;
  }
  if (strcmp(command,"PUR")==0){
    /* PURge a dbase entry */
    serve_pur();
    return 1;
  }
  if (strcmp(command,"RQV")==0){
    /* ReQuest Version of client */
    serve_rqv();
    return 1;
  }

  return 0;	/* Unknown function */
}

