#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <ctype.h>

#ifndef FD_ZERO                                       /* For BSD 4.2 systems */
#define DESCR_MASK int
#define FD_ZERO(p) (*p = 0)
#define FD_SET(n,p) (*p |= (1<<(n)))
#define FD_ISSET(n,p) (*p & (1<<(n)))
#else /* FD_ZERO */                                   /* For BSD 4.3 systems */
#define DESCR_MASK fd_set
#endif /* FD_ZERO */

static struct in_addr host_address;
static struct sockaddr_in socket_address;

int mud_socket;   /* static ? */

int connected;
/*---------------------------------------------------------------------------*/
fatal(str) 
char *str;
{
  puts(fatal);
  exit(0);
}

/* Get the address (in the internal format) to the mud host. */
static int get_host_address(name, addr)
char *name;
struct in_addr *addr;
{

    if (*name == '\0') {
        fatal("This cannot happen: No host address specified.");
    }
    else if (isdigit(name[0])) {
	/* Numerical address, e g "130.236.254.13" */
        union {
            long signed_thingy;
            unsigned long unsigned_thingy;
        } thingy;

        addr->s_addr = (unsigned long)inet_addr(name);
        thingy.unsigned_thingy = addr->s_addr;
        if (thingy.signed_thingy == -1)
	    fatal("Couldn't find host '%s'.", name);
    }
    else {
	/* A name address, t ex "nanny.lysator.liu.se" */
        struct hostent *the_host_entry_p;

        if ((the_host_entry_p = gethostbyname(name)) == NULL)
	    fatal("Couldn't find host '%s'.", name);
        bcopy(the_host_entry_p->h_addr, (char *)addr, sizeof(struct in_addr));
    }
    return 0;
} /* get_host_address */

/* Create a socket and connect it to mud. */
int connect_to_mud(host_string, port_string)
char *host_string, *port_string;
{

    if (get_host_address(host_string, &host_address) != 0)
        fatal("connect_to_mud: This shouldn't happen.\n");
    socket_address.sin_addr.s_addr = host_address.s_addr;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(atoi(port_string));
    if ((mud_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fatal("Couldn't open socket.");
    if (connect(mud_socket, (struct sockaddr *)&socket_address,
		sizeof(struct sockaddr_in)) < 0) {
        fatal("Couldn't connect to socket.");
    }
#ifdef FNDELAY
    fcntl(mud_socket, F_SETFL, FNDELAY);
#endif

    connected = 1;
    return 0;
} /* connect_to_mud */

int disconnect()
{
    connected = 0;
    return close(mud_socket);
}
