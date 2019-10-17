/*
 * Lock test will create two threads, a more important thread that will acquire
 * the lock first, and a second thread that will wait until it can acquire the lock.
 * The first thread will periodically try to yield, but while the second thread
 * is unable to acquire the lock, it will also yield until finally first thread
 * finishes and releases the lock. Then second thread can finish and the main
 * program will exit.
 */


#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

int important_variable = 0;
lock_t lock;

void* important_func(void* nothing) {
    int i, j = 0;

    acquire(&lock);

    while(i < 20) {
        ++i;
        printf("in very important thread %d (%d)\n",uthread_self(),i);
        for (j = 0; j < 10000; j++);
        uthread_yield();
    }

    important_variable = 24;

    release(&lock);

    uthread_terminate(uthread_self());
    return 0;
}

void* less_important_func(void* nothing) {

	printf("in less important thread %d\n", uthread_self());

	acquire(&lock);

	printf("lock was finally acquired by less important thread\n");
	printf("important variable is %d\n", important_variable);

	release(&lock);

	uthread_terminate(uthread_self());
	return NULL;
}

int main (void) {

    int thread1;
    int thread2;

    uthread_init(100);

    thread1 = uthread_create(&important_func, NULL);
    thread2 = uthread_create(&less_important_func, NULL);

    uthread_join(thread2, NULL);
    uthread_join(thread1, NULL);

    printf("both threads finished\n");

    return 0;
}
