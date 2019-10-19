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
#define CNT 5
#define CNT2 4

int important_variable = 0;
lock_t lock;
int total_count = 0;


void* worker(void* nothing) {
    int i, j = 0;
    int my_tid = uthread_self();

	for (i=0; i<=CNT2;i++){
	    printf("Thread %d request lock\n", my_tid);
        acquire(&lock);

        for (j=0;j<=CNT;j++){
            printf("Thread %d has lock\n", my_tid);
            total_count += 1;
//          printf("Thread %d yielding\n", my_tid);
            uthread_yield();
        }

    important_variable = 24;

    printf("Thread %d releasing lock...\n", my_tid);
    release(&lock);
    printf("Lock Released\n");
	uthread_yield();
	}

    uthread_terminate(my_tid);
    return NULL;
}

int main (void) {

    int thread_count = 2;
    int i = 0;

    int threads[thread_count];

    //int thread1;
    //int thread2;

    uthread_init(1000);
	
	lock_init(&lock);

	for (i = 0; i < thread_count; i++) {
		int tid = uthread_create(&worker, NULL);
		printf("tid=%d\n", tid);
		threads[i] = tid;
	}

    //thread1 = uthread_create(&worker, NULL);
    //thread2 = uthread_create(&worker, NULL);

    while(total_count < CNT*CNT2)

	for (i = 0; i < thread_count; i++) {
		uthread_join(threads[i], NULL);
	}

    //uthread_join(thread1, NULL);

    //uthread_join(thread2, NULL);
 

    printf("both threads finished\n");

    return 0;
}