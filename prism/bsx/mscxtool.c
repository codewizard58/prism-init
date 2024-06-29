/*----------------------------------------------------------------------------

        Purpose         :       Equivalent for xtool file of X11
        Method          :       Ask BGI to do some drawing
        Author          :       Bram Stolk
        Date            :       Jan. 1992

        This program is a part of the bsxmud client, MS-DOS version.

	MSC mods by P.J.Churchyard.

 *---------------------------------------------------------------------------*/
#include <stdio.h>
#ifdef MSC
#include <graph.h>
#endif
#include "config.h"

#define WIN_TITLE	"Brasto"                /* title to put on the window */
#define SCR_WIDTH	512
#define SCR_HEIGHT	256

#define SOLID_FILL	_GFILLINTERIOR

void visualise_poly();

/* error() - generic error message handler */
error(message)
char *message;
{
  fprintf(stderr, "bsxclient: %s\n", message);
  exit(-1);
}

struct videoconfig vc;

/* init_display() - initialize the X display */
init_display()
{	int driver,mode;
	struct polygon p;

	_getvideoconfig( &vc);

	if( !_setvideomode( _MAXRESMODE)){
fprintf(stderr,"Can't set video to MAXRESMODE\n");
		return;
	}

	_getvideoconfig( &vc);


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
  _setvideomode( _DEFAULTMODE);
}

update_display() {
/* copy the finished frame buffer to the window */
}

/* Handle different types of screen resolution */
int (*xfunc)();
int (*yfunc)();

int cga_x(x)
int x;
{	return x << 1;
}

int cga_y(y)
int y;
{	return y >> 1;
}

int ega_x(x)
int x;
{	return x << 1;
}

int ega_y(y)
int y;
{	return y;
}

int MAP_X( x)
int x;
{
	if( !xfunc){
		switch( vc.numxpixels){
		case 320:
			xfunc = cga_x;
			break;
		default:
			xfunc = ega_x;
			break;
		}
	}
	return (*xfunc)(x);
}

int MAP_Y( y)
int y;
{
	if( !yfunc){
		switch( vc.numypixels){
		case 200:
			yfunc = cga_y;
			break;
		default:
			yfunc = ega_y;
			break;
		}
	}
	return (*yfunc)(y);
}


/*-------------------------------------------------------------------------*/

void visualise_poly(p)          /* Display a single poly */
struct polygon p;
{
  int i;
  struct xycoord q[32];

  if( p.size > 32)
	p.size = 32;

  for (i=0; i<p.size; i++) {
	q[i].xcoord = MAP_X(p.x[i]);
	q[i].ycoord = MAP_Y(255 - p.y[i]);
  }
  setfillstyle(SOLID_FILL,p.color);
  _polygon(_GFILLINTERIOR, q, p.size);
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

fillpoly(num, q)
int num;
int *q;
{	int i;

	_moveto((short) q[0],(short) q[1]);
	for(i = 1; i <num;i++){
		_lineto((short)q[i<<1], (short) q[(i<<1) + 1]);
	}
	_lineto( (short)q[0], (short)q[1]);
}

char mask[16][8] = { 
{0xff, 0xff,0xff, 0xff,0xff, 0xff,0xff, 0xff},
{0x55, 0xaa,0x55, 0xaa,0x55, 0xaa,0x55, 0xaa},
{0x33, 0xcc,0x33, 0xcc,0x33, 0xcc,0x33, 0xcc},
{0xcc, 0xcc,0x33, 0x33,0xcc, 0xcc,0x33, 0x33},
{0x01, 0x02,0x04, 0x08,0x10, 0x02,0x04, 0x08},
{0x80, 0x40,0x20, 0x10,0x08, 0x04,0x02, 0x01},
{0x08, 0x08,0xff, 0x08,0x08, 0xff,0x08, 0xff},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
{0x55, 0xaa,0xaa, 0x55,0x55, 0xaa,0xaa, 0x55},
};

int setfillstyle(style, colour)
int style, colour;
{	char *rmask = &mask[0][0];

	if( vc.numcolors == 2){
		rmask = &mask[colour][0];
	}	
	_setfillmask(rmask);
	_setcolor(colour);
}

setcolor(colour)
int colour;
{
	_setcolor(colour);
}

line(x0,y0, x1,y1)
int x0,x1,y0,y1;
{
	_moveto((short)MAP_X(x0), (short)MAP_Y(y0));
	_lineto((short)MAP_X(x1), (short)MAP_Y(y1));
}

#include <signal.h>

void (*saved_func)();

void do_brk(code)
int code;
{
	if( saved_func)
		(*saved_func)(code);
	exit();
}

ctrlbrk(func)
void (*func)();
{
	saved_func = func;

	signal(SIGINT, do_brk);
}

print_debug()
{
	fprintf(stderr,"numcolours = %d\n", vc.numcolors);
	fprintf(stderr,"xpixels    = %d\n", vc.numxpixels);
	fprintf(stderr,"ypixels    = %d\n", vc.numypixels);
}
