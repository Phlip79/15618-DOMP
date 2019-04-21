DEBUG=0
CC=mpic++
OMP=-fopenmp -msse4.2 -msse2 -msse3
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG)
LDFLAGS= -lm

all: domp arraySum


domp: lib/domp.o
	(cd lib; make)

arraySum: domp tests/arraySum.cpp
	$(CC) $(CFLAGS) $(OMP) -o build/arraySum tests/arraySum.cpp lib/domp.o $(LDFLAGS)