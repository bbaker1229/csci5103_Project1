/*
 * Resume Test will create two threads and then alternate suspending and resuming one of them. 
 */


#include "uthread.h"
#include <stdio.h>
#include <unistd.h>

int important_variable = 0;
lock_t lock;
int total_count = 0;

void* worker(void* nothing) {

    int my_tid = uthread_self();
    unsigned int count = 0;

	while(1){
        count += 1;
        if(count == 100000000){
            printf("thread %d\n", my_tid);
            count = 0;
            total_count += 1;
        }
    }
	uthread_terminate(my_tid);
}

int main (void) {

    int thread1;
    int thread2;

    uthread_init(100);

    printf("Start both threads\n");
    thread1 = uthread_create(&worker, NULL);
    thread2 = uthread_create(&worker, NULL);

    while(total_count < 10);

    printf("Suspend thread 1\n");
    uthread_suspend(thread1);
    
    while(total_count < 20);
    
    printf("Resume thread 1\n");
    uthread_resume(thread1);

    while(total_count < 30);

    printf("Suspend thread 2\n");
    uthread_suspend(thread2);

    while(total_count < 40);

    printf("Resume thread 2\n");
    uthread_resume(thread2);

    while(total_count < 50);

    uthread_join(thread1, NULL);
    uthread_join(thread2, NULL);

    printf("both threads finished\n");

    return 0;
}
