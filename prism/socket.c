/* socket.c
27/12/92:	pjc
	Connection stuff put into connect.c
	This should be TCP/IP connection stuff only.
 */

#ifndef MSDOS
#ifndef TCPIP
#define TCPIP
#endif
#else
#include "defs.h"
#endif

#include <stdio.h>
#include "net.h"

#ifdef TCPIP

#include <sys/types.h>
#ifdef MSDOS
#ifndef WINSOCK
#include <sys/nfs_time.h>
#include <sys/ioctl.h>
#endif
#else
#include <sys/ioctl.h>
#endif

#ifdef MSDOS

#ifdef WINSOCK
#include <sys/time.h>

#endif

#else
#include <sys/time.h>
#include <sys/file.h>
#include <sys/wait.h>
#endif	/* MSDOS */

#include <sys/signal.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <ctype.h>

#ifndef WINSOCK
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef DECNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif


#include "db.h"
#include "interfac.h"
#include "config.h"
#include "externs.h"

#include "os.h"

#include "tcpcoms.h"
#include "misc.h"

extern char *mud_name(PROTO(int));

#include "mud_set.h"

#ifdef WINSOCK
#define perror win_perror

typedef int (PASCAL FAR *xxfn)(WORD, LPWSADATA);
typedef	int (PASCAL FAR *xxcfn)(void);
typedef int (PASCAL FAR *xxsfn)(SOCKET, const char FAR *, int, int);
typedef int (PASCAL FAR *xxpfn)(SOCKET, struct sockaddr FAR *, int FAR *);
u_short (PASCAL FAR *lpfnHToNS)(u_short);
u_long (PASCAL FAR *lpfnHToNL)(u_long);
int (PASCAL FAR *lpfnIOCtlSocket)(SOCKET, long, u_long FAR *);
int (PASCAL FAR *lpfnBind)(SOCKET, const struct sockaddr FAR *, int);
int (PASCAL FAR *lpfnCloseSocket)(SOCKET);
u_short (PASCAL FAR *lpfnNToHS)(u_short);
u_long (PASCAL FAR *lpfnNToHL)(u_long);

xxcfn lpfnWSACleanup = NULL;
xxsfn lpfnSend = NULL;
xxpfn lpfnGetPeerName = NULL;
xxcfn lpfnWSAGetLastError;
xxsfn lpfnRecv;
xxpfn lpfnAccept;
int (PASCAL FAR *lpfnSetSockOpt)(SOCKET, int, int, const char FAR *, int);
SOCKET (PASCAL FAR *lpfnSocket)(int, int, int);
int (PASCAL FAR *lpfnListen)(SOCKET, int);
int (PASCAL FAR *lpfnSelect)(int, fd_set FAR *, fd_set FAR *, fd_set FAR *, const struct timeval FAR *);
struct hostent FAR *(PASCAL FAR *lpfnGetHostByAddr)(const char FAR *,int,int);
int (PASCAL FAR *lpfnFDIsSet)(SOCKET, fd_set FAR *);
#ifdef FD_ISSET
#undef FD_ISSET
#endif
#define FD_ISSET(a,b) (*lpfnFDIsSet)(a,b)
extern char winsockname[];
HINSTANCE hinstWS;


#endif

extern struct descriptor_data *descriptor_list ;

extern SOCKET sock;
extern int ndescriptors ;
extern int avail_descriptors;
extern int maxd;

#ifndef EINTR
#define EINTR -99	/* pjc not a real value... */
#endif


#ifndef MSC
extern int	errno;
#endif

extern int notcp;
extern int httpsock;
int realhttpsock;

#ifdef WINSOCK
static SOCKET sockarray[64];

#define ConOf(a)  sockarray[a]	/* winsock does not use ints so have a table */
#define ConOfx(a) (a)
#define BAD_SOCKET(a) ( a == INVALID_SOCKET)
#else

#define ConOf(a)  a		/* Just use the int */
#define BAD_SOCKET(a)  (a < 0)
#endif

static int tcp_init_done;

/* Was in interfac.c */

void make_nonblocking(s)
SOCKET s;
{
#ifndef MSDOS
    if (fcntl (s, F_SETFL, FNDELAY) == -1) {
	perror ("make_nonblocking: fcntl");
	panic ("FNDELAY fcntl failed");
    }
#else

#ifdef TCPIP
#ifndef WINSOCK
	int arg = 1;	/* 1 = non blocking, 0 = blocking */
	tk_ioctl(s, FIONBIO, &arg);
#else
	u_long arg = 1;	/* 1 = non blocking, 0 = blocking */
	(*lpfnIOCtlSocket)(s, FIONBIO, &arg);	
#endif

#endif
#endif
}


struct descriptor_data *initializesock( );
char *getpeer();

const char *addrout( a)
long a;
{
    static char buf[1024];

    sprintf (buf, "%d.%d.%d.%d", (a >> 24) & 0xff, (a >> 16) & 0xff,
	     (a >> 8) & 0xff, a & 0xff);
    return buf;
}


SOCKET make_socket( port)
int port;
{
    SOCKET s;
    int opt;
    struct sockaddr_in server;
                                
	if( notcp){
		return INVALID_SOCKET;
	}
#ifdef WINSOCK
    s = (*lpfnSocket) (PF_INET, SOCK_STREAM, 0);
    if ( BAD_SOCKET(s) ) {
		errno = (*lpfnWSAGetLastError)();
		fprintf(logfile,"creating stream socket - %d",errno);
    }
    opt = 1;
    if ((*lpfnSetSockOpt) (s, SOL_SOCKET, SO_REUSEADDR,
		    (char *) &opt, Sizeof (opt)) < 0) {
		errno = (*lpfnWSAGetLastError)();
		fprintf(logfile,"setsockopt - %d",errno);
    }
#else
    s = socket (AF_INET, SOCK_STREAM, 0);
    if ( BAD_SOCKET(s) ) {
		perror ("creating stream socket");
		exit (3);  
    }
    opt = 1;

    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		    (char *) &opt, Sizeof (opt)) < 0) {
		perror ("setsockopt");
		exit (1);
    }
#endif
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
#ifdef WINSOCK
    server.sin_port = (*lpfnHToNS) (port);
    if ((*lpfnBind) (s, (struct sockaddr *) & server, Sizeof (server))) {
		perror ("binding stream socket");
		(*lpfnCloseSocket)(s);
		return INVALID_SOCKET;
#else
    server.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) & server, Sizeof (server))) {
		perror ("binding stream socket");
		close (s);
		exit (4);
#endif
    }
#ifdef WINSOCK
    (*lpfnListen) (s, 5);
#else
	listen(s, 5);
#endif
    return s;
}


/* return allocated string containing name of host */

char *getpeer(s)
SOCKET s;
{
	struct sockaddr_dn *daddr;
	struct nodeent *peernode;
	struct sockaddr_in *xbuffer;
	char buffer[256];
	int peernamelength, error, i;
	int status;
	static char node[256], object[256];
	int ol=255, nl=255;
	struct hostent *hp;

	node[0] = object[0] = '\0';
	error = 0;

	peernamelength = sizeof(struct sockaddr);
#ifdef WINSOCK
	status = (*lpfnGetPeerName)(s, (struct sockaddr *)buffer, &peernamelength);
#else
	status = getpeername(s, (struct sockaddr *)buffer, &peernamelength);
#endif
	if(status == -1){
		error++;
	}else {
		xbuffer = ((struct sockaddr_in *)&buffer);

#ifdef WINSOCK
		hp = (*lpfnGetHostByAddr) ( (char *)&xbuffer->sin_addr, 
			Sizeof( xbuffer->sin_addr), AF_INET);
#else
		hp = gethostbyaddr ( (char *)&xbuffer->sin_addr, 
			Sizeof( xbuffer->sin_addr), AF_INET);
#endif
		if( !hp){
			printf("getnodebyaddr failed\n");
			error++;
		}else {
			strcpy(node,hp->h_name) ;
		}
	}
	if(error)
		strcpy(node, "UNKNOWN");
	return alloc_string(node);
}

static SOCKET tcp_socket;
static int dev_index = -3;
extern int tcp_port;

int tcp_reinit()
{	if( notcp == 0){
		tcp_term();
	}
	tcp_init_done = 0;
	notcp = 0;
	
	return tcp_init(dev_index);
}

int tcp_init(dev_num)
int dev_num;
{	int n;

	if( tcp_init_done)
		return 0;

#ifdef MSDOS
#ifndef WINSOCK
	if( !is_pc_nfs_installed()){
		notcp = 1;
	}
#else
	{	int err=0;
		WSADATA lpWSAData;
		xxfn lpfnWSAStartup = NULL;
		FARPROC x;
        
        if( winsockname[0] == '\0'){
        	fprintf(logfile,"No WINSOCK.DLL configured\n");
        	notcp = 1;
        	goto done;
        }
        
		SetErrorMode(SEM_NOOPENFILEERRORBOX);

		hinstWS = LoadLibrary(winsockname);
		if( (UINT) hinstWS <= 32){
			notcp = 1;
			fprintf(logfile,"Failed to load module %s\n", winsockname);
			goto done;
		}
	
		lpfnWSAStartup = (xxfn)GetProcAddress(hinstWS, MAKEINTRESOURCE(115));
	
		x = GetProcAddress(hinstWS, MAKEINTRESOURCE(116));
		lpfnWSACleanup = (xxcfn)x;
	
		if( lpfnWSAStartup != NULL){
			int res;
		
			res = (*lpfnWSAStartup)((WORD)0x101, &lpWSAData);
		
			if( res != 0){
				fprintf(logfile,"WSAStartup returned %d\n", res);
				notcp = 1;
				goto done;
			}
		}else{
			fprintf(logfile,"Failed to locate WSAStartup\n");
			notcp = 1;
			goto done;
		}	

		((FARPROC)lpfnAccept) = GetProcAddress(hinstWS, MAKEINTRESOURCE(1));
		((FARPROC)lpfnBind) = GetProcAddress(hinstWS, MAKEINTRESOURCE(2));
		((FARPROC)lpfnCloseSocket) = GetProcAddress(hinstWS, MAKEINTRESOURCE(3));

		((FARPROC)lpfnGetPeerName) = GetProcAddress(hinstWS, MAKEINTRESOURCE(5));

		((FARPROC)lpfnHToNL) = GetProcAddress(hinstWS, MAKEINTRESOURCE(8));
		((FARPROC)lpfnHToNS) = GetProcAddress(hinstWS, MAKEINTRESOURCE(9));

		((FARPROC)lpfnIOCtlSocket) = GetProcAddress(hinstWS, MAKEINTRESOURCE(12));
		((FARPROC)lpfnListen) = GetProcAddress(hinstWS, MAKEINTRESOURCE(13));
		((FARPROC)lpfnNToHL) = GetProcAddress(hinstWS, MAKEINTRESOURCE(14));
		((FARPROC)lpfnNToHS) = GetProcAddress(hinstWS, MAKEINTRESOURCE(15));
		((FARPROC)lpfnRecv) = GetProcAddress(hinstWS, MAKEINTRESOURCE(16));

		((FARPROC)lpfnSelect) = GetProcAddress(hinstWS, MAKEINTRESOURCE(18));
		((FARPROC)lpfnSend) = GetProcAddress(hinstWS, MAKEINTRESOURCE(19));
		((FARPROC)lpfnSetSockOpt) = GetProcAddress(hinstWS, MAKEINTRESOURCE(21));

		((FARPROC)lpfnSocket) = GetProcAddress(hinstWS, MAKEINTRESOURCE(23));

		((FARPROC)lpfnGetHostByAddr) = GetProcAddress(hinstWS, MAKEINTRESOURCE(51));

		((FARPROC)lpfnWSAGetLastError) = GetProcAddress(hinstWS, MAKEINTRESOURCE(111));
		((FARPROC)lpfnFDIsSet) = GetProcAddress(hinstWS, MAKEINTRESOURCE(151));

		if( err != 0){
			notcp = 1;
		}

		{	int n;
			for(n=0; n < 64; n++)
				sockarray[n] = INVALID_SOCKET;
		}
		fprintf(logfile,"WINSOCK initialized\n");
	}
#endif
#endif

done:
	tcp_init_done = 1;

	if( notcp){
		return 0;
	}
	dev_index = dev_num;
	tcp_socket= make_socket(tcp_port);

	if(BAD_SOCKET(tcp_socket)){
		notcp = 1;
	}else{
		int nnew;
		
		n = make_socket(tcp_port+1);
#ifdef WINSOCK
		for(nnew = 1; nnew < 32; nnew++){
			if(sockarray[nnew] == n){
					break;
			}
		}
		if( nnew == 32){
		  for(nnew = 1; nnew < 32; nnew++){
			if( BAD_SOCKET(sockarray[nnew])){
				sockarray[nnew] = n;
				break;
			}
		  }
		}
#else
		nnew = n;
#endif
		net_new_con(dev_index, nnew);
		realhttpsock = nnew;
		httpsock = mud_open();
	}
	return 0;
}


int tcp_check()
{	SOCKET n;
	int nnew;
	fd_set input_set, output_set;
	struct sockaddr_in addr;
	int addr_len;
	struct timeval timeout;

	if(notcp)
		return 0;

	FD_ZERO (&input_set);
	FD_ZERO (&output_set);
	
	if( tcp_socket == INVALID_SOCKET){
		fprintf(stderr,"tcp_socket is INVALID");
		return 0;
	}

#ifndef MSDOS
	if (ndescriptors < avail_descriptors){
	    FD_SET (tcp_socket, &input_set);
	}
#else
	    FD_SET (tcp_socket, &input_set);
#endif

	timeout.tv_sec = 0l;
	timeout.tv_usec= 0l;

	maxd = 32;

#ifndef WINSOCK
	if( maxd <= tcp_socket)
		maxd = tcp_socket+1;
#endif

#ifdef WINSOCK
	if ((*lpfnSelect) (maxd, &input_set, &output_set,
		    (fd_set *) NULL, &timeout) < 0) {
	    if (errno != EINTR) {
			errno = (*lpfnWSAGetLastError)();
#else
	if (select(maxd, &input_set, &output_set,
		    (fd_set *) NULL, &timeout) < 0) {
	    if (errno != EINTR) {
#endif
	    	fprintf(stderr,"select tcp-check %d", errno);
			return 0;
	    }

	}

	if( !(FD_ISSET(tcp_socket, &input_set))){
		return 0;
	}

/* Do the listen / accept stuff. */

	addr_len = Sizeof (addr);
#ifdef WINSOCK
	n = (*lpfnAccept) (tcp_socket, (struct sockaddr *) & addr, &addr_len);
#else
	n = accept(tcp_socket, (struct sockaddr *) & addr, &addr_len);
#endif
	if (BAD_SOCKET(n)) {
		fprintf (stderr, "ACCEPT failed!\n");
		return 0;
	} else {
#ifdef WINSOCK
		fprintf (stderr, "ACCEPT from %s(%d) on descriptor %d\n", addrout ((*lpfnNToHL) (addr.sin_addr.s_addr)), (*lpfnNToHS) (addr.sin_port), n);
#else
		fprintf (stderr, "ACCEPT from %s(%d) on descriptor %d\n", addrout (ntohl(addr.sin_addr.s_addr)), ntohs(addr.sin_port), n);
#endif
	}

#ifdef WINSOCK
	for(nnew = 1; nnew < 32; nnew++){
		if(sockarray[nnew] == n){
				break;
		}
	}
	if( nnew == 32)
	  for(nnew = 1; nnew < 32; nnew++){
		if( BAD_SOCKET(sockarray[nnew])){
			sockarray[nnew] = n;
			break;
		}
	}
#else
	nnew = n;
#endif	

	make_nonblocking(n);

/* Queue the request to start another session */

	net_new_con(dev_index, nnew);
	return 1;
}

int tcp_open(fd, net)
int fd;
struct netinfo *net;
{
	return net->data;
}

int tcp_close(fd, net)
int fd;
struct netinfo *net;
{
#ifndef WINSOCK
	close(net->data);
#else
	(*lpfnCloseSocket)(ConOf(net->data));
	sockarray[net->data] = INVALID_SOCKET;	/* Mark as free */
#endif
	return -1;
}

int tcp_read(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{	int ret;

#ifdef WINSOCK
	ret = (*lpfnRecv)(ConOf(net->data), buf, cnt, 0);
	if( ret < 0)
		errno = (*lpfnWSAGetLastError)();
#else
	ret = recv(ConOf(net->data), buf, cnt, 0);
#endif

	return ret;
}

int tcp_write(fd, net, buf, cnt)
int fd;
struct netinfo *net;
char *buf;
int cnt;
{	int ret;
	int e = 0;

#ifdef WINSOCK
	ret = (*lpfnSend)(ConOf(net->data), buf, cnt, 0);
	if( ret < 0)
		errno = (*lpfnWSAGetLastError)();
#else
	ret = send(ConOf(net->data), buf, cnt, 0);
#endif

	if( ret < 0){
		e = errno;
	}
	return ret;
}

int tcp_select(in, out)
mud_set *in, *out;
{	struct timeval timeout;
	int n,net;
	fd_set input_set, output_set;
	int mask=0, cmask=0;
	mud_set savedin, savedout;
	int maxd = 0;
	
	savedin = *in;
	savedout= *out;

	FD_ZERO(&input_set);
	FD_ZERO(&output_set);

	for(n=0; n< NCONNECTIONS; n++){
		if( notcp == 0 && net_connections[n] &&
			net_connections[n]->type == dev_index){
			net = net_connections[n]->data;

			if( maxd <= net)
				maxd = net+1;

			if( in && MUD_ISSET(n, in) ){
				mask |= 1<<n;
				FD_SET(ConOf(net), &input_set);
				cmask|= 1<< ConOf(net);
			}

			if( out && MUD_ISSET(n, out) ){
				FD_SET(ConOf(net), &output_set);
			}


		}else {
			if( in)
				MUD_CLR(n, in);
			if( out)
				MUD_CLR(n, out);
		}
	}
	if(notcp){
		return 0;
	}

#ifdef WINSOCK
	timeout.tv_sec = 0l;
	timeout.tv_usec = 0l;

	if ((*lpfnSelect) (maxd, &input_set, &output_set,
		    (fd_set *) 0, &timeout) < 0) {
#else
	timeout.tv_sec = 2l;
	timeout.tv_usec = 0l;

	if (select(maxd, &input_set, &output_set,
		    (fd_set *) 0, &timeout) < 0) {
#endif
#ifndef MSDOS
	    if (errno != EINTR) {
		perror ("select");
		return 0;
	    }
#endif
	}
	
	for(n=0; n< NCONNECTIONS; n++){
		if( net_connections[n] &&
			net_connections[n]->type == dev_index){
			net = net_connections[n]->data;

			if( !(FD_ISSET(ConOf(net), &input_set)) && in ){
				MUD_CLR(n, in);
			}
			if( !(FD_ISSET(ConOf(net), &output_set)) && out){
				MUD_CLR(n, out);
			}
		}
	}

	return 0;
}

char *tcp_name(fd, net)
int fd;
struct netinfo *net;
{
	return getpeer(net->data);
}

int tcp_shutdown_net(fd, net)
int fd;
struct netinfo *net;
{
#ifdef WINSOCK
	if( tcp_init_done){
		if( lpfnWSACleanup)
			(*lpfnWSACleanup)();
			FreeLibrary(hinstWS);
		tcp_init_done = 0;
	}
	return 0;
#else
	if( !net || !tcp_init_done){
		return 0;
	}
	tcp_init_done = 0;

	return 0;
#endif
}

int tcp_term()
{
#ifdef WINSOCK
	if( tcp_init_done){
		if( lpfnWSACleanup)
			(*lpfnWSACleanup)();
		FreeLibrary(hinstWS);
		tcp_init_done = 0;
	}
#endif
	return 0;
}



int tcp_shut(fd, net, mode)
int fd;
struct netinfo *net;
int mode;
{
	return 0;
}

/* new accept code */

int tcp_accept(fd)
int fd;
{	int addr_len;
	int n, nnew;
	struct sockaddr_in addr;
	int sock = ConOf(fd);

	addr_len = Sizeof (addr);
#ifdef WINSOCK
	n = (*lpfnAccept) (sock, (struct sockaddr *) & addr, &addr_len);
#else
	n = accept(sock, (struct sockaddr *) & addr, &addr_len);
#endif
	if (BAD_SOCKET(n)) {
		fprintf (stderr, "ACCEPT failed!\n");
		return 0;
	}

#ifdef WINSOCK
	for(nnew = 1; nnew < 32; nnew++){
		if(sockarray[nnew] == n){
				break;
		}
	}
	if( nnew == 32)
	  for(nnew = 1; nnew < 32; nnew++){
		if( BAD_SOCKET(sockarray[nnew])){
			sockarray[nnew] = n;
			break;
		}
	}
#else
	nnew = n;
#endif	

/* Queue the request to start another session */
	make_nonblocking(n);

	net_new_con(dev_index, nnew);
	nnew = mud_open();
    return nnew;
}


#endif	/* TCPIP */
