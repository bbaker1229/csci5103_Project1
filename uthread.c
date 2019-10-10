#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "uthread.h"

#define SECOND 			1000000 	// time in microseconds
#define TIME_SLICE		2500000
#define STACK_SIZE 		4096	// size in bytes

#ifdef __x86_64__


typedef unsigned long address_t;
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
	address_t ret;
	asm volatile("xor    %%fs:0x30,%0\n"
			"rol    $0x11,%0\n"
			: "=g" (ret)
			  : "0" (addr));
	return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
	address_t ret;
	asm volatile("xor    %%gs:0x18,%0\n"
			"rol    $0x9,%0\n"
			: "=g" (ret)
			  : "0" (addr));
	return ret;
}

#endif

// enum for thread states
typedef enum {
	INIT,
	READY,
	RUNNING,
	SUSPEND,
	FINISHED
} thread_state_t;

// struct to define thread control block
typedef struct TCB {
	int tid; // thread id
	thread_state_t state;
	int stack_size;
	char* stack;
	address_t sp;
	address_t pc;
	sigjmp_buf jbuf;
	struct TCB* next;
} TCB;

// count of threads created by system.
// used to issue tid.
int thread_count;

// scheduling queues
TCB* ready_queue;
TCB* suspend_queue;

// currently running thread
TCB* running_thread;

// our main scheduler thread
TCB* main_thread;

// test and set atomic lock mechanism
int TAS(volatile int *addr, int newval){
    int result = newval;
    asm volatile("lock; xchg %0, %1"
                 : "+m" (*addr), "=r" (result)
                 : "1" (newval)
                 : "cc");
    return result;

}

// adds the tcb to the end of the ready_queue
void ready_queue_add(TCB* tcb) {
	// create a TCB pointer and point it at the head
	// of ready_queue
	TCB* index = ready_queue;

	tcb->state = Ready;

	// if there are no entries, just make tcb the head
	if ( index == NULL ) {
		ready_queue = tcb;
	} else {
		// traverse to the end of the list
		while(index->next != NULL) {
			index = index->next;
		}
		index->next = tcb;
	}
}


/* * * * * * * * * * * */
/* pthread equivalents */
/* * * * * * * * * * * */

// Allocates a Thread Control Block and stack for the new thread and
// adds it to the scheduling queue. Returns the id of the new thread.
// Note: this ignores the second void* arg at this time.
int uthread_create( void *( *start_routine )( void * ), void *arg ) {

	// allocate the control block structure
	TCB* tcb = (TCB*) malloc(sizeof(TCB));
	if (tcb == NULL) {
		printf("uthread_create: Failed to allocate TCB\n");
		return -1;
	}

	// start the state at Init
	tcb->state = Init;

	// assign thread id
	tcb->tid = thread_count++;

	// allocate a stack
	tcb->stack_size = STACK_SIZE;
	char* stack = (char*) malloc(tcb->stack_size * sizeof(char));
	if (stack == NULL) {
		free(tcb);
		printf("uthread_create: Failed to allocate stack\n");
		return -1;
	}
	tcb->stack = stack;

	// point the stack pointer and program counter at the right things
	tcb->sp = (address_t)stack + STACK_SIZE -sizeof(int);
	tcb->pc = (address_t)start_routine;

	// store context for jumping
    sigsetjmp(tcb->jbuf,1);
    (tcb->jbuf->__jmpbuf)[JB_SP] = translate_address(tcb->sp);
    (tcb->jbuf->__jmpbuf)[JB_PC] = translate_address(tcb->pc);
    sigemptyset(&tcb->jbuf->__saved_mask);

	//add the new_tcb to the thread scheduler queue
    ready_queue_add(tcb);

	return tcb->tid;
}

void uthread_start(int tid) {
	TCB* index;
	index = ready_queue;

	while (index->tid != tid && index->next != NULL) {
		index = index->next;
	}

	running_thread = index;
    siglongjmp(index->jbuf,1);
}

int uthread_yield( void ) {
	// store context
	// move thread to waiting queue
	// needs to call uthread_init(TIME_SLICE); somewhere to reset the time after the thread yields. 
	// Also should call suspend and resume. 
    return 0;
}             

// Returns currently running thread id.
int uthread_self( void ) {
    return running_thread->tid;
}        

int uthread_join( int tid, void **retval ) {
	//Once threads complete: Free all threads in FINISHED state
    return 0;
}

/* * * * * * * * * * * */
/* uthread control     */
/* * * * * * * * * * * */
int uthread_init( int time_slice ) {
	struct itimerval timer;
	struct sigaction yield; //Not sure how to implement the timer.

	memset (&yield, 0, sizeof (yield));
 	yield.sa_handler = &uthread_yield;
 	sigaction (SIGVTALRM, &yield, NULL);

	timer.it_value.tv_sec = time_slice / SECOND;
 	timer.it_value.tv_usec = time_slice % SECOND;
	setitimer (ITIMER_VIRTUAL, &timer, NULL);
    return 0;
}

int uthread_terminate( int tid ) {
	tcb[tid].state = FINISHED;
	//The rest should be freed by uthread_join
    return 0;
}

int uthread_suspend( int tid ) {
	if(tcb[tid].state == RUNNING) {
		tcb[tid].state == SUSPEND;
		//move to suspend queue...
	} else if(tcb[tid].state == READY) {
		tcb[tid].state == SUSPEND;
		//move to suspend queue...
	}
    return 0;
}

int uthread_resume( int tid ) {
	if(tcb[tid].state == SUSPEND) {
		tcb[tid].state == RUNNING;
		//remove from suspend queue...
	} else if(tcb[tid].state == READY) {
		tcb[tid].state == RUNNING;
		//remove from ready queue...
	}
    return 0;
}

/* * * * * * * * * * * */
/* lock functions      */
/* * * * * * * * * * * */
int lock_init( lock_t *lock1 ) {
    return 0;
}

int acquire( lock_t *lock1 ) {
    return 0;
}

int release( lock_t *lock1 ) {
    return 0;
}
