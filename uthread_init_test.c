#define _XOPEN_SOURCE
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

//#include "uthread.h"

#define SECOND 1000000

void uthread_yield (int signum) {
    static int count = 0;
    printf("timer expired %d times\n", ++count);
}

int uthread_init( int time_slice ) {
	struct itimerval timer;
	struct sigaction sa;

	memset (&sa, 0, sizeof (sa));
 	sa.sa_handler = &uthread_yield;
 	sigaction (SIGVTALRM, &sa, NULL);

	timer.it_value.tv_sec = time_slice / SECOND;
 	timer.it_value.tv_usec = time_slice % SECOND;
	timer.it_interval.tv_sec = time_slice / SECOND;
	timer.it_interval.tv_usec = time_slice % SECOND;
	setitimer (ITIMER_VIRTUAL, &timer, NULL);
    return 0;
}

int main (void) {
    int time_slice = 500000;

    uthread_init(time_slice);

    while (1);
}
