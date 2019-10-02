
OBJ = uthread_demo.o test1.o test2.o uthread.o

CC =gcc
CFLAGS = -Wall -g

uthread.o: uthread.c uthread.h
	$(CC) -c $(CFLAGS) uthread.c

test1: test1.c uthread.h
	$(CC) -o test1 $(CFLAGS) test1.c

test2: test2.c uthread.h
	$(CC) -o test2 $(CFLAGS) test2.c

uthread_demo: uthread_demo.c 
	$(CC) -o uthread_demo $(CFLAGS) uthread_demo.c 

# Clean Up Objects, Executables, Dumps out of source directory
.PHONY: clean

clean:
	rm -f $(OBJ) uthread test1 test2 uthread_demo
