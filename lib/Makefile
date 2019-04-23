CC=g++

MPI =-DDEBUG_DOMP
MPICC = mpic++


DEBUG=0
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++11
#LDFLAGS= -lm
DDIR = ./data

CFILES = domp.cpp MPIServer.cpp util/SplitList.cpp util/DoublyLinkedList.cpp
HFILES = domp.h MPIServer.h util/SplitList.h util/DoublyLinkedList.h

all: domp

domp: $(CFILES) $(HFILES)
	$(MPICC) $(CFLAGS) $(MPI) $(CFILES)

clean:
	rm -f *.o

