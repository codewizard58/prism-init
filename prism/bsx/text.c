#include <stdio.h>
#ifndef MSC
#include "mem.h"
#else
#include <memory.h>
#include <graph.h>
#endif
#include <dos.h>

unsigned char xpos;
unsigned char prompt_xpos;
unsigned char set[128][8];
unsigned char prev_was_cr;
char dummy_space[640];

extern char * malloc();

#ifdef MSC
extern struct videoconfig vc;
#endif

/*
 * Wipes the 8 lines on and below the specified y-coordinate
 */
#ifdef MSC

#undef FP_SEG
#undef FP_OFF

unsigned FP_SEG(x)
char *x;
{	long y;

	y = (long)x;

	return (unsigned) (y >> 16);
}

unsigned FP_OFF(x)
char *x;
{	long y;

	y = (long)x;
	return (unsigned) (y & 0xffff);
}

char *MK_FP(seg,off)
unsigned seg,off;
{
	long x = (((long) seg) << 16)|off;
	return (char *)x;
}

#endif

char solidmask[8] = {
0xff, 0xff,
0xff, 0xff,
0xff, 0xff,
0xff, 0xff
};

void wipe_line(y)
int y;
{ 
/*  movedata(FP_SEG(dummy_space),FP_OFF(dummy_space),0xa000,y*80,640); */
	_setfillmask( solidmask);
	_setcolor(0);
	_rectangle(_GFILLINTERIOR, 0, MAP_Y(y), MAP_X(319), MAP_Y(y+8));
	_setcolor(3);
}

/*
 * Please note!
 * This is a very weird function:
 * the x coordinate is in 8pixel res.
 * the y coordinate is in pixels.
 */
void print_at(x,y,c)
int x,y;
char c;
{	char buf[2];

	buf[0] = c;
	buf[1] = '\0';

	_moveto(MAP_X(x*4), MAP_Y(y) );
	_outgtext(buf);
}

void print_string_at(x,y,s)
int x,y;
char *s;
{
  while (*s) {
    print_at(x++,y,*s);
    s++;
  }
}

/*
 * Scrolls up the textwindow by 8 pixels.
 */
void scroll_up() {
long cnt;
char *buf;

  if (prev_was_cr)
    return;
  xpos = 0;

  cnt = _imagesize(0, MAP_Y(259+8), MAP_X(319), MAP_Y(331+8));
  buf = (char *)malloc( (size_t) cnt);
  _getimage(0, MAP_Y(259+8), MAP_X(319), MAP_Y(331+8), buf);
  _putimage(0, MAP_Y(259), buf, _GPSET);
  free(buf);
  wipe_line(331);
}

/*
 * This is where the non-graphic data from the mudserver goes.
 */ 
void add_char_to_text_window(c)
char c;
{
  c &= 127;
  if (c==13 || c==10) {	/* Handle CRs and Newlines */
    scroll_up();
    prev_was_cr = 1;
    return;
  }
  prev_was_cr = 0;
  if (c==8) {	/* Handle backspaces */
    if (xpos) {
      xpos--;
      print_at(xpos,331,' ');
    }
    return;
  }
  if (c==9) {	/* Handle TAB chars .... assume 8 spacing */
    unsigned char t;
    t = ((xpos>>3)+1)<<3;
    while ((xpos < t) && (xpos<80))
      print_at(xpos++,331,' ');
    return;
  }
  if (c<32)
    c=127;
  if (xpos==80)
    scroll_up();
  print_at(xpos,331,c);
  xpos++;
}

void add_string_to_text_window(s)
char *s;
{
  while (*s) {
    add_char_to_text_window(*s);
    s++;
  }
}

void add_char_to_prompt_line(c)
char c;
{
  if (c==8) {
    if (prompt_xpos) {
      prompt_xpos--;
      print_at(prompt_xpos,342,' ');
    }
    return;
  }
  if ((c==13 || c==10) && (prompt_xpos!=0)) {
    prompt_xpos = 0;
    wipe_line(342);
    return;
  }
  if (c<32)
    return;
  if (prompt_xpos==80)
    return;
  print_at(prompt_xpos++,342,c);
}

void init_text_window() {
  int c,b;
  FILE *f;

  xpos = 0;
  prompt_xpos = 0;
  prev_was_cr = 0;
  for (c=0; c<640; c++)
    dummy_space[c] = 0;
#ifdef MSC
	if( 0 > _registerfonts( "ROMAN.FON")){
fprintf(stderr,"Could not find ROMAN font\n");
		exit(1);
	}
	switch( vc.numypixels){
	case 200:
		_setfont( "t'roman'h4w8v");
		break;

	default:
		_setfont( "t'roman'h8w8v");
		break;
	}
#else
  f = fopen("clean.set","r+b");

  if (!f) {
    puts("Error, cannot load character set file 'clean.set'");
    exit(1);
  }
  for (c=32; c<128; c++)
    for (b=0; b<8; b++)
      set[c][b] = fgetc(f);
  fclose(f);
#endif
  add_string_to_text_window("BSX client for MS-DOS\nversion 1.0\n(c)Feb.1992 by Bram Stolk.\nMSC conversion by P.J.Churchyard 17-Apr-92\nUse the -h option for a help screen.\n");
  print_string_at(66,0, "DOWNLOADING:");
  print_string_at(66,24,"POLYDRAWING:");
}

/*
 * Some code to implement the wait counters, to calm down the users
 */

download_counter(c)
unsigned char c;
{
  char lo,hi;

  if (!c) {
    print_at(78,0,' ');
    print_at(79,0,' ');
    return;
  }
  hi = c/10;
  if (hi)
    hi = hi + '0';
  else
    hi = ' ';
  print_at(78,0,hi);
  lo = c%10;
  lo = lo + '0';
  print_at(79,0,lo);
}

draw_counter(c)
unsigned char c;
{
  if (!c) {
    print_at(79,24,' ');
    return;
  }
  print_at(79,24,c+'0');
}

