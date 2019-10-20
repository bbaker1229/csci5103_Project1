
# csci5103_Project1

Intro to Operating Systems Project 1 - User-level Thread Library

- Bryan Baker - bake1358@umn.edu
- Alice Anderegg - and08613@umn.edu
- Hailin Archer - deak0007@umn.edu

## Assumptions

- asdf

## Function Definitions

### Pthread Equivalents

- **uthread_create**:
    - Purpose: Used to create a thread and add it to the ready queue.  It allocates a thread using the TCB structure created in the library, sets the thread state, creates a tid for the thread, allocates the stack, sets up the context for jumping back and forth between threads using sigsetjmp, and finally adds the newly created thread to the ready list.  
    - Input: Takes two inputs - a pointer to a worker function and an argument for the worker function.
    - Output: Creates thread and adds to ready queue.
    - Return Value: Will return the tid of the created thread.

- **uthread_yield**:
    - Purpose: Used by a thread to add itself back to the ready queue and return to the scheduler.
    - Input: No input
    - Output: Moves running thread to ready queue.
    - Return Value: No return value.

- **uthread_self**:
    - Purpose: Used to return the tid from a running thread. 
    - Input: No input
    - Output: Tid
    - Return Value: Returns the tid of the running thread.

- **uthread_join**:
    - Purpose: Used to join a thread to the main thread
    - Input: Takes two inputs - a tid and a pointer to a returnvalue array.
    - Output: Joins tid to main thread.
    - Return Value: Returns zero on success.

### Uthread Control

- **uthread_init**:
    - Purpose: Used to call a setup function which will create locks on ready, suspend, and finished queues, creates the main thread, and sets up a virtual timer to force context swiching.  A time slice of zero is no time slice and will not initialize the timer. 
    - Input: Takes an integer for the value to set the time slice for context switching.  A zero time slice will not set a timer.
    - Output: Initializes main thread and sets timer.
    - Return Value: No real return value.

- **uthread_terminate**:
    - Purpose: Used to send a thread to the finished queue.
    - Input: A tid
    - Output: Finds tid and adds to the finished queue
    - Return Value: Zero upon success.

- **uthread_suspend**:
    - Purpose: Used to send a thread to the suspend queue.  It will first see if the running thread is the tid to suspend and then move to check the ready queue.  When found it will move the thread to the suspend queue.  
    - Input: A tid
    - Output: Adds thread to suspend queue.
    - Return Value: Zero upon success.

- **uthread_resume**:
    - Purpose: Used to remove a thread from the suspend queue.  This function will search in the suspend queue and move the tid to the ready queue.  
    - Input: A tid
    - Output: Moves thread from suspend queue to ready queue.
    - Return Value: Zero upon success.

### Locks

- **lock_init**:
    - Purpose: Used to create a lock.  It takes a pointer to a lock structure which has a flag to define free or busy.
    - Input: A pointer to a lock structure variable.
    - Output: A lock will be created.
    - Return Value: Zero upon success.

- **acquire**:
    - Purpose: Used to secure a lock prior to running a section of code that should not be interrupted.  Once the lock is acquired the flag is set to 1 or busy.  For example these are used in this code when accessing the queues (ready, suspend, and finished).
    - Input: a pointer to a lock structure variable.
    - Output: A thread will acquire a lock.
    - Return Value: Zero upon success.

- **release**:
    - Purpose: Used to free a lock after running a section of code that should not be interrupted.  The flag for the lock is set to 0 or free prior to exiting release.  This is used opposite to acquire above.  
    - Input: a pointer to a lock structure variable.
    - Output: A thread will release a lock.
    - Return Value: Zero upon success.

## Customized API Items Added

None

## Passing Input/Output Parameters

Input and output parameters for this implementation are only passed via global variables.

## Effects of Time Slice

A shorter time slice gives a shorter amount of time to the threads before it is preempted back to the scheduler.  

## Critical Sections of Code Where Locks Required

Locks were used for all of the queues to ensure that the thread does not get disrupted when adding or removing a thread from a queue.  If this were not used a thread could get "lost" or the wrong thread could get added to a queue.

## Scheduling Algorithum

The scheduling algorithum first saves the state of the running thread.  Next the suspend queue is checked to see if there was a thread waiting for another thread to finish.  Afterwards, the main thread reviews the finished queue to see if there are threads to remove and free and does so if they are there.  The scheduler will next check the ready queue and select another thread if one is available.  Otherwise the running thread will continue.

## Signal Masks

Only one signal mask is used for this implementation.  The mask is implemented globaly and if a signal is recieved during processing of the running thread the control is sent to the scheduler.  Once a new thread is chosen, if required, the signal mask is reset.  

## Test Files
To build test files use `make`

**T1_hello_thread**
This tests the functions for uthread_init, uthread_create, uthread_join, uthread_self, and uthread_terminate.  This functions similar to the hello thread example from the book. 
10 threads are created, each thread prints a Hello message and then each thread is returned to the main thread.  The main thread then completes and the program ends.

**T2_resume_test**
This primarily tests the functions for uthread_suspend and uthread_resume.  Two threads are created and then one thread is suspended and then resumed, then the other thread is suspended and resumed.  

**T3_lock_test**
this primarily tests the lock functions for lock_init, acquire, and release, but also tests uthread_yield.  Two threads are created and then one thread acquires a lock and yields to the other thread.  The other thread continually tried to acquire the lock, but yields.  Finally the first thread will release the lock and then the second thread can acquire the lock.  This process is repeated.

**T4_uthread_demo**
This tests uthread_init and functions similar to the demo given as an example for this project.  10 threads are created and each thread starts counting using separate counters to see if context switching will keep their set points.  This runs continually.  

**test1**
Test provided for project.  This creates a number of threads which process points to give an approximation to pi.  

**test2**
Test provided for project.  This creates a number of threads and suspends the first thread.  The rest of the threads process over time and then the first thread is resumed.  You can see that the first section of the test the thread numbers do not include thread 1 and then after it is resumed you can see that thread 1 has returned.  
