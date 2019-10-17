
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
    - Purpose: Used to free a lock after running a section of code that should not be interrupted.  The flag for the lock is set to 0 or free prior to exiting release.  This is used similar to acquire above.  
    - Input: a pointer to a lock structure variable.
    - Output: A thread will release a lock.
    - Return Value: Zero upon success.

## Customized API Items Added

## Passing Input/Output Parameters

## Effects of Time Slice

## Critical Sections of Code Where Locks Required

Locks were used for all of the queues to ensure that the thread does not get disrupted when adding or removing a thread from a queue.  If this were not used a thread could get "lost" or the wrong thread could get added to a queue.

## Scheduling Algorithum

The scheduling algorithum first saves the state of the running thread.  Next the main thread reviewed the finished queue to see if there are threads to remove and free and does so if they are there.  The scheduler will next check the ready queue and select another thread if one is available.  Otherwise the running thread will continue.

## Signal Masks

## Test Files
To build test files use `make`

**HelloThread**
This tests the functions for uthread_create, uthread_join, uthread_self, and uthread_terminate.

**new_uthread_demo**
This tests uthread_init.

```
Need to test out uthread_yield, uthread_suspend, uthread_resume, and create locks to acquire and release.
```