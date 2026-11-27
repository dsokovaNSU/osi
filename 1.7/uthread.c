#define _XOPEN_SOURCE 700
#include "uthread.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define GUARD_SIZE  (4096)
#define STACK_SIZE (64 * 1024)
#define MAX_THREADS 64

struct uthread {
    ucontext_t ctx;
    char *stack;
    void *(*fn)(void *);
    void *arg;
    int finished;
};

static struct uthread *threads[MAX_THREADS];
static int nthreads = 0;     
static int current = -1;
static ucontext_t main_ctx;

static void trampoline(void);

static void trampoline(void) {
    struct uthread *t = threads[current]; 
    
    t->fn(t->arg);
    
    t->finished = 1;
    nthreads--;
    uthread_yield(); 
}

int uthread_create(uthread_t *thread, void *(*fn)(void *), void *arg) {
    if (!thread || !fn) return -1;
    
    int slot = -1;
    
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        for (int i = 0; i < MAX_THREADS; i++) {
            if (threads[i] != NULL && threads[i]->finished) {
                size_t total = STACK_SIZE + GUARD_SIZE;
                munmap(threads[i]->stack, total);
                free(threads[i]);
                slot = i;
                break;
            }
        }
    }
    
    if (slot == -1) return -1;

    struct uthread *t = calloc(1, sizeof(*t));
    if (!t) return -1;
    
    size_t total = STACK_SIZE + GUARD_SIZE;
    t->stack = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (t->stack == MAP_FAILED) { 
        free(t); 
        return -1; 
    }
    
    t->ctx.uc_stack.ss_sp = t->stack + GUARD_SIZE;
    t->ctx.uc_stack.ss_size = STACK_SIZE;
    
    mprotect(t->stack, GUARD_SIZE, PROT_NONE);

    t->fn = fn;
    t->arg = arg;
    t->finished = 0;

    getcontext(&t->ctx);
    t->ctx.uc_link = &main_ctx;  
    makecontext(&t->ctx, trampoline, 0);

    threads[slot] = t;
    nthreads++;
    *thread = t;
    return 0;
}

void uthread_yield(void) {
    if (nthreads == 0) return;

    int prev = current;
    int next = -1;
    for (int i = 1; i <= MAX_THREADS; i++) {
        int idx = (current + i) % MAX_THREADS;
        if (threads[idx] != NULL && !threads[idx]->finished) {
            next = idx;
            break;
        }
    }
    
    if (next < 0) {
        return;
    }
    
    current = next;
    
    if (prev < 0) {
        swapcontext(&main_ctx, &threads[next]->ctx);
    } else {
        swapcontext(&threads[prev]->ctx, &threads[next]->ctx);
    }
}

void uthread_run(void) {
    while (nthreads > 0) {
        uthread_yield();
    }
    size_t total = STACK_SIZE + GUARD_SIZE;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] != NULL) {
            munmap(threads[i]->stack, total);
            free(threads[i]);
            threads[i] = NULL;
        }
    }
    current = -1;
   
}