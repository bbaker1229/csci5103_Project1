#include "uthread.h"
#include <stdio.h>

static void* go(void*);

#define NTHREADS 10

static int threads[NTHREADS];

void* go(void* nothing) {
    int i = 0;
    while(1) {
        ++i;
        printf("in thread %i (%d)\n",uthread_self(),i);
        sleep(1);
    }
    return 0;
}

int main (void) {

    int i;
    uthread_setup();

    uthread_init(2500000);

    for (i = 0; i < NTHREADS; i++) {
        threads[i] = uthread_create(&go, NULL);
    }

    while(1);

    return 0;
}