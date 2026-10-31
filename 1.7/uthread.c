#define _XOPEN_SOURCE 700
#include "uthread.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>
#include <sys/mman.h>

#define GUARD_SIZE  (4096)
#define STACK_SIZE (64 * 1024)
#define MAX_THREADS 64

struct uthread {
    ucontext_t ctx; // штука для сохранения регистров для свапа контеста
    char *stack;
    void *(*fn)(void *);
    void *arg;
    int finished;
};

static struct uthread *threads[MAX_THREADS];
static int nthreads = 0;
static int current = -1;
static ucontext_t main_ctx;


static void trampoline(void) {
    struct uthread *t = threads[current]; 
    t->fn(t->arg);
    t->finished = 1;
    swapcontext(&t->ctx, &main_ctx);
}

int uthread_create(uthread_t *thread, void *(*fn)(void *), void *arg) {
    if (!thread || !fn) return -1;
    if (nthreads >= MAX_THREADS) return -1;

    struct uthread *t = calloc(1, sizeof(*t));
    if (!t) return -1;
    
    size_t total = STACK_SIZE + GUARD_SIZE;
    t->stack = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
   
    if (t->stack == MAP_FAILED) { free(t); return -1; }
    mprotect(t->stack, GUARD_SIZE, PROT_NONE);

    t->fn = fn;
    t->arg = arg;
    t->finished = 0;
   

    getcontext(&t->ctx); // main
    t->ctx.uc_stack.ss_sp = t->stack;
    t->ctx.uc_stack.ss_size = STACK_SIZE;
    t->ctx.uc_link = &main_ctx;  //говорим куда вернутся после return    
    makecontext(&t->ctx, trampoline, 0);

    threads[nthreads++] = t;
    *thread = t;
    return 0;
}

void uthread_yield(void) {
    if (nthreads == 0) return;

    int prev = current;
    int next = -1;
    for (int i = 1; i <= nthreads; i++) {
        int idx = (current + i + nthreads) % nthreads;
        if (!threads[idx]->finished) { next = idx; break; }
    }
    if (next < 0) { // живых потоков нет свапаемся на main
        if (prev >= 0)
            swapcontext(&threads[prev]->ctx, &main_ctx);
        return;
    }
    current = next;

    if (prev < 0) { // main свапаеться на живой поток:  первый запуск run или возврат из trampoline
        swapcontext(&main_ctx, &threads[next]->ctx);
    } else if (prev == next) { // только main 
        return;
    } else { // свап из потока в поток
        swapcontext(&threads[prev]->ctx, &threads[next]->ctx);
    }
}

void uthread_run(void) {
    for (;;) {
        int alive = 0;
        for (int i = 0; i < nthreads; i++)
            if (!threads[i]->finished) { alive = 1; break; }
        if (!alive) break;

        uthread_yield();
    }

    size_t total = STACK_SIZE + GUARD_SIZE;
    for (int i = 0; i < nthreads; i++) {
        munmap(threads[i]->stack, total);
        free(threads[i]);
        threads[i] = NULL;
    }
    nthreads = 0;
    current = -1;
}