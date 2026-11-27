#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void *qmonitor(void *arg) {
	queue_t *q = (queue_t *)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		queue_print_stats(q);
		sleep(1);
	}

	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t *q = malloc(sizeof(queue_t));
	if (!q) {
		printf("Cannot allocate memory for a queue\n");
		abort();
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	sem_init(&q->items, 0, 0);
	sem_init(&q->spaces, 0, max_count);
	
	pthread_mutex_init(&q->lock_2, NULL);

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}

	return q;
}

void queue_destroy(queue_t *q) {
	if (!q) return;
	pthread_cancel(q->qmonitor_tid);
	pthread_join(q->qmonitor_tid, NULL);

	qnode_t *cur = q->first;
	while (cur) {
		qnode_t *next = cur->next;
		free(cur);
		cur = next;
	}
	sem_destroy(&q->items);
	sem_destroy(&q->spaces);
	sem_destroy(&q->lock_3);

	free(q);
}

int queue_add(queue_t *q, int val) {
	sem_wait(&q->spaces);
	qnode_t *new = malloc(sizeof(qnode_t));
	if (!new) {
        sem_post(&q->spaces);  
        return 0;
    }

	new->val = val;
	new->next = NULL;

	pthread_mutex_lock(&q->lock_2);
	q->add_attempts++;
	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	pthread_mutex_unlock(&q->lock_2);

	sem_post(&q->items);

	return 1;
}

int queue_get(queue_t *q, int *val) {
	sem_wait(&q->items);

	pthread_mutex_lock(&q->lock_2);

	q->get_attempts++;

	assert(q->count >= 0);

	qnode_t *tmp = q->first; 

	*val = tmp->val;
	q->first = q->first->next;
	q->count--;
	q->get_count++;

	pthread_mutex_unlock(&q->lock_2);
	free(tmp);
	sem_post(&q->spaces);
	return 1;
}

void queue_print_stats(queue_t *q) {
	sem_wait(&q->lock_3);
	
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
		
	sem_post(&q->lock_3);
}