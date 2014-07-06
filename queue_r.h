/*
 * queue_r.h
 *
 * Thread-safe, reentrant, double-sided queue implementation.
 *
 *      Author: Fabian Foerg
 */

#ifndef QUEUE_R_H_
#define QUEUE_R_H_

/**
 * Node of a queue data structure.
 */
typedef struct node_struct {
	void *object;
	struct node_struct *prev;
	struct node_struct *next;
} node;

/**
 * Thread-safe queue data structure.
 * The variables lock, write_cond, and
 * read_cont must be initialized and destroyed
 * manually.
 */
typedef struct queue_r_t_struct {
	node *head;
	node *tail;
	// synchronization objects
	pthread_mutex_t lock;
	pthread_cond_t write_cond;
	pthread_cond_t read_cond;
	int readers;
	int destroyed;
} queue_r_t;

int q_init(queue_r_t *);
int q_push_front(queue_r_t *, void *object);
int q_push_back(queue_r_t *, void *object);
void *q_pop_front(queue_r_t *);
void *q_pop_back(queue_r_t *);
int q_traverse(queue_r_t *, void (*traverse)(void * object));
void q_deinit(queue_r_t *);

#endif /* QUEUE_R_H_ */
