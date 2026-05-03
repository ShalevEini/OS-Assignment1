#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
print_result(const char *name, int rc, int expected)
{
    printf("%s: returned %d (expected %d)\n", name, rc, expected);
    if (rc != expected) {
        printf("%s failed\n", name);
        exit(1);
    }
    printf("%s passed\n", name);
}

static void
test_invalid_pid_zero(void)
{
    int rc = co_yield(0, 1);
    print_result("test_invalid_pid_zero", rc, -1);
}

static void
test_invalid_pid_negative(void)
{
    int rc = co_yield(-7, 1);
    print_result("test_invalid_pid_negative", rc, -1);
}

static void
test_invalid_value_zero(void)
{
    int child_pid = fork();
    if (child_pid < 0) {
        printf("test_invalid_value_zero: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        while (1)
            sleep(100);
    } else {
        int rc = co_yield(child_pid, 0);
        print_result("test_invalid_value_zero", rc, -1);
        kill(child_pid);
        wait(0);
    }
}

static void
test_invalid_value_negative(void)
{
    int child_pid = fork();
    if (child_pid < 0) {
        printf("test_invalid_value_negative: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        while (1)
            sleep(100);
    } else {
        int rc = co_yield(child_pid, -3);
        print_result("test_invalid_value_negative", rc, -1);
        kill(child_pid);
        wait(0);
    }
}

static void
test_nonexistent_pid(void)
{
    int rc = co_yield(99999, 1);
    print_result("test_nonexistent_pid", rc, -1);
}

static void
test_self_yield(void)
{
    int rc = co_yield(getpid(), 1);
    print_result("test_self_yield", rc, -1);
}

static void
test_killed_process(void)
{
    int child_pid = fork();

    if (child_pid < 0) {
        printf("test_killed_process: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        while (1)
            sleep(100);
    } else {
        kill(child_pid);
        wait(0);

        int rc = co_yield(child_pid, 1);
        print_result("test_killed_process", rc, -1);
    }
}

static void
test_many_rounds(void)
{
    int parent_pid = getpid();
    int child_pid = fork();

    if (child_pid < 0) {
        printf("test_many_rounds: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        while (1) {
            int v = co_yield(parent_pid, 1);
            if (v < 0)
                exit(0);
            if (v != 2) {
                printf("test_many_rounds child failed: got %d\n", v);
                exit(1);
            }
        }
    } else {
        for (int i = 0; i < 50; i++) {
            int v = co_yield(child_pid, 2);
            if (v != 1) {
                printf("test_many_rounds parent failed at round %d: got %d\n", i, v);
                kill(child_pid);
                wait(0);
                exit(1);
            }
        }
        kill(child_pid);
        wait(0);
        printf("test_many_rounds passed\n");
    }
}

static void
test_killed_while_sleeping(void)
{
    int parent_pid = getpid();
    int child_pid = fork();

    if (child_pid < 0) {
        printf("test_killed_while_sleeping: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        int rc = co_yield(parent_pid, 1);
        printf("test_killed_while_sleeping child woke with rc=%d\n", rc);
        exit(0);
    } else {
        sleep(20);
        kill(child_pid);
        wait(0);
        printf("test_killed_while_sleeping passed\n");
    }
}



static void
infinite_ping_pong(void)
{
    int pid1 = getpid();   // Parent PID
    int pid2 = fork();     // Child PID

    if (pid2 < 0) {
        printf("infinite_ping_pong: fork failed\n");
        exit(1);
    }

    if (pid2 == 0) { // Child
        for (;;) {
            int value = co_yield(pid1, 1);
            printf("Child received: %d\n", value); // Should print 2
        }
    } else { // Parent
        for (;;) {
            int value = co_yield(pid2, 2);
            printf("parent received: %d\n", value); // Should print 1
        }
    }
}

int
main(void)
{
    printf("=== co_yield tests start ===\n");

    test_invalid_pid_zero();
    test_invalid_pid_negative();
    test_invalid_value_zero();
    test_invalid_value_negative();
    test_nonexistent_pid();
    test_self_yield();
    test_killed_process();
    test_many_rounds();
    test_killed_while_sleeping();

    printf("=== finite tests finished ===\n");

    infinite_ping_pong();

    exit(0);
}