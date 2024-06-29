#include "config.h"

/*
 * This file contains code for the administration of a scene contents.
 * It keeps record of the current scene and all the objects that are present
 * in that particular scene.
 * It can even redraw the entire scene, if you ask it to. *smile*
 */

typedef struct {
  char          name[MAXIDLEN];
  unsigned char position;
  unsigned char depth;
} obj_info;

struct {
  char     scene[MAXIDLEN];
  obj_info objects[MAXCROWD];
} scene_admin;

extern void draw_counter();
extern int find_id();
extern void lookup_and_depict_scene();
extern void update_display();

void introduce_new_scene(name)
char *name;
{
  unsigned char i;

  strcpy(scene_admin.scene, name);
  for (i=0; i<MAXCROWD; i++)
    strcpy(scene_admin.objects[i].name,"**Empty**");
}

void add_object_to_scene(name,position,depth)
char *name;
unsigned char position,depth;
{
  unsigned char i;

  /* Assume: object not already in scene! */
  for (i=0; i<MAXCROWD; i++)
    if (strcmp(scene_admin.objects[i].name,"**Empty**")==0) {
      strcpy(scene_admin.objects[i].name,name);
      scene_admin.objects[i].position = position;
      scene_admin.objects[i].depth    = depth;
      break;
    }
    if (i==MAXCROWD)
      puts("Error - the scene has been overloaded!\n");
}

void remove_object_from_scene(name)
char *name;
{
  unsigned char i;

  for (i=0; i<MAXCROWD; i++)
    if (strcmp(scene_admin.objects[i].name,name)==0) {
      strcpy(scene_admin.objects[i].name,"**Empty**");
      break;
    }
    if (i==MAXCROWD)
      printf("Error - Scenery administrator was asked to remove non present obj: '%s'\n",name);
}

void list_scene_contents() {
  unsigned char i;

  printf("The current scene is called %s and contains:\n", scene_admin.scene);
  for (i=0; i<MAXCROWD; i++)
    if (strcmp(scene_admin.objects[i].name,"**Empty**"))
      printf("  -  %s\t\t: %d,%d\n",scene_admin.objects[i].name,
				    scene_admin.objects[i].position,
                                    scene_admin.objects[i].depth);
}

void redraw_entire_scene() {
  int s,layer;
  unsigned char i;
  char *name;

  s = find_id(scene_admin.scene);
  if (s==-1)
    return;  /* Desc not yet known... there will probably be another @RFS */
  lookup_and_depict_scene(s,127,127,100);
  for (layer=7; layer>=0; layer--) {
    draw_counter(layer);
    for (i=0; i<MAXCROWD; i++) {
      name = scene_admin.objects[i].name;
      if ((strcmp(name,"**Empty**"))&&(scene_admin.objects[i].depth==layer)) {
        s = find_id(name);
        if (s!=-1) {
          lookup_and_depict_scene(s,
                                  16*scene_admin.objects[i].position,
                                  4*scene_admin.objects[i].depth,
                                  100
                                 );
        }
      }
    }
  }
  update_display();
}

void test_scenery_administrator() {
  introduce_new_scene("Mooi Landschapje");
  list_scene_contents();
  add_object_to_scene("Mazarax",1,1);
  add_object_to_scene("Hellhound",2,2);
  add_object_to_scene("Brainiac",3,3);
  list_scene_contents();
  remove_object_from_scene("Brainiac");
  list_scene_contents();
  remove_object_from_scene("Emperor");
}
