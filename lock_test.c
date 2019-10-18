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
int total_count = 0;


void* worker1(void* nothing) {
    int i, j = 0;

    int my_tid = uthread_self();
    unsigned int count = 0;

    acquire(&lock);

    while(1){
        count += 1;
        if(count == 100000000){
            printf("Thread %d has lock\n", my_tid);
            count = 0;
            total_count += 1;
            //printf("Try to yield\n");
            uthread_yield();
        }
    }

    important_variable = 24;

    printf("important variable: %i\n", important_variable);
    release(&lock);

    printf("Lock Released\n");

    uthread_terminate(my_tid);
}

void* worker2(void* nothing) {
    int i, j = 0;
    int my_tid = uthread_self();
    unsigned int count = 0;
	printf("thread %d trying to get lock\n", my_tid);

	acquire(&lock);

    while(1){
        count += 1;
        if(count == 100000000){
            printf("lock was finally acquired by less important thread\n");
	        printf("important variable is %d\n", important_variable);
            count = 0;
            //total_count += 1;
            //printf("Try to yield\n");
            //uthread_yield();
        }
    }

	

	release(&lock);

	uthread_terminate(my_tid);
	return NULL;
}

int main (void) {

    int thread1;
    int thread2;

    int i = 0;

    uthread_init(1000);
	
	lock_init(&lock);

    thread1 = uthread_create(&worker1, NULL);
    thread2 = uthread_create(&worker2, NULL);

    while(total_count < 10);

    while(i < 10000000){
        i++;
    }

    uthread_join(thread1, NULL);

    //uthread_yield();

    uthread_join(thread2, NULL);

    printf("both threads finished\n");

    return 0;
}
