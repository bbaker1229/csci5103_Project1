#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "uthread.h"

void* single_thread_example()
{
    int i=0;
    while(1) {
        ++i;
        printf("in single_thread_example (%d)\n",i);
        if (i == 5) {
            printf("terminating\n");
            break;
        }
        sleep(1);
    }
    uthread_terminate(uthread_self());
    return NULL;
}

int main(int argc, const char* argv[])
{
	uthread_setup();
    int tid = uthread_create(single_thread_example, NULL);
    return 0;
}
