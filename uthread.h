
#define NAME_SIZE   10

typedef struct {
    char name[NAME_SIZE];
} lock_t;

void uthread_setup();

// pthread equivalents
extern int 
uthread_create( void *( *start_routine )( void * ), void *arg );         // returns tid

extern int 
uthread_yield( void );                                                   // return value not meaningful

extern int 
uthread_self( void );                                                    // returns tid

void
uthread_start(int tid);

extern int 
uthread_join( int tid, void **retval );

// uthread control
extern int 
uthread_init( int time_slice );

extern int 
uthread_terminate( int tid );

extern int 
uthread_suspend( int tid );

extern int 
uthread_resume( int tid );

extern int 
lock_init( lock_t *lock1 );

extern int 
acquire( lock_t *lock1 );

extern int 
release( lock_t *lock1 );
