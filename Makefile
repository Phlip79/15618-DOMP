DEBUG=0
CC=mpic++
OMP=-openmp -msse4.2 -msse2 -msse3
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG)
LDFLAGS= -lm

all: domp arraySum


domp: lib/domp.o
	(cd lib; make)

arraySum: domp tests/arraySum.cpp
	$(CC) $(CFLAGS) $(OMP) -c -o build/arraySum.o tests/arraySum.cpp
	$(CC) -o build/arraySum build/arraySum.o lib/domp.o $(LDFLAGS)