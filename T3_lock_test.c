/*
 * Lock test will create two threads, they alternate in holding the lock and trying to aquire the lock.
 * When the lock is held by one thread, the other thread will yield and only aquires the lock after the first thread releases the lock.
 */


#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

int important_variable = 0;
lock_t lock;
int total_count = 0;
const int CNT=5;
const int CNT2=4;

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

    int thread1;
    int thread2;

    uthread_init(1000);
	
	lock_init(&lock);

    thread1 = uthread_create(&worker, NULL);
    thread2 = uthread_create(&worker, NULL);

    while(total_count < CNT*CNT2)

    uthread_join(thread1, NULL);
    uthread_join(thread2, NULL);

    printf("both threads finished\n");

    return 0;
}
