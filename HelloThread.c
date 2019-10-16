#include <stdio.h>
#include "uthread.h"

static void* go(void*);

#define NTHREADS 10

static int threads[NTHREADS];

int main (void) {
    int i;

    uthread_setup();

    for (i = 0; i < NTHREADS; i++){
        threads[i] = uthread_create(&go, NULL);
    }
    for (i = 0; i < NTHREADS; i++){
        uthread_join(threads[i], NULL);
        printf("Thread %d returned\n", threads[i]);
    }
    printf("Main thread done.\n");
    return 0;
}

void* go(void* nothing) {
    printf("Hello from thread %d\n", uthread_self());
    uthread_terminate(uthread_self());
    return NULL;
}
