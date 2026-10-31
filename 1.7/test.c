#include "unity.h"
#include "uthread.h"
#include <stdio.h>

#define MAX_THREADS 64

void setUp(void)    {}
void tearDown(void) {}

static void *worker(void *arg) {
    const char *name = arg;
    for (int i = 0; i < 3; i++) {
        printf("  [%s] step %d\n", name, i);
        uthread_yield();
    }
    printf("  [%s] done\n", name);
    return NULL;
}

static void *worker_quiet(void *arg) {
    (void)arg;
    for (int i = 0; i < 3; i++)
        uthread_yield();
    return NULL;
}

static void *many_yield(void *arg) {
    (void)arg;
    for (int i = 0; i < 5; i++)
        uthread_yield();
    return NULL;
}



void test_yield_without_threads_does_not_crash(void) {
    uthread_yield();          
    TEST_PASS();
}

void test_run_without_threads_returns_immediately(void) {
    uthread_run();            
    TEST_PASS();
}

void test_run_twice_in_a_row(void) {
    uthread_run();            
    uthread_run();            
    TEST_PASS();
}

void test_create_null_start_routine_fails(void) {
    uthread_t t;
    TEST_ASSERT_EQUAL_INT(-1, uthread_create(&t, NULL, NULL));
}

void test_create_null_thread_pointer_fails(void) {
    TEST_ASSERT_EQUAL_INT(-1, uthread_create(NULL, worker, "A"));
}

void test_create_null_arg_is_ok(void) {
    uthread_t t;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t, many_yield, NULL));
    uthread_run();
}


void test_three_threads_run_to_completion(void) {
    uthread_t t1, t2, t3;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t1, worker, "A"));
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t2, worker, "B"));
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t3, worker, "C"));
    uthread_run();
    TEST_PASS();
}

void test_run_can_be_reused_after_completion(void) {
    uthread_t t;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t, many_yield, NULL));
    uthread_run();

    uthread_t t2;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t2, many_yield, NULL));
    uthread_run();
}


void test_create_max_threads_all_succeed(void) {
    uthread_t ts[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        TEST_ASSERT_EQUAL_INT(0, uthread_create(&ts[i], worker_quiet, NULL));
    }
    uthread_run();   

}
void test_create_over_limit_fails(void) {
    uthread_t ts[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        TEST_ASSERT_EQUAL_INT(0, uthread_create(&ts[i], worker_quiet, NULL));
    }
    uthread_t extra;
    TEST_ASSERT_EQUAL_INT(-1, uthread_create(&extra, worker_quiet, NULL));

    uthread_run();   
}

void test_can_create_after_limit_and_run(void) {
    uthread_t ts[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++)
        uthread_create(&ts[i], worker_quiet, NULL);
    uthread_run();  

    uthread_t t;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t, worker, "X"));
    uthread_run();
}


void test_many_yields_inside_one_thread(void) {
    uthread_t t;
    TEST_ASSERT_EQUAL_INT(0, uthread_create(&t, many_yield, NULL));
    uthread_run();
    TEST_PASS();
}



int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_yield_without_threads_does_not_crash);
    RUN_TEST(test_run_without_threads_returns_immediately);
    RUN_TEST(test_run_twice_in_a_row);
    RUN_TEST(test_create_null_start_routine_fails);
    RUN_TEST(test_create_null_thread_pointer_fails);
    RUN_TEST(test_create_null_arg_is_ok);

    RUN_TEST(test_three_threads_run_to_completion);
    RUN_TEST(test_run_can_be_reused_after_completion);

    RUN_TEST(test_create_max_threads_all_succeed);
    RUN_TEST(test_create_over_limit_fails);
    RUN_TEST(test_can_create_after_limit_and_run);

    RUN_TEST(test_many_yields_inside_one_thread);

    return UNITY_END();
}
