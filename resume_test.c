/*
 * Resume Test will create two threads and then suspend one of them. It
 * joins the other one, which will try to yield to the other thread.
 * However, because thread 2 is suspended, it will not run until thread 1
 * finishes and thread 2 is resumed. Then thread 2 will finish and the
 * proram will end.
 */


#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

int important_variable = 0;
lock_t lock;

void* important_func(void* nothing) {
    int i, j = 0;

    while(i < 20) {
        ++i;
        printf("in very important thread %d (%d)\n",uthread_self(),i);
        for (j = 0; j < 10000; j++);
        uthread_yield();
    }

    uthread_terminate(uthread_self());
    return 0;
}

void* less_important_func(void* nothing) {

	printf("in less important thread %d\n", uthread_self());

	uthread_terminate(uthread_self());
	return NULL;
}

int main (void) {

    int thread1;
    int thread2;

    uthread_init(100);

    thread1 = uthread_create(&important_func, NULL);
    thread2 = uthread_create(&less_important_func, NULL);

    uthread_suspend(thread2);

    uthread_join(thread1, NULL);

    uthread_resume(thread2);

    uthread_join(thread2, NULL);


    printf("both threads finished\n");

    return 0;
}
