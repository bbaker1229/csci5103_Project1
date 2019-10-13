#include <stdio.h>
#include "uthread.h"

static void go(int n);

#define NTHREADS 10
//static thread_t threads[NTHREADS];

int main (void) {
    int i;
    long exitValue;
    long *returnValue;

    for (i = 0; i < NTHREADS; i++){
        uthread_create(&go, NULL);
    }
    for (i = 0; i < NTHREADS; i++){
        exitValue = uthread_join(i, returnValue);
        printf("Thread %d returned with %ld\n", i, exitValue);
    }
    printf("Main thread done.\n");
    return 0;
}

void go(int n) {
    printf("Hello form thread %d\n", n);
    uthread_terminate(uthread_self());
}
