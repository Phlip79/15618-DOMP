CC=gcc

MPI=-DMPI
MPI+=-DDEBUG_DOMP
MPICC = mpicc


DEBUG=0
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG)
#LDFLAGS= -lm
DDIR = ./data

CFILES = domp.c util/linkedlist.c
HFILES = domp.h util/linkedlist.h

all: domp

domp: $(CFILES) $(HFILES)
	$(MPICC) $(CFLAGS) $(MPI) $(CFILES)

clean:
	rm -f *.o

