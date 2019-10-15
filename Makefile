BINARIES := HelloThread test1 test2 uthread_demo 

all: $(BINARIES)
LIB = uthread.o uthread.h

CC =gcc
CFLAGS = -Wall -g

# Clean Up Objects, Executables, Dumps out of source directory
.PHONY: clean

clean:
	rm -f *.o $(BINARIES)

tags:
	etags *.h *.c *.cpp

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

HelloThread: HelloThread.c $(PWD)/$(LIB)
	$(CC) $(CFLAGS) $^ -o $@

test1: $(PWD)/example_code/test1.cpp $(PWD)/$(LIB)
	$(CC) $(CFLAGS) $^ -o $@

test2: $(PWD)/example_code/test2.cpp $(PWD)/$(LIB)
	$(CC) $(CFLAGS) $^ -o $@

uthread_demo: $(PWD)/example_code/uthread_demo.c $(PWD)/$(LIB)
	$(CC) $(CFLAGS) $^ -o $@ 
