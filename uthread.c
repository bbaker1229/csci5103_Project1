
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "uthread.h"

#define SECOND 			1000000 	// time in microseconds
#define STACK_SIZE 		4096	// size in bytes
#define MAX_THREADS		64

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
typedef struct tcb_t{
	int tid; // thread id
	thread_state state;
	int stack_size;
	char* stack;
	address_t sp;
	address_t pc;
} TCB;

// currently running thread_id
int tid;

// @todo: this is not the right way to store these. ultimately I think sigjmp_buf
// should probably live in  each tcb, and the control_blocks should be stored as
// a linked list.
sigjmp_buf jbuf[MAX_THREADS];
TCB* control_blocks[MAX_THREADS];

// test and set atomic lock mechanism
int TAS(volatile int *addr, int newval){
    int result = newval;
    asm volatile("lock; xchg %0, %1"
                 : "+m" (*addr), "=r" (result)
                 : "1" (newval)
                 : "cc");
    return result;

}

/* * * * * * * * * * * */
/* pthread equivalents */
/* * * * * * * * * * * */

// Allocates a Thread Control Block and stack for the new thread and
// adds it to the scheduling queue. Returns the id of the new thread.
// Note: this ignores the second void* arg at this time.
int uthread_create( void *( *start_routine )( void * ), void *arg ) {

	// allocate the control block structure
	TCB tcb = (TCB*) malloc(sizeof(TCB));
	if (tcb == NULL) {
		printf("uthread_create: Failed to allocate TCB\n");
		return -1;
	}

	// start the state at Init
	tcb->state = Init;

	// assign an id number and increment for the next thread
	if (tid >= MAX_THREADS - 2) {
		free(tcb);
		printf("uthread_create: Exceeded max number of threads: %d\n", MAX_THREADS);
	}
	tcb->tid = tid++;

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

	// save a pointer to our tcb
	control_blocks[tcb->tid] = &tcb;

	// store context for jumping
	// @todo: I'm not sure this belongs in the create function
    sigsetjmp(jbuf[tcb->tid],1);
    (jbuf[tcb->tid]->__jmpbuf)[JB_SP] = translate_address(control_blocks[tcb->tid]->sp);
    (jbuf[tcb->tid]->__jmpbuf)[JB_PC] = translate_address(control_blocks[tcb->tid]->pc);
    sigemptyset(&jbuf[0]->__saved_mask);

	// @todo: add the new_tcb to the thread scheduler queue

	return tcb->tid;
}       

int uthread_yield( void ) {                                                  // return value not meaningful
    return 0;
}             

int uthread_self( void ) {                                                   // returns tid
    return 0;
}        

int uthread_join( int tid, void **retval ) {
    return 0;
}

/* * * * * * * * * * * */
/* uthread control     */
/* * * * * * * * * * * */
int uthread_init( int time_slice ) {
    return 0;
}

int uthread_terminate( int tid ) {
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
