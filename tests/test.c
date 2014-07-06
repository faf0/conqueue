/*
 * test.c
 *
 * Test program for the queue_r library.
 *
 *      Author: Fabian Foerg
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "queue_r.h"

/**
 * Number of objects for the sequential test.
 */
#define SEQ_VALUES 1024
/**
 * Number of threads to spawn
 * for the concurrent test.
 * Must not change this constant
 * without adapting the code.
 */
#define NUMBER_THREADS 10
/**
 * Time to sleep in seconds before the main thread
 * deinitializes the queues that multiple threads
 * access during the concurrent test.
 */
#define SLEEP_TIME 1
/**
 * Number of objects for the performance test.
 */
#define PERF_VALUES 123456
/**
 * Number of test runs for the performance test.
 * The result values are averaged over the test
 * runs.
 */
#define PERF_RUNS 40

/**
 * Data structure that is passed to the threads.
 * It contains pointers to a queue and an integer
 * object.
 */
typedef struct thread_arg_struct {
	queue_r_t *queue;
	int *values;
} thread_arg;

/**
 * Initializes the synchronization objects of the
 * given queue. Must be called before any threads
 * access the queue.
 */
static void queue_sync_init(queue_r_t *queue) {
	pthread_mutex_init(&queue->lock, NULL);
	pthread_cond_init(&queue->write_cond, NULL);
	pthread_cond_init(&queue->read_cond, NULL);
}

/**
 * Destroys the synchronization objects of the given
 * queue. Threads accessing the given queue must have
 * finished before this method is called.
 */
static void queue_sync_destroy(queue_r_t *queue) {
	pthread_cond_destroy(&queue->read_cond);
	pthread_cond_destroy(&queue->write_cond);
	pthread_mutex_destroy(&queue->lock);
}

/**
 * Thread function that pushes all given values to the
 * front of the given queue.
 */
static void *push_front_queue(void *arg) {
	thread_arg *cast_arg = (thread_arg *) arg;
	for (int i = 0; i < SEQ_VALUES; i++) {
		q_push_front(cast_arg->queue, (void *) &cast_arg->values[i]);
	}
	return NULL;
}

/**
 * Thread function that pushes all given values to the
 * back of the given queue.
 */
static void *push_back_queue(void *arg) {
	thread_arg *cast_arg = (thread_arg *) arg;
	for (int i = 0; i < SEQ_VALUES; i++) {
		q_push_back(cast_arg->queue, (void *) &cast_arg->values[i]);
	}
	return NULL;
}

/**
 * Thread function that attempts to pop all objects from the
 * front of the given queue.
 */
static void *pop_front_queue(void *arg) {
	thread_arg *cast_arg = (thread_arg *) arg;
	for (int i = 0; i < SEQ_VALUES; i++) {
		int *value = (int *) q_pop_front(cast_arg->queue);
		if (value) {
			printf("popped front value: %d\n", *value);
		}
	}
	return NULL;
}

/**
 * Thread function that attempts to pop all objects from the
 * back of the given queue.
 */
static void *pop_back_queue(void *arg) {
	thread_arg *cast_arg = (thread_arg *) arg;
	for (int i = 0; i < SEQ_VALUES; i++) {
		int *value = (int *) q_pop_back(cast_arg->queue);
		if (value) {
			printf("popped back value: %d\n", *value);
		}
	}
	return NULL;
}

/**
 * Prints the integer that object points to.
 */
static void print_int_object(void *object) {
	if (object) {
		int *int_object = (int *) object;
		printf("traversed object: %d\n", *int_object);
	}
}

/**
 * Thread function that traverses the given queue and
 * prints its integer objects.
 */
static void *traverse(void *arg) {
	thread_arg *cast_arg = (thread_arg *) arg;
	q_traverse(cast_arg->queue, &print_int_object);
	return NULL;
}

/**
 * Runs a sequential (non-concurrent), concurrent, and
 * performance test.
 */
int main(int argc, char *argv[]) {
	int values[SEQ_VALUES];
	queue_r_t queue1;
	queue_r_t queue2;
	pthread_t threads[NUMBER_THREADS];
	thread_arg thread_arg[2];
	clock_t start, end;
	double push_front_time = 0.0;
	double push_back_time = 0.0;
	double pop_front_time = 0.0;
	double pop_back_time = 0.0;

	// initialize queue synchronization objects
	queue_sync_init(&queue1);
	queue_sync_init(&queue2);

	// functionality test
	// initialize queues
	q_init(&queue1);
	q_init(&queue2);

	printf("Sequential functionality test\n");

	// push items into queues
	for (int i = 0; i < SEQ_VALUES; i++) {
		values[i] = i + 1;
		q_push_front(&queue1, &values[i]);
		q_push_back(&queue2, &values[i]);
	}

	// traverse queues
	q_traverse(&queue1, &print_int_object);
	q_traverse(&queue2, &print_int_object);

	// pop objects from queues and print them
	for (int i = 0; i < SEQ_VALUES; i++) {
		int *pop_1 = (int *) q_pop_back(&queue1);
		int *pop_2 = (int *) q_pop_front(&queue2);

		printf("popped queue 1 object: %d\tqueue 2 object: %d\n", *pop_1,
				*pop_2);
	}

	// test with multiple threads
	printf("\nConcurrent functionality test\n");

	thread_arg[0].queue = &queue1;
	thread_arg[0].values = values;

	thread_arg[1].queue = &queue2;
	thread_arg[1].values = values;

	// start threads
	for (int i = 0; i < NUMBER_THREADS / 2; i++) {
		int index = 2 * i;

		switch (i) {
		case 0:
			pthread_create(&threads[index], NULL, &traverse, thread_arg);
			pthread_create(&threads[index + 1], NULL, &traverse,
					thread_arg + 1);
			break;

		case 1:
			pthread_create(&threads[index], NULL, &pop_front_queue, thread_arg);
			pthread_create(&threads[index + 1], NULL, &pop_front_queue,
					thread_arg + 1);
			break;

		case 2:
			pthread_create(&threads[index], NULL, &pop_back_queue, thread_arg);
			pthread_create(&threads[index + 1], NULL, &pop_back_queue,
					thread_arg + 1);
			break;

		case 3:
			pthread_create(&threads[index], NULL, &push_front_queue,
					thread_arg);
			pthread_create(&threads[index + 1], NULL, &push_front_queue,
					thread_arg + 1);
			break;

		case 4:
			pthread_create(&threads[index], NULL, &push_back_queue, thread_arg);
			pthread_create(&threads[index + 1], NULL, &push_back_queue,
					thread_arg + 1);
			break;

		default:
			break;
		}
	}

	sleep(SLEEP_TIME);

	// de-initialize queues before joining threads
	q_deinit(&queue1);
	q_deinit(&queue2);

	// join threads
	for (int i = 0; i < NUMBER_THREADS; i++) {
		int error = pthread_join(threads[i], NULL);

		if (error) {
			fprintf(stderr, "A thread did not exit successfully!\n");
		}
	}

	// initialize queues again
	q_init(&queue1);
	q_init(&queue2);

	// performance test
	printf("\nPerformance test\n");

	for (int t = 0; t < PERF_RUNS; t++) {
		// push
		for (int i = 0; i < PERF_VALUES; i++) {
			start = clock();
			q_push_front(&queue1, &values[0]);
			end = clock();
			push_front_time += end - start;
			start = clock();
			q_push_back(&queue2, &values[0]);
			end = clock();
			push_back_time += end - start;
		}

		// pop
		for (int i = 0; i < PERF_VALUES; i++) {
			start = clock();
			q_pop_front(&queue1);
			end = clock();
			pop_front_time += end - start;
			start = clock();
			q_pop_back(&queue2);
			end = clock();
			pop_back_time += end - start;
		}
	}

	push_front_time /= (double) PERF_RUNS;
	push_back_time /= (double) PERF_RUNS;
	pop_front_time /= (double) PERF_RUNS;
	pop_back_time /= (double) PERF_RUNS;

	// print test results
	printf("user + system time for %d repetitions in seconds "
			"(averaged over %d test runs):\n"
			"push front: %lf\tpush back: %lf\t"
			"pop front: %lf\tpop back %lf\n", PERF_VALUES, PERF_RUNS,
			push_front_time / CLOCKS_PER_SEC, push_back_time / CLOCKS_PER_SEC,
			pop_front_time / CLOCKS_PER_SEC, pop_back_time / CLOCKS_PER_SEC);

	// de-initialize queues
	q_deinit(&queue1);
	q_deinit(&queue2);

	// destroy queues
	queue_sync_destroy(&queue1);
	queue_sync_destroy(&queue2);

	return EXIT_SUCCESS;
}
