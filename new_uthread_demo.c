#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

static void* go(void*);

#define NTHREADS 10

static int threads[NTHREADS];

void* go(void* nothing) {
    int i, j = 0;
    while(1) {
        ++i;
        printf("in thread %i (%d)\n",uthread_self(),i);
        for (j = 0; j < 10000000; j++);
    }
    uthread_terminate(uthread_self());
    return 0;
}

int main (void) {

    int i;

    
    //uthread_setup();

    uthread_init(25000);

    for (i = 0; i < NTHREADS; i++) {
        threads[i] = uthread_create(&go, NULL);
    }

    //scheduler();

    while(1);

    return 0;
}