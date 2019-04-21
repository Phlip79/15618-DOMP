CC=g++

MPI =-DDEBUG_DOMP
MPICC = mpic++


DEBUG=0
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG)
#LDFLAGS= -lm
DDIR = ./data

CFILES = domp.cpp util/linkedlist.cpp
HFILES = domp.h util/linkedlist.h

all: domp

domp: $(CFILES) $(HFILES)
	$(MPICC) $(CFLAGS) $(MPI) $(CFILES)

clean:
	rm -f *.o

