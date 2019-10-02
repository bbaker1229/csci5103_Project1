
#include "uthread.h"

// pthread equivalents
int uthread_create( void *( *start_routine )( void * ), void *arg ) {        // returns tid
    return 0;
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

// uthread control
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

int lock_init( lock_t *lock1 ) {
    return 0;
}

int acquire( lock_t *lock1 ) {
    return 0;
}

int release( lock_t *lock1 ) {
    return 0;
}
