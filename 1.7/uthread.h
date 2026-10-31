#ifndef UTHREAD_H
#define UTHREAD_H

typedef struct uthread *uthread_t;

//0 при успехе, -1 при ошибке
int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg);

void uthread_yield(void);
void uthread_run(void);

#endif