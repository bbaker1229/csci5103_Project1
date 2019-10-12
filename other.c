#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "uthread.h"

#define STACK_SIZE 4096
#define TCB_SIZE 8192
#define MAXTHREADS 25

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
} tcb_t;

tcb_t threads[MAXTHREADS];

int uthread_create( void *( *start_routine )( void * ), void *arg ) {

    // allocate TCB and stack
    tcb_t *tcb = (tcb_t *) malloc(TCB_SIZE);

    address_t sp, pc;

    // Initialize registers so that when thread is resumed, it will start running at stub.  The stack starts at the top of the allocated region and grows down.

    // stack pointer is calculated this way because on Linux, the stack pointer
    // grows from high address to low address.
    sp = (address_t) *tcb->stack + STACK_SIZE - sizeof(int); 
    pc = (address_t) *( *start_routine );

    // Create a stack frame by pushing stubs arguments and start address onto the stack: func, arg
    
    sigsetjmp(jbuf[0],1);

    // the index is recorded in glibc soucre code, if you are interested, check
    // glibc source code /x86_64/jmpbuf-offsets.h
    // the reason why we need translate_address() function is because glibc
    // mangles(i.e. encrypts) pointer in jmpbuf to ensure security
    (jbuf[0]->__jmpbuf)[JB_SP] = translate_address(sp);
    (jbuf[0]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&jbuf[0]->__saved_mask);     

    // Create another stack frame so that thread_switch works correctly.






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

	//add the new_tcb to the thread scheduler queue
    ready_queue_add(tcb);

	return tcb->tid;
}