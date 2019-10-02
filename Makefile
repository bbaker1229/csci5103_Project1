# Source, Executable, Includes, Library Defines
INCL = headername.h
SRC = code1.c code2.c 
OBJ = $(SRC:.c=.o)
LIBS = -lgen
EXE = ProgramName

# Compiler, Linker Defines
CC = gcc
CFLAGS = -Wall -g 
LIBPATH = -L.
LDFLAGS = -o $(EXE) # $(LIBPATH) $(LIBS)
CFDEBUG = -Wall -g -DDEBUG $(LDFLAGS)
RM = rm -f

# Compile and Assemble C Source Files into Object Files
%.o: %.c 
	$(CC) -c $(CFLAGS) $*.c 

# Link all Object Files with external Libraries into Binaries
$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ)

# Objects depend on these Libraries
$(OBJ): $(INCL)

# Create a gdb/dbx Capable Executable with DEBUG flags turned on
debug:
	$(CC) $(CFDEBUG) $(SRC)

# Clean Up Objects, Executables, Dumps out of source directory
.PHONY: clean

clean:
	$(RM) $(OBJ) $(EXE) core a.out
