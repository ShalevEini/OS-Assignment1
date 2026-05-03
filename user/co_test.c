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
    int child_pid = fork();

    if (child_pid < 0) {
        printf("test_killed_while_sleeping: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        int rc = co_yield(getpid(), 1);
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
test_two_children_same_parent_target(void)
{
    int p = getpid();
    int c1 = fork();
    if (c1 < 0) {
        printf("test_two_children_same_parent_target: fork c1 failed");
        exit(1);
    }
    if (c1 == 0) {
        int v = co_yield(p, 100);
        if (v != 101) {
            printf("test_two_children_same_parent_target child1 failed: got %d", v);
            exit(1);
        }
        exit(0);
    }

    int c2 = fork();
    if (c2 < 0) {
        printf("test_two_children_same_parent_target: fork c2 failed");
        kill(c1);
        wait(0);
        exit(1);
    }
    if (c2 == 0) {
        int v = co_yield(p, 200);
        if (v != 201) {
            printf("test_two_children_same_parent_target child2 failed: got %d", v);
            exit(1);
        }
        exit(0);
    }

    int r1 = co_yield(c1, 101);
    int r2 = co_yield(c2, 201);

    if (r1 != 100 || r2 != 200) {
        printf("test_two_children_same_parent_target parent failed: r1=%d r2=%d", r1, r2);
        kill(c1);
        kill(c2);
        wait(0);
        wait(0);
        exit(1);
    }

    wait(0);
    wait(0);
    printf("test_two_children_same_parent_target passed");
}

int
main(void)
{
    printf("=== edge_case tests start ===\n");

    test_invalid_pid_zero();
    test_invalid_pid_negative();
    test_invalid_value_zero();
    test_invalid_value_negative();
    test_nonexistent_pid();
    test_self_yield();
    test_killed_process();
    test_many_rounds();
    test_killed_while_sleeping();
    test_two_children_same_parent_target();

    printf("=== edge_case tests finished ===\n");
    exit(0);
}
