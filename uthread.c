
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

typedef struct _lock_t {
    int flag;
} lock_t;

int lock_init( lock_t *lock1 ) {
    // 0: lock is available, 1: lock is held
    lock->flag=0;
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