#include <stddef.h>
#define _GNU_SOURCE
#include "my_pthread_create.h"
#include <sched.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/mman.h>

#define GUARD_SIZE  (4096)

#define STACK_SIZE (1024 * 1024)  

struct mythread_struct {
    int  ctid;          
    void *stack;    
    size_t stack_size;    
    void *(*fn)(void *);
    void *arg;
    void *retval;
};

static int trampoline(void *p) {
    struct mythread_struct *ts = (struct mythread_struct *)p;
    void *ret = ts->fn(ts->arg);
    ts->retval = ret;
    syscall(SYS_exit, 0);
    return 0; 
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg)
{
    if (!thread || !start_routine) {
        errno = EINVAL; 
        return -1;
    }

    struct mythread_struct *ts = calloc(1, sizeof(*ts));
    if (!ts) return -1;

    size_t total = STACK_SIZE + GUARD_SIZE;
    void *mem = mmap(NULL, total, PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,-1, 0);
    if (mem == MAP_FAILED) { free(ts); return -1; }

    if (mprotect(mem, GUARD_SIZE, PROT_NONE) == -1) {
        munmap(mem, total);
        free(ts);
        return -1;
    }

    ts->stack = mem;               
    ts->stack_size = total;

    ts->fn    = start_routine;
    ts->arg   = arg;
    ts->ctid  = 1;      

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_CHILD_CLEARTID;  

    pid_t tid = clone(trampoline,  (char *)mem + total, flags, ts, NULL, NULL, &ts->ctid);

    if (tid == -1) {
        
        munmap(ts->stack, ts->stack_size);
        free(ts);
        return -1;
    }

    *thread = ts;
    return 0;
}

int mythread_join(mythread_t t, void **retval) {
    if (!t) { errno = EINVAL; return -1; }

    while (t->ctid != 0) {
        syscall(SYS_futex, &t->ctid, FUTEX_WAIT, t->ctid, NULL, NULL, 0);
    }

    if (retval) *retval = t->retval;

    munmap(t->stack, t->stack_size);
    free(t);
    return 0;
}

