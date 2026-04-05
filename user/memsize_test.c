#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    // (a) Print current memory usage using the new system call
    int m1 = memsize();
    printf("Process memory usage: %d bytes\n", m1);

    // (b) Allocate 20k more bytes of memory by calling malloc
    printf("Allocating 20,000 bytes...\n");
    void *p = malloc(20000);
    if(p == 0){
        printf("malloc failed\n");
        exit(1);
    }

    // (c) Print memory usage after the allocation
    int m2 = memsize();
    printf("Memory usage after allocation: %d bytes\n", m2);

    // (d) Free the allocated array
    printf("Freeing allocated memory...\n");
    free(p);

    // (e) Print memory usage after the release
    int m3 = memsize();
    printf("Memory usage after free: %d bytes\n", m3);

    exit(0);
}