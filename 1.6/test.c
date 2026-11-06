#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include "unity.h"
#include <string.h>
#include "my_pthread_create.h"
#include "unity_internals.h"


static void *worker(void *arg) {
    long id = (long)arg;
    usleep(10000);
    return (void *)(id * 10);
}

static void *check_ret1(void *arg) { usleep(10000); return (void *)(1); }
static void *check_ret2(void *arg) { usleep(50000); return (void *)(2); }

static void *child_thread(void *arg) {
    long id = (long)arg;
    usleep(10000);
    return (void *)(id + 100);
}

static void *parent_thread(void *arg) {
    long id = (long)arg;
    mythread_t child;
    void *child_ret;
    if (mythread_create(&child, child_thread, (void *)(id * 10)) != 0) return (void *)-1;
    if (mythread_join(child, &child_ret) != 0) return (void *)-1;
    return (void *)(id + 1000);
}

static void *deep_recursion(void *arg) {
    volatile char buf[1024];
    buf[0] = (char)(long)arg;
    return deep_recursion((void *)((long)arg + 1));
}

static void *simple_worker(void *arg) {
    return (void *)5;
}

void setUp(void) { }
void tearDown(void) { }

void test_create_and_join_multiple(void) {
    mythread_t t[5];
    void *ret;
    for (long i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(0, mythread_create(&t[i], worker, (void *)i));
    }
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(0, mythread_join(t[i], &ret));
        TEST_ASSERT_EQUAL((long)(i * 10), (long)ret);
    }
}

void test_create_null_thread(void) {
    TEST_ASSERT_NOT_EQUAL(0, mythread_create(NULL, worker, NULL));
}

void test_create_null_routine(void) {
    mythread_t t;
    TEST_ASSERT_NOT_EQUAL(0, mythread_create(&t, NULL, NULL));
}

void test_join_null(void) {
    TEST_ASSERT_NOT_EQUAL(0, mythread_join(NULL, NULL));
}

void test_retval_not_swapped(void) {
    mythread_t t1, t2;
    void *r1, *r2;
    TEST_ASSERT_EQUAL(0, mythread_create(&t1, check_ret1, NULL));
    TEST_ASSERT_EQUAL(0, mythread_create(&t2, check_ret2, NULL));
    TEST_ASSERT_EQUAL(0, mythread_join(t1, &r1));
    TEST_ASSERT_EQUAL(0, mythread_join(t2, &r2));
    TEST_ASSERT_EQUAL(1, (long)r1);
    TEST_ASSERT_EQUAL(2, (long)r2);
}


void test_double_join_crashes(void) {
    pid_t pid = fork();
    if (pid == 0) {
        mythread_t t;
        void *ret;
        if (mythread_create(&t, simple_worker, NULL) != 0) _exit(1);
        mythread_join(t, &ret);
        mythread_join(t, &ret);
        _exit(0);
    }
    int status = 0;
    int timeout_ms = 1000; 
    int waited = 0;
    pid_t res = 0;

    while (waited < timeout_ms) {
        res = waitpid(pid, &status, WNOHANG);
        if (res != 0) break; 
        usleep(10000);
        waited += 10;
    }

    if (res == 0) {
        kill(pid, SIGKILL);    
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(1); 
    } else {
        TEST_FAIL_MESSAGE("Поток не завис, как ожидалось (защита не работает?)");
    }
}
void test_nested_thread(void) {
    mythread_t parent;
    void *parent_ret;
    TEST_ASSERT_EQUAL(0, mythread_create(&parent, parent_thread, (void *)7));
    TEST_ASSERT_EQUAL(0, mythread_join(parent, &parent_ret));
    TEST_ASSERT_EQUAL(1007, (long)parent_ret);
}

void test_guard_page_sigsegv(void) {
    pid_t pid = fork();
    if (pid == 0) {
        mythread_t t;
        mythread_create(&t, deep_recursion, (void *)0);
        mythread_join(t, NULL);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    TEST_ASSERT_TRUE(WIFSIGNALED(status));
    TEST_ASSERT_EQUAL(SIGSEGV, WTERMSIG(status));
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_double_join_crashes);
    RUN_TEST(test_create_and_join_multiple);
    RUN_TEST(test_create_null_thread);
    RUN_TEST(test_create_null_routine);
    RUN_TEST(test_join_null);
    RUN_TEST(test_retval_not_swapped);
    RUN_TEST(test_nested_thread);
    RUN_TEST(test_guard_page_sigsegv);
    return UNITY_END();
}
