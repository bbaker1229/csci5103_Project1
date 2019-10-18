/* 
	Definition of uthread functions.
*/

// include header files
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

// Include header file for this file.
#include "uthread.h"

// Define constants
#define SECOND 			1000000 	// time in microseconds
#define STACK_SIZE 		4096		// size in bytes
#define QUEUE_SIZE		150			// Queue size which will be used as the maximum num of threads to create.

// The following is code for 64 or 32 bit processing.  No need to change.
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
	int main;
	int joined_tid;
} TCB;

// Define struture of queues
typedef struct {
	int start;
	int end;
	int count;
	TCB* tcb_queue[QUEUE_SIZE];
	lock_t lock;
} tcb_fifo_t;

// count of threads created by system.
// used to issue tid.
int thread_count;

// scheduling queues
tcb_fifo_t ready_queue;
tcb_fifo_t suspend_queue;
tcb_fifo_t finished_queue;

// If we are going to refer to threads by tid can we create an array of TCBs?
TCB* threads[QUEUE_SIZE];
// Then we can ensure that all queues will be the correct size to hold all the threads and we can
// pass around tid values rather than TCB pointers?

// currently running thread
TCB* running_thread;
sigset_t mask;

// our main scheduler thread
TCB* main_thread;

// test and set atomic lock mechanism - Probably do not have to change this.
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

	acquire(&queue->lock);

	// make sure we have space for the new tcb
	if (queue->count >= QUEUE_SIZE) {
		printf("queue_add: could not add new tcb. queue already at capacity\n");
		release(&queue->lock);
		return -1;
	}

	// change status depending on which queue we're adding to
	if ( queue == &ready_queue ) {
		tcb->state = READY;
	} else if ( queue == &suspend_queue ) {
		tcb->state = SUSPEND;
	} else if ( queue == &finished_queue) {
		tcb->state = FINISHED;
	}

	// store the tcb pointer and increment our queue counts
	queue->tcb_queue[queue->end] = tcb;
	queue->count++;
	queue->end = (queue->end + 1) % QUEUE_SIZE;

	release(&queue->lock);
	return 0;
}

// returns the TCB at the start of the queue
// or NULL if queue is empty
TCB* queue_remove(tcb_fifo_t* queue) {
	TCB* tcb;
	acquire(&queue->lock);

	// make sure we have something to get
	if (queue->count == 0) {
		release(&queue->lock);
		return NULL;
	}

	// grab the tcb at the queue start and change the queue counts
	tcb = queue->tcb_queue[queue->start];
	queue->count--;
	queue->start = (queue->start + 1) % QUEUE_SIZE;

	release(&queue->lock);
	return tcb;
}

// Returns whether a thread with TID is present in the queue.
bool is_tid_in_queue(int tid, tcb_fifo_t* queue) {
	int i;
	acquire(&queue->lock);

	// go through all the entries in the queue and if one matches the tid,
	// return true. otherwise return false.
	for (i = 0; i < queue->count; i++) {
		if (queue->tcb_queue[(queue->start + i) % QUEUE_SIZE]->tid == tid) {
			release(&queue->lock);
			return true;
		}
	}

	release(&queue->lock);
	return false;
}

// Moves the given tid to the front of the queue.
void move_tid_to_front(int tid, tcb_fifo_t* queue) {
	int i = 0;

	// move the front of the queue to the back until the TCB at the
	// start of the queue has the correct TID
	while (queue->tcb_queue[queue->start]->tid != tid ) {
		queue_add(queue_remove(queue), queue);

		i++;
		// double check we're not just going around in circles
		if (i >= queue->count) {
			printf("TID not found in queue!\n");
			break;
		}
	}
}

// Returns a pointer to the TCB if one exists with the given TID.
// Returns NULL if one can't be found. Note: this function
// will remove the TCB* from where it is found, assuming you want
// to move or delete it anyway.
TCB* find_tcb_by_tid(int tid) {
	TCB* temp;

	// first check the running thread
	if (running_thread != NULL && running_thread->tid == tid) {
		temp = running_thread;
		running_thread = NULL;
		return temp;
	}

	// next check ready queue
	else if (is_tid_in_queue(tid, &ready_queue)) {
		move_tid_to_front(tid, &ready_queue);
		return queue_remove(&ready_queue);
	}

	// it better be in the suspend queue then...
	else if (is_tid_in_queue(tid, &suspend_queue)) {
		move_tid_to_front(tid, &suspend_queue);
		return queue_remove(&suspend_queue);
	}

	// guess we don't have that thread in our system
	return NULL;
}

// Create the details for the scheduler.
void scheduler()
{
	TCB* next;
	TCB* finished;

	sigprocmask(SIG_BLOCK, &mask, NULL);

	//printf("Number of tids in ready queue: %i\n", ready_queue.count);

	// quick store our context
	if (running_thread != NULL) {
		if (sigsetjmp(running_thread->jbuf, 1) == 1) {
			return;
		}
	}

	// check if main is waiting for a thread to finish
	if (main_thread->joined_tid != -1) {
		if (is_tid_in_queue(main_thread->joined_tid, &finished_queue)) {
			main_thread->joined_tid = -1;
			siglongjmp(main_thread->jbuf,1);
		}
	}

	// free everything in the finished queue
	while (finished_queue.count > 0) {
		finished = queue_remove(&finished_queue);
		free(finished->stack);
		free(finished);
	}

	// try to give someone else a chance to run
	if (ready_queue.count > 0 ) {
		next = queue_remove(&ready_queue);
		if (running_thread != NULL) {
			queue_add(running_thread, &ready_queue);
		}
	// otherwise, see if we were already running someone
	} else if (running_thread != NULL) {
		next = running_thread;
	// hmm, there doesn't seem to be anyone we can run.
	} else {
		// let's kick back to main then
		siglongjmp(main_thread->jbuf,1);
	}

	// context switch to the new running thread
	next->state = RUNNING;
	running_thread = next;

	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	siglongjmp(running_thread->jbuf,1);

}

// Create the main thread
int uthread_setup() {

	// initialize locks
	lock_init(&ready_queue.lock);
	lock_init(&suspend_queue.lock);
	lock_init(&finished_queue.lock);

	// allocate the control block structure
	main_thread = (TCB*) malloc(sizeof(TCB));

	if (main_thread == NULL) {
		printf("could not allocate main_thread TCB\n");
		return -1;
	}

	main_thread->state = RUNNING;
	main_thread->tid = 9999;
	main_thread->main = 1;
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

	// store context for jumping
    sigsetjmp(main_thread->jbuf,1);
    (main_thread->jbuf->__jmpbuf)[JB_SP] = translate_address(main_thread->sp);
    sigemptyset(&main_thread->jbuf->__saved_mask);

	queue_add(main_thread, &ready_queue);
	//printf("Ready queue count: %i\n", ready_queue.count);
	//printf("Main Thread added to ready queue.\n");

	return 0;
}


/* * * * * * * * * * * */
/* pthread equivalents */
/* * * * * * * * * * * */

// Allocates a Thread Control Block and stack for the new thread and
// adds it to the scheduling queue. Returns the id of the new thread.
int uthread_create( void *( *start_routine )( void * ), void *arg ) {

	// allocate the control block structure
	TCB* tcb = (TCB*) malloc(sizeof(TCB));
	if (tcb == NULL) {
		printf("uthread_create: Failed to allocate TCB\n");
		return -1;
	}

	// start the state at Init
	tcb->state = INIT;
	tcb->main = 0;

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

	//printf("Ready queue count: %i\n", ready_queue.count);
	//printf("Thread added to ready queue.\n");

	return tcb->tid;
}

// Returns currently running thread id.
int uthread_self( void ) {
	if ( running_thread == NULL ) {
		return main_thread->tid;
	} else {
		return running_thread->tid;
	}
}

// Changes running thread state to ready and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_yield( void ) {
	// add running thread to the end of the ready queue
	queue_add(running_thread, &ready_queue);
	
	// save our context before jumping!
    if ( sigsetjmp(running_thread->jbuf,1) == 0 ) {
		// go to our scheduler
    	running_thread = NULL;
    	scheduler();
    }

    return 0;
}

// Changes main thread state to waiting and adds it
// to the suspend_queue before returning control to the
// main_thread;
int uthread_join( int tid, void **retval ) {
	// make a note of who we're waiting for
	main_thread->joined_tid = tid;
	
    // save our context before jumping!
    if ( sigsetjmp(main_thread->jbuf,1) == 0 ) {
		// go to our scheduler
    	scheduler();
    }
	
	// if we ended up here, the joined thread must be finished!
	return 0;
}

/* * * * * * * * * * * */
/* uthread control     */
/* * * * * * * * * * * */
int uthread_init( int time_slice ) {
	struct itimerval timer;

	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	signal(SIGVTALRM, (void (*)(int))scheduler);

	//sigprocmask(SIGVTALRM,&mask,NULL);

	uthread_setup();

	if (time_slice != 0) {
		timer.it_value.tv_sec = time_slice / SECOND;
 		timer.it_value.tv_usec = time_slice % SECOND;
		timer.it_interval.tv_sec = time_slice / SECOND;
		timer.it_interval.tv_usec = time_slice % SECOND;
		setitimer (ITIMER_VIRTUAL, &timer, NULL);
	}

    return 0;
}

int uthread_terminate( int tid ) {
	// find where this thing is
	TCB* thread = find_tcb_by_tid(tid);

	// if we actually found something, add it to the finished_queue
	if ( thread != NULL ) {
		queue_add(thread, &finished_queue);
	}

	scheduler();
    return 0;
}

int uthread_suspend( int tid ) {
	TCB* next = NULL;

	if (running_thread != NULL && running_thread->tid == tid) {
		next = running_thread;
		running_thread = NULL;
	}
	else if (is_tid_in_queue(tid, &ready_queue)) {
		move_tid_to_front(tid, &ready_queue);
		next = queue_remove(&ready_queue);
	}

	if (next != NULL) {
		queue_add(next, &suspend_queue);
	}

    return 0;
}

int uthread_resume( int tid ) {
	TCB* next = NULL;

	// quick store our context
	if (running_thread != NULL) {
		if (sigsetjmp(running_thread->jbuf, 1) == 1) {
			return;
		}
	}

	if (is_tid_in_queue(tid, &suspend_queue)) {
		move_tid_to_front(tid, &suspend_queue);
		//queue_add(queue_remove(&suspend_queue), &ready_queue);
		next = queue_remove(&suspend_queue);
		queue_add(running_thread, &ready_queue);
		running_thread = next;
		siglongjmp(running_thread->jbuf,1);
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
        uthread_yield();
    }    
    return 0;
}

int release( lock_t *lock1 ) {
    lock1->flag=0;
    return 0;
}
