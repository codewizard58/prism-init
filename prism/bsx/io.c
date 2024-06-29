#include <stdio.h>
#include <dos.h>

#ifdef MSC
#define REGPACK WORDREGS
#define r_ax ax
#define r_dx dx
#define r_bx bx
#endif

int com_port;

extern void add_char_to_text_window();
extern void add_char_to_prompt_line();
void try_get_from_keyboard();

extern int done;

/*
 * Sets up an N,8,1 connection using the specified port and baudrate
 */
void init_com_port(nr,baud)
char nr[];
char baud[];
{
  struct REGPACK regs;
  int speed;

  /*
   * Determine COM port to use
   * If not specified by user, assume com1
   */
  com_port = 0;
  if (!strcmp("1",nr)) {
    com_port = 0;
    puts("Using COM1.");
  }
  if (!strcmp("2",nr)) {
    com_port = 1;
    puts("Using COM2.");
  }
  if (!strcmp("3",nr)) {
    com_port = 2;
    puts("Using COM3.");
  }
  if (!strcmp("4",nr)) {
    com_port = 3;
    puts("Using COM4.");
  }

  /*
   * Determine baudrate to use
   * If not specified by user, assume 1200
   */
  speed = 4;
  if (!strcmp("1200",baud)) {
    speed = 4;
    puts("Using a 1200 baud connection.\n");
  }
  if (!strcmp("2400",baud)) {
    speed = 5;
    puts("Using a 2400 baud connection.\n");
  }
  if (!strcmp("4800",baud)) {
    speed = 6;
    puts("Using a 4800 baud connection.\n");
  }
  if (!strcmp("9600",baud)) {
    speed = 7;
    puts("Using a 9600 baud connection.\n");
  }

  /*
   * Hmm, maybe the writers of the docs for x00 should have put
   * more emphasis on function 4. I didnt even notice it, let alone
   * find it to be important... could have saved me a whole lotta
   * trouble.
   */
  regs.r_dx = com_port;
  regs.r_ax = 1024;
  intr(0x14,&regs);
  if (regs.r_ax!=0x1954) { /* X00-author's year of bith? */
    puts("Oops... looks like ya forgot to load x00 aye?");
    puts("Try something like 'x00 R=8192 T=1024 E'");
    exit(0);
  }
  if ((regs.r_bx&255)<0x20) {
    puts("Gee, sorry, can't work with the FOSSIL driver ya're using.\n");
    puts("Try to get a FOSSIL driver that knows 20h calls (like X00).\n");
  }

  /*
   * Init line settings.
   */
  regs.r_ax = (speed << 5) + 3;
  regs.r_dx = com_port;
  intr(0x14,&regs);
}

/*
 * Try get from mud
 * If no char is available, this function returns 0
 * If one is available, it is retreived from the key_strokesfer and returned
 */
char try_get_from_mud() {
  struct REGPACK regs;
  unsigned int result;

  /*
   * Getting a char from the mud may take a while, if there
   * is netlag, or just because there aint nothing to send.
   * Meanwhile, dont forget the keyboard input!
   */
  try_get_from_keyboard();
  /*
   * Now do the stuff we are supposed to do!
   */
#ifdef DONT_USE_FUNC_20
  regs.r_dx = com_port;
  regs.r_ax = 768;
  intr(0x14,&regs);
  result = regs.r_ax;
  printf(" RES= %X ",result);
  if (result & 256) {
    regs.r_dx = com_port;
    regs.r_ax = 512;
    intr(0x14,&regs);
    return(regs.r_ax);
  } else
    return 0;
#else
  regs.r_dx = com_port;
  regs.r_ax = 8192;
  intr(0x14,&regs);
  result = regs.r_ax;
  if (result<256) {
    return(result);
  }
  if (result==0xffff) {
    return(0);
  }
  puts("THIS SHOULD NEVER HAPPEN! Function 20h screwed up!\n");
  puts("Are you sure you have an extended fossil driver loaded (x00) ?\n");
  exit(0);
#endif
}

unsigned char eat_char() {
  char c;
  do {
    c = try_get_from_mud();
  } while (!c);
  return c;
}

unsigned char eat_raw_char() {
  char c;
  do {
    c = try_get_from_mud();
  } while (!c);
  return c;
}
  
/*
 * Read data from server, and convert hex to dec
 */
unsigned char eat_hex() {
  unsigned char lo,hi;

  hi = eat_char();
  lo = eat_char();
  if (hi > '9')
    hi = hi - 55;
  else
    hi = hi - 48;
  if (lo > '9')
    lo = lo - 55;
  else
    lo = lo - 48;
  return (hi*16+lo);
}

void put_char_to_mud(c)
unsigned char c;
{
  struct REGPACK regs;

  regs.r_ax = 256+c;
  regs.r_dx = com_port;
  intr(0x14,&regs);
}

void tell_server(mesg)
char mesg[];
{
  int i;

  /* printf("SENDING TO SERVER: '%s'\n",mesg); */
  if (strlen(mesg)>=512) return; /* Ja, daar begin ik niet aan hoor! */
  for (i=0; mesg[i]!=0; i++)
    put_char_to_mud(mesg[i]);
}

/*
 * So much for the client-server communication stuff.
 * From here, we'll do the client-keyboard communication stuff.
 */
int fill=0;
char key_strokes[1001];

void try_get_from_keyboard() {
  if (kbhit()) {
    char c;
    c = getch();

	if( !c){
		c = getch();
		switch(c){
		case 0x43:	/* F9 */
			done = 1;
			return;
		case 0x42:	/* F8 */
			print_debug();
			return;
		case 0x4b:	/* <- */
			c = 8;
			break;
		default:
			fprintf(stderr,"<%2x>", c);
		}
	}
    add_char_to_prompt_line(c);

    if (c==8) {
      if (fill) {
        fill--;
      }
    } else
      if (c==13) {
        key_strokes[fill++] = c;
        key_strokes[fill++] = 0;
	fill = 0;
        tell_server(key_strokes);
      } else {
        /* Normal char -> store it! */
        key_strokes[fill++] = c;
        if (fill>=1000) fill=999;
      }
  }
}

disconnect() {
}

#ifdef MSC
int intr(func, regs)
union REGS *regs;
{	int rax;

	rax = int86(func, regs, regs);
	return rax;
}
#endif
