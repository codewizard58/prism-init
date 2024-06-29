/* Defines the TCP/IP connection routines
 */
#ifndef TCPIP
#define TCPIP
#endif

#ifndef _TCPIP_
#define _TCPIP_

extern int tcp_init(PROTO(int));
extern int tcp_check();
extern int tcp_open(PROTO2(int, struct netinfo *));
extern int tcp_close(PROTO2(int, struct netinfo *));
extern int tcp_read(PROTO4(int, struct netinfo *, char *, int));
extern int tcp_write(PROTO4(int, struct netinfo *, char *, int));
extern int tcp_select(PROTO2( int *, int *));
extern int tcp_shut(PROTO2(int, struct netinfo *));
extern char *tcp_name(PROTO2(int, struct netinfo *));
extern int tcp_term();

#define TCPIP_DEVICES {tcp_check, tcp_init, tcp_open, tcp_close, tcp_read, tcp_write, tcp_select, tcp_shut, tcp_name, tcp_term },

#endif
