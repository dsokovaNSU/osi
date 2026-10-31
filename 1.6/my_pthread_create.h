#ifndef MY_PTHREAD_CREATE_H
#define MY_PTHREAD_CREATE_H

typedef struct mythread_struct *mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);
int mythread_join(mythread_t thread, void **retval);

#endif