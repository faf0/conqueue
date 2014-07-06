/*
 * queue_r.c
 *
 * Thread-safe, reentrant, double-sided queue implementation.
 *
 *      Author: Fabian Foerg
 */

#include <pthread.h>
#include <stdlib.h>

#include "queue_r.h"

/**
 * Releases a read lock of the given queue.
 * A read lock must have been acquired before.
 */
static void read_unlock(queue_r_t *queue) {
	pthread_mutex_lock(&queue->lock);
	queue->readers--;
	if (queue->readers == 0) {
		// informing the writers is enough, since readers
		// can re-enter the lock
		pthread_cond_broadcast(&queue->write_cond);
	}
	pthread_mutex_unlock(&queue->lock);
}

/**
 * Acquires a read lock on the given queue.
 */
static void read_lock(queue_r_t *queue) {
	pthread_mutex_lock(&queue->lock);
	// wait until there is a queue head or the queue is destroyed
	while (!queue->head && !queue->destroyed) {
		pthread_cond_wait(&queue->read_cond, &queue->lock);
	}
	queue->readers++;
	pthread_mutex_unlock(&queue->lock);
}

/**
 * Releases the write lock of the given queue.
 * A write lock must have been acquired before.
 */
static void write_unlock(queue_r_t *queue) {
	// inform readers and writers, since both readers and
	// writers may wait conditionally on writers to finish
	pthread_cond_broadcast(&queue->read_cond);
	pthread_cond_broadcast(&queue->write_cond);
	pthread_mutex_unlock(&queue->lock);
}

/**
 * Acquires a write lock.
 * Waits for readers to finish or the queue to become deinitialized.
 */
static void write_lock_push(queue_r_t *queue) {
	pthread_mutex_lock(&queue->lock);
	// wait until there are no readers or the queue is destroyed
	while (queue->readers && !queue->destroyed) {
		pthread_cond_wait(&queue->write_cond, &queue->lock);
	}
}

/**
 * Acquires a write lock.
 * Waits until the queue has at least one element and waits for
 * readers to finish. When the queue is deinitialized, the method
 * returns.
 */
static void write_lock_pop(queue_r_t *queue) {
	pthread_mutex_lock(&queue->lock);
	// wait until there are items and no readers, or the queue is destroyed
	while ((!queue->head || queue->readers) && !queue->destroyed) {
		pthread_cond_wait(&queue->write_cond, &queue->lock);
	}
}

/**
 * Creates the head and tail node of the given empty queue.
 */
static void create_first_node(queue_r_t *queue, void *object) {
	if (!queue || !object || queue->head || queue->tail) {
		return;
	}

	// create head
	queue->head = (node *) malloc(sizeof(node));
	queue->head->object = object;
	queue->head->prev = NULL;
	queue->head->next = NULL;

	// tail node is the same as head node
	queue->tail = queue->head;
}

/**
 * Initializes the queue and returns 0 on success.
 * If the operation fails, a non-zero value will be
 * returned.
 * The variables lock, write_cond, and
 * read_cont must be initialized manually before
 * usage.
 */
int q_init(queue_r_t *queue) {
	if (!queue) {
		return EXIT_FAILURE;
	}

	// assuming that the given queue is empty
	queue->head = NULL;
	queue->tail = NULL;
	queue->readers = 0;
	queue->destroyed = 0;

	return EXIT_SUCCESS;
}

/**
 * Adds the given object to the front of the queue.
 * Returns 0 on success and a non-zero value otherwise.
 */
int q_push_front(queue_r_t *queue, void *object) {
	int return_value = EXIT_SUCCESS;

	if (!queue || !object || queue->destroyed) {
		return EXIT_FAILURE;
	}

	write_lock_push(queue);
	if (queue->destroyed) {
		return_value = EXIT_FAILURE;
	} else if (!queue->head) {
		create_first_node(queue, object);
	} else {
		// create new node
		node *new_node = (node *) malloc(sizeof(node));

		if (new_node) {
			new_node->object = object;
			new_node->prev = NULL;
			new_node->next = queue->head;

			// change head
			queue->head->prev = new_node;
			queue->head = new_node;
		} else {
			return_value = EXIT_FAILURE;
		}
	}
	write_unlock(queue);

	return return_value;
}

/**
 * Adds the given object to the front of the queue.
 * Returns 0 on success and a non-zero value otherwise.
 */
int q_push_back(queue_r_t *queue, void *object) {
	int return_value = EXIT_SUCCESS;

	if (!queue || !object || queue->destroyed) {
		return EXIT_FAILURE;
	}

	write_lock_push(queue);
	if (queue->destroyed) {
		return_value = EXIT_FAILURE;
	} else if (!queue->tail) {
		create_first_node(queue, object);
	} else {
		// create new node
		node *new_node = (node *) malloc(sizeof(node));

		if (new_node) {
			new_node->object = object;
			new_node->prev = queue->tail;
			new_node->next = NULL;

			// change head
			queue->tail->next = new_node;
			queue->tail = new_node;
		} else {
			return_value = EXIT_FAILURE;
		}
	}
	write_unlock(queue);

	return return_value;
}

/**
 * Removes the front node from the queue and returns
 * its object.
 * Waits until an element is added to the queue or the
 * queue is deinitialized.
 * NULL is returned, if the operation failed.
 */
void *q_pop_front(queue_r_t *queue) {
	void *front_object = NULL;

	if (!queue || queue->destroyed) {
		return NULL;
	}

	write_lock_pop(queue);
	if (!queue->destroyed) {
		front_object = queue->head->object;

		if (queue->head == queue->tail) {
			free(queue->head);
			queue->head = NULL;
			queue->tail = NULL;
		} else {
			node *head_next = queue->head->next;
			free(queue->head);
			head_next->prev = NULL;
			queue->head = head_next;
		}
	}
	write_unlock(queue);

	return front_object;
}

/**
 * Removes the back node from the queue and returns
 * its object.
 * Waits until an element is added to the queue or the
 * queue is deinitialized.
 * NULL is returned, if the operation failed.
 */
void *q_pop_back(queue_r_t *queue) {
	void *back_object = NULL;

	if (!queue || queue->destroyed) {
		return NULL;
	}

	write_lock_pop(queue);
	if (!queue->destroyed) {
		back_object = queue->tail->object;

		if (queue->head == queue->tail) {
			free(queue->tail);
			queue->head = NULL;
			queue->tail = NULL;
		} else {
			node *tail_prev = queue->tail->prev;
			free(queue->tail);
			tail_prev->next = NULL;
			queue->tail = tail_prev;
		}
	}
	write_unlock(queue);

	return back_object;
}

/**
 * Traverses the queue from front to back and calls traverse
 * on each traversed object.
 */
int q_traverse(queue_r_t *queue, void (*traverse)(void * object)) {
	int result = EXIT_SUCCESS;

	if (!queue || !traverse || queue->destroyed) {
		return EXIT_FAILURE;
	}

	read_lock(queue);
	if (!queue->destroyed) {
		for (node *current = queue->head; current; current = current->next) {
			traverse(current->object);
		}
	} else {
		result = EXIT_FAILURE;
	}
	read_unlock(queue);

	return result;
}

/**
 * Deinitializes the queue.
 * The variables lock, write_cond, and
 * read_cont must be destroyed manually.
 */
void q_deinit(queue_r_t *queue) {
	if (!queue || queue->destroyed) {
		return;
	}

	// free node space, but not object space
	// set pointers to NULL
	write_lock_push(queue);
	if (!queue->destroyed) {
		node* next;

		for (node *current = queue->head; current; current = next) {
			next = current->next;
			free(current);
		}

		queue->head = NULL;
		queue->tail = NULL;
		queue->destroyed = 1;
	}
	write_unlock(queue);
}
