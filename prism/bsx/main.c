/*----------------------------------------------------------------------------

        Purpouse        :       Visualisation of X-MUD data
        Method          :       Polygon drawing
        Author          :       Bram Stolk
        Date            :       June 1991
	Last mod	:	Jan  1992

        This program is a bsxmud client. It reads data sent by a
        bsxmud server. The data is then interpreted and sent to the
        screen. This should cause mudding never to be the same.

 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <fcntl.h>
#include "config.h"

/* Interface to text.c code */
extern void add_string_to_text_window();
extern void add_char_to_text_window();
extern void init_text_window();

/* Interface to io.c code */
extern void init_com_port();
extern unsigned char try_get_from_mud();

/* Interface to internet.c code */
extern void spit_char();
extern unsigned char eat_char();

/* interface to server.c code */
extern void process_bsx_command();

int done;                               /* Marks end of session */

/*
 * Ctrl-Break handler
 */
int quit_bsx() {
  close_display();
  disconnect();
  return 0;
}

/* ----------------- Main Program --------------------------------------*/

void main(argc,argv)
int  argc;
char *argv[];
{
  int i, s;
  unsigned char c, command[4];
  char debugging = 0;
  char *arg_port;
  char *arg_baud;

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-b"))
      arg_baud = argv[i+1];
    if (!strcmp(argv[i],"-p"))
      arg_port = argv[i+1];
    if (!strcmp(argv[i],"-d"))
      debugging = 1;
    if (!strcmp(argv[i],"-h")) {
      printf("Syntax is : %s [OPTION]*\n",argv[0]);
      puts(  "  -p <COM PORT NR>           (1..4)");
      puts(  "  -b <BAUD RATE>             (1200,2400,4800,9600)");
      puts(  "  -d                         (debugging on)");
      exit(0);
    }
  }
  init_com_port(arg_port,arg_baud);
  ctrlbrk(quit_bsx);
  done = 0;
  init_display();
  setcolor(4);
  line(0,257,319,257);
  setcolor(3);
  line(0,340,319,340);
  update_display();
  init_text_window();

  /*
   * Main body of the program.
   * Process all IO here:
   *   o  Server -> client traffic
   *   o  client -> server traffic
   *   o  client -> crt traffic
   *   o  keyboard -> client traffic
   * And all this on a single tasking computer (!)
   * If that does make me a great programmer, nothing will :-)
   * I'll just have to write something without a fork() in it.
   */
  while (!done) {
    c = eat_char();
    if (c=='@') {
      if (debugging) add_char_to_text_window(c);
      command[0]=eat_char();
      command[1]=eat_char();
      command[2]=eat_char();
      command[3]=0;
      if (debugging) add_string_to_text_window(command);
      process_bsx_command(command);
    } else {
      add_char_to_text_window(c);
    }
  }

  close_display();
  disconnect();
}
