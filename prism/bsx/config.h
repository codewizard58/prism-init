#define VERSION "1.0"
#define DBASESIZE 16                    /* Size of FIFO LRU */
#define MAXDESCSIZE 32                  /* Max # of polys in 1 desc */
#define MAXIDLEN 64                     /* Max # of chars in id */
#define MAXCROWD 32                     /* Max # of objects in scene */

/* Note : Ehh... it's wise to have MAXCROWD < DBASESIZE to avoid a mess.
 *        But it's also nice to have the DBASESIZE not too large.
 *        Then again we DO want lotsa objects per scene.......
 *        See the dilemma? ... Good!                            Bram
 */

typedef struct polygon {                     /* Structure describing 1 poly */
  unsigned char size;
  unsigned char color;
  unsigned char x[32];
  unsigned char y[32];
};

#define STIPPLEDIR "bitmaps/"

