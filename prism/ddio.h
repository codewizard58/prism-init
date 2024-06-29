/* ddio.h
 */
 
struct queue {
struct queue *next, *prev;
char	*data;
int		len;
};

struct sink {
struct sink *next, *prev;
struct queue *qhead;
int		(*s_func)();
int		(*s_free)();
char	*tdata;
};

typedef struct queue Queue;
typedef struct sink Sink;

Sink *new_sink();
void free_sink();

Queue *new_queue();
void free_queue();

