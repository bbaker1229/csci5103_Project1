
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "uthread.h"

#define SECOND 			1000000 	// time in microseconds
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
typedef enum thread_state_t{
	Init = 0,
	Ready = 1,
	Running = 2,
	Waiting = 3,
	Finished = 4,
} thread_state;

// struct to define thread control block
typedef struct TCB {
	int tid; // thread id
	thread_state state;
	int stack_size;
	char* stack;
	address_t sp;
	address_t pc;
	sigjmp_buf jbuf;
	int joined_tid;
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

// adds the tcb to the end of the suspend_queue
void suspend_queue_add(TCB* tcb) {
	// create a TCB pointer and point it at the head
	// of ready_queue
	TCB* index = suspend_queue;

	// if there are no entries, just make tcb the head
	if ( index == NULL ) {
		suspend_queue = tcb;
	} else {
		// traverse to the end of the list
		while(index->next != NULL) {
			index = index->next;
		}
		index->next = tcb;
	}
}

void uthread_setup()
{
	// allocate the control block structure
	main_thread = (TCB*) malloc(sizeof(TCB));

}


/* * * * * * * * * * * */
/* pthread equivalents */
/* * * * * * * * * * * */

// Allocates a Thread Control Block and stack for the new thread and
// adds it to the scheduling queue. Returns the id of the new thread.
// @todo: add arguments and return values to TCB
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
	
	// we haven't joined anything yet
	tcb->joined_tid = -1;

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

	// save our context before jumping!
    if ( sigsetjmp(main_thread->jbuf,1) == 0 ) {
    	siglongjmp(index->jbuf,1);
    }
}

// Changes running thread state to ready and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_yield( void ) {
	// change our state
	running_thread->state = Ready;
	
	suspend_queue_add(running_thread);
	
	// save our context before jumping!
    if ( sigsetjmp(running_thread->jbuf,1) == 0 ) {
		// go to our main scheduler thread, wherever
		// it left off
		running_thread = main_thread;
    	siglongjmp(main_thread->jbuf,1);
    }
	
    return 0;
}             

// Returns currently running thread id.
int uthread_self( void ) {
    return running_thread->tid;
}        

// Changes running thread state to waiting and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_join( int tid, void **retval ) {
	// change our state
	running_thread->state = Waiting;
	
	// make a note of who we're waiting for
	running_thread->joined_tid;
	
	suspend_queue_add(running_thread);
	
    // save our context before jumping!
    if ( sigsetjmp(running_thread->jbuf,1) == 0 ) {
		// go to our main scheduler thread, wherever
		// it left off
		running_thread = main_thread;
    	siglongjmp(main_thread->jbuf,1);
    }
	
	// if we ended up here, the joined thread must be finished!
	// @todo: assign retval to our joined thread's return value?
	
	return 0;
}

/* * * * * * * * * * * */
/* uthread control     */
/* * * * * * * * * * * */
int uthread_init( int time_slice ) {
    return 0;
}

int uthread_terminate( int tid ) {
	longjmp(main_thread->jbuf, 1);
    return 0;
}

int uthread_suspend( int tid ) {
    return 0;
}

int uthread_resume( int tid ) {
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
