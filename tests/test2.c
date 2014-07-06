#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "queue_r.h"

#define NUMTESTOPS 1000000 
#define NTHREADS   30

static void queue_sync_init(queue_r_t *queue) {
	pthread_mutex_init(&queue->lock, NULL);
	pthread_cond_init(&queue->write_cond, NULL);
	pthread_cond_init(&queue->read_cond, NULL);
}

static void queue_sync_destroy(queue_r_t *queue) {
	pthread_cond_destroy(&queue->read_cond);
	pthread_cond_destroy(&queue->write_cond);
	pthread_mutex_destroy(&queue->lock);
}

void * test_fn(void *_qptr) {
	queue_r_t *qptr = (queue_r_t *) _qptr;
	int i;
	for (i = 0; i < NUMTESTOPS; i++) {
		int choice = rand();
		int error = 0;
		switch (choice % 4) {
		case 0:
			if (q_push_front(qptr, (void *) &choice))
				error = 1;
			break;
		case 1:
			if (q_push_back(qptr, (void *) &choice))
				error = 1;
			break;
		case 2:
			if (!q_pop_front(qptr))
				error = 1;
			break;
		case 3:
			if (!q_pop_back(qptr))
				error = 1;
			break;
		}
		if (error)
			break;
	}
	return 0;
}

int main() {
	int i;
	queue_r_t queue;
	pthread_t threads[NTHREADS];

	queue_sync_init(&queue);

	if (0 != q_init(&queue)) {
		exit(2);
	}
	for (i = 0; i < NTHREADS; i++) {
		if (0 != pthread_create(threads + i, 0, test_fn, &queue))
			exit(1);
	}

	sleep(10);

	q_deinit(&queue);

	for (i = 0; i < NTHREADS; i++) {
		int error = pthread_join(threads[i], NULL);

		if (error) {
			fprintf(stderr, "A thread did not exit successfully!\n");
		}
	}

	queue_sync_destroy(&queue);

	return 0;
}
