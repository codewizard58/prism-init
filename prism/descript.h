/*
	Descript.h
27-12-92:
	extended to add per user display types.
 */

#ifndef _DESCRIPT_
#define _DESCRIPT_

#include "net.h"

struct text_block {
	struct text_block	*nxt;
	int			nchars;
	char		*start;
	char		*buf;
	dbref		obj;
	int		size;		/* size of block */
};

struct text_queue {
    struct text_block *head;
    struct text_block **tail;
};

struct descriptor_data {
        SOCKET descriptor;
	int connected;
	dbref player;
	char *output_prefix;
	char *output_suffix;
	int output_size;
	struct text_queue output;
	struct text_queue input;
	char *raw_input;
	char *raw_input_at;
	long last_time;
	int quota;
	struct descriptor_data *next;
	struct descriptor_data *prev;
	int  olen;
	char *host;
	int  caps;		/* Capabilities */
	void *client_data;	/* Client type dependant data */
};

extern int queue_string __PROTO((struct descriptor_data *, char *));
extern int queue_write __PROTO((struct descriptor_data *, char *, int));

/* Capabilities defines */
#define CAP_NORMAL	0
#define CAP_BSX		1
#define CAP_HIT		2
#define CAP_MMEDIA	4

#endif
