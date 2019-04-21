CC=gcc

MPI=-DMPI
MPI+=-DDEBUG_DOMP
MPICC = mpicc


DEBUG=0
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG)
#LDFLAGS= -lm
DDIR = ./data

CFILES = domp.c
HFILES = domp.h

all: domp

domp: $(CFILES) $(HFILES)
	$(MPICC) $(CFLAGS) $(MPI) -o domp.o $(CFILES)

clean:
	rm -f domp.o

