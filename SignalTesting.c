// define xOPEN_SOUCE for timer to work
//#define _XOPEN_SOURCE

// include header files
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>


sigset_t mask;

void foo()
{
	printf("handler function\n");
	return;
}

int main (void){
    struct itimerval timer;
    sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	signal(SIGVTALRM, (void (*)(int))foo);

    timer.it_value.tv_sec = 1;
 	timer.it_value.tv_usec = 0;
	timer.it_interval = timer.it_value;
	//timer.it_interval.tv_sec = 1;
	//timer.it_interval.tv_usec = 0;
	setitimer (ITIMER_VIRTUAL, &timer, NULL);

	sigprocmask(SIG_UNBLOCK,&mask,NULL);

    while(1);

    return 0;
}
