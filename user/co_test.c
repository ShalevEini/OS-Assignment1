#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
success_test(void)
{
    int parent_pid = getpid();
    int child_pid = fork();

    if (child_pid < 0) {
        printf("success_test: fork failed\n");
        exit(1);
    }

    if (child_pid == 0) {
        while (1) {
            int value = co_yield(parent_pid, 1);
            if (value < 0) {
                printf("child: co_yield returned %d, exiting\n", value);
                exit(0);
            }
            printf("child received: %d\n", value);
            if (value != 2) {
                printf("success_test failed in child: expected 2, got %d\n", value);
                exit(1);
            }
        }
    } else {
        for (int i = 0; i < 5; i++) {
            int value = co_yield(child_pid, 2);
            printf("parent received: %d\n", value);
            if (value != 1) {
                printf("success_test failed in parent: expected 1, got %d\n", value);
                kill(child_pid);
                wait(0);
                exit(1);
            }
        }

        // Child is probably sleeping inside co_yield waiting for one more handoff.
        kill(child_pid);
        wait(0);
        printf("success_test passed\n");
    }
}

static void
test_nonexistent_pid(void)
{
    int rc = co_yield(99999, 1);
    printf("test_nonexistent_pid: returned %d\n", rc);
    if (rc != -1) {
        printf("test_nonexistent_pid failed\n");
        exit(1);
    }
    printf("test_nonexistent_pid passed\n");
}

static void
test_self_yield(void)
{
    int mypid = getpid();
    int rc = co_yield(mypid, 1);
    printf("test_self_yield: returned %d\n", rc);
    if (rc != -1) {
        printf("test_self_yield failed\n");
        exit(1);
    }
    printf("test_self_yield passed\n");
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
        printf("test_killed_process: returned %d\n", rc);
        if (rc != -1) {
            printf("test_killed_process failed\n");
            exit(1);
        }
        printf("test_killed_process passed\n");
    }
}

int
main(void)
{
    printf("=== co_yield tests start ===\n");

    success_test();
    test_nonexistent_pid();
    test_self_yield();
    test_killed_process();

    printf("=== all co_yield tests passed ===\n");
    exit(0);
}