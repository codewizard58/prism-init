/*----------------------------------------------------------------------------

        Purpouse        :       Equivalent for xtool file of X11
        Method          :       Ask BGI to do some drawing
        Author          :       Bram Stolk
        Date            :       Jan. 1992

        This program is a part of the bsxmud client, MS-DOS version.

	MSC mods by P.J.Churchyard.

 *---------------------------------------------------------------------------*/
#include <stdio.h>
#ifdef tcc
#include <graphics.h>
#endif
#ifdef MSC
#include <graph.h>
#endif
#include "config.h"

#define WIN_TITLE	"Brasto"                /* title to put on the window */
#define SCR_WIDTH	512
#define SCR_HEIGHT	256

void visualise_poly();

/* error() - generic error message handler */
error(message)
char *message;
{
  fprintf(stderr, "bsxclient: %s\n", message);
  exit(-1);
}

/* init_display() - initialize the X display */
init_display() {
  int driver,mode;
  struct polygon p;

  mode = EGAHI;
  driver = EGA;
  initgraph(&driver,&mode,"");
  if (graphresult()!=grOk) {
    puts("Oops.. seems like we have a problem here.");
    puts("Please make sure that   1) your equipment is 'EGAble' and");
    puts("                        2) EGAVGA.BGI is in this directory. :-)");
  }
  p.size = 4;
  p.color = 15;
  p.x[0] =   0; p.y[0] =   0;
  p.x[1] = 255; p.y[1] =   0;
  p.x[2] = 255; p.y[2] = 255;
  p.x[3] =   0; p.y[3] = 255;
  visualise_poly(p);
}

close_display()
{
  closegraph();
}

update_display() {
/* copy the finished frame buffer to the window */
}

/*-------------------------------------------------------------------------*/

void visualise_poly(p)          /* Display a single poly */
struct polygon p;
{
  int i;
  int q[64];

  for (i=0; i<p.size; i++) {
    q[i<<1  ] = (p.x[i])<<1;
    q[(i<<1)+1] = 255 - p.y[i];
  }
  setfillstyle(SOLID_FILL,p.color);
  fillpoly(p.size,q);
}

/*-------------------------------------------------------------------------*/

void add_line(m)
char *m;
{
}

void do_text(m)
char *m;
{
}

