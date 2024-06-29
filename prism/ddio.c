/* ddio.c
 */

#include "ddio.h"

Queue	*q_freelist;
Sink	*s_freelist;

Queue *new_queue()
{	Queue *n;

	n = q_freelist;
	if( n != (Queue *)0){
		q_freelist = n->next;
	}else {
		n = (Queue *)malloc( sizeof (Queue));
		n->data = (char *)malloc( 8192);
	}
	n->next = n->prev = (Queue *)0;
	n->len = 0;
	
	return n;
}

void free_queue(q)
Queue *q;
{	if( q == (Queue *)0)
		return;
	q->next = q_freelist;
	q_freelist = q;
}


int sf_throw(s)
Sink *s;
{	Queue *q1;

	q1 = s->qhead;
	while(q1 != (Queue *)0){
		     s->qhead = q1->next;
		     free_queue(q1);
		     q1 = s->qhead;
	}
	return 0;
}


Sink *new_sink()
{	Sink *n;

	n = (Sink *)malloc( sizeof(Sink) );
	
	n->next = n->prev = (Sink *)0;
	n->qhead = (Queue *)0;
	n->s_func = sf_throw;
	n->s_free = (void *)0;
	n->tdata = (char *)0;
	
	return n;
}


void free_sink(s)
Sink *s;
{	if( s == (Sink *)0)
		return;
	if( s->s_free != (void *)0){
		s->s_free(s);
	}
	if( s->qhead){
		sf_throw(s);
	}
	
	s->next = s_freelist;
	s_freelist = s;
}

