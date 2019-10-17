BINARIES := HelloThread new_uthread_demo uthread_demo test1 test2 Signal

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

HelloThread: HelloThread.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

new_uthread_demo: new_uthread_demo.c $(OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

uthread_demo: uthread_demo.c 
	$(CC) $(CFLAGS) $^ -o $@ 

test1: test1.cpp $(OBJ) $(LIB)
	g++ $(CFLAGS) $^ -o $@

test2: test2.cpp $(OBJ) $(LIB)
	g++ $(CFLAGS) $^ -o $@

Signal: SignalTesting.c 
	$(CC) $(CFLAGS) $^ -o $@
