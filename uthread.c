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
#define QUEUE_SIZE		100

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
	int joined_tid;
} TCB;

typedef struct {
	int start;
	int end;
	int count;
	TCB* tcb_queue[QUEUE_SIZE];
} tcb_fifo_t;

// count of threads created by system.
// used to issue tid.
int thread_count;

// scheduling queues
tcb_fifo_t ready_queue;
tcb_fifo_t suspend_queue;

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

// adds the tcb to the end of the queue
// returns -1 on fail, 0 on success
int queue_add(TCB* tcb, tcb_fifo_t* queue) {

	// make sure we have space for the new tcb
	if (queue->count >= QUEUE_SIZE) {
		printf("queue_add: could not add new tcb. queue already at capacity\n");
		return -1;
	}

	// change status depending on which queue we're adding to
	if ( queue == &ready_queue ) {
		tcb->state = READY;
	} else if ( queue == &suspend_queue ) {
		tcb->state = SUSPEND;
	}

	// store the tcb pointer and increment our queue counts
	queue->tcb_queue[queue->end] = tcb;
	queue->count++;
	queue->end = (queue->end + 1) % QUEUE_SIZE;

	return 0;
}

// returns the TCB at the start of the queue
// or NULL if queue is empty
TCB* queue_remove(tcb_fifo_t* queue) {
	TCB* tcb;

	// make sure we have something to get
	if (queue->count == 0) {
		return NULL;
	}

	// grab the tcb at the queue start and change the queue counts
	tcb = queue->tcb_queue[queue->start];
	queue->count--;
	queue->start = (queue->start + 1) % QUEUE_SIZE;

	return tcb;
}


void scheduler()
{
	int i;
	TCB* next;

		// first things first, let's sort through our suspend queue and
		// see what house keeping we need to do
		for (i = 0; i < suspend_queue.count; i++) {
			// if the thread in the suspend_queue doesn't need to wait anymore, then move it to
			// the ready queue
			//@todo: any other conditions we need to check? why else is a thread suspended?
			if (suspend_queue.tcb_queue[(suspend_queue.start + i) % QUEUE_SIZE]->joined_tid == -1) {
				queue_add(queue_remove(&suspend_queue), &ready_queue);
			}
		}

		// try to give someone else a chance to run
		if (ready_queue.count > 0 ) {
			next = queue_remove(&ready_queue);
			queue_add(running_thread, &ready_queue);
		// otherwise, see if we were already running someone
		} else if (running_thread != NULL) {
			next = running_thread;
		// hmm, there doesn't seem to be anyone we can run.
		} else {
			// returning from the scheduler likely means we're leaving the library and headed
			// back to the calling code
			return;
		}

		// context switch to the new running thread
		next->state = RUNNING;
		running_thread = next;
		siglongjmp(running_thread->jbuf,1);

}

int uthread_setup() {
	// allocate the control block structure
	main_thread = (TCB*) malloc(sizeof(TCB));

	if (main_thread == NULL) {
		printf("could not allocate main_thread TCB\n");
		return -1;
	}

	main_thread->state = RUNNING;
	main_thread->tid = 0;
	main_thread->stack_size = STACK_SIZE;
	char* stack = (char*) malloc(main_thread->stack_size * sizeof(char));
	if (stack == NULL) {
		free(main_thread);
		printf("failed to allocate main_thread stack\n");
		return -1;
	}
	main_thread->stack = stack;
    main_thread->joined_tid = -1;

	// point the stack pointer and program counter at the right things
	main_thread->sp = (address_t)stack + STACK_SIZE -sizeof(int);
	main_thread->pc = (address_t)scheduler;

	// store context for jumping
    sigsetjmp(main_thread->jbuf,1);
    (main_thread->jbuf->__jmpbuf)[JB_SP] = translate_address(main_thread->sp);
    (main_thread->jbuf->__jmpbuf)[JB_PC] = translate_address(main_thread->pc);
    sigemptyset(&main_thread->jbuf->__saved_mask);

    siglongjmp(main_thread->jbuf,1);
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
	tcb->state = INIT;

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
    queue_add(tcb, &ready_queue);

	return tcb->tid;
}

// Returns currently running thread id.
int uthread_self( void ) {
    return running_thread->tid;
}

// Changes running thread state to ready and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_yield( void ) {
	// @todo: needs to call uthread_init(TIME_SLICE); somewhere to reset the time after the thread yields.

	// add running thread to the end of the ready queue
	queue_add(running_thread, &ready_queue);
	
	// save our context before jumping!
    if ( sigsetjmp(running_thread->jbuf,1) == 0 ) {
		// go to our main scheduler thread, wherever
		// it left off
    	siglongjmp(main_thread->jbuf,1);
    }
	
    return 0;
}

// Changes running thread state to waiting and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_join( int tid, void **retval ) {
	// make a note of who we're waiting for
	running_thread->joined_tid = tid;
	
	queue_add(running_thread, &suspend_queue);
	
    // save our context before jumping!
    if ( sigsetjmp(running_thread->jbuf,1) == 0 ) {
		// go to our main scheduler thread, wherever
		// it left off
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
	longjmp(main_thread->jbuf, 1);
    return 0;
}

int uthread_suspend( int tid ) {
	if(tcb[tid].state == RUNNING) {
		tcb[tid].state == SUSPEND;
		//move to suspend queue...
		//pull a new thread off the ready queue and start.
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

int lock_init( lock_t *lock1 ) {
    // 0: lock is available, 1: lock is held
    lock1->flag=0;
    return 0;
}

int acquire( lock_t *lock1 ) {
    while (TAS(&lock1->flag,1)==1){
        //uthread_suspend(); // yield when lock is not available. Need to know how to pass current thread tid to this call
    }    
    return 0;
}

int release( lock_t *lock1 ) {
    lock1->flag=0;
    return 0;
}
