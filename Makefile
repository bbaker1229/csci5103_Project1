BINARIES := T1_hello_thread T2_resume_test T3_lock_test T4_uthread_demo test1 test2 uthread_demo

all: $(BINARIES)
OBJ = uthread.o 
LIB = uthread.h

CC =gcc
CFLAGS = -Wall -g

# Clean Up Objects, Executables, Dumps out of source directory
.PHONY: clean

clean:
	rm -f *.o $(BINARIES)

tags:
	etags *.h *.c *.cpp

%.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

T1_hello_thread: T1_hello_thread.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

T2_resume_test: T2_resume_test.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

T3_lock_test: T3_lock_test.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

T4_uthread_demo: T4_uthread_demo.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@ 

test1: test1.cpp $(OBJ) $(LIB)
	g++ $(CFLAGS) $^ -o $@

test2: test2.cpp $(OBJ) $(LIB)
	g++ $(CFLAGS) $^ -o $@

uthread_demo: uthread_demo.c 
	$(CC) $(CFLAGS) $^ -o $@
