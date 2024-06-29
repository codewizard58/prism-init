#define NULL (void *) 0

extern char logging;

unsigned char slurp_char() {
  while (!kbhit());
  return (getch());
}

unsigned char suck_keyboard() {
  unsigned char c;
  while (!kbhit())
    return;
  return (c=getch());
}

char key_strokes[1000];
int fill=0;

void spit_char() {
  unsigned char c;

  c = suck_keyboard();
  if (c==8) {
    /* Backspace */
    if (fill)
      fill--;
  } else {
    /* Normal char -> store it in a buffer */
    key_strokes[fill++] = c;
    if (c=='\n') {
      /* Go flush it! */
      puts(fill);
      fill = 0;
    }
  }
}

unsigned char eat_char() {
  unsigned char t;

  t=slurp_char();
  return t;
}

unsigned char eat_raw_char() {
  unsigned char t;

  t=slurp_char();
  return t;
}

unsigned char old_eat_hex() {       /* Read server data and convert hex to dec */
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

tell_server(mesg)
char *mesg;
{
}

disconnect() {
}

