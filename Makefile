DEBUG=0
CC=mpic++
OMP=-fopenmp -msse4.2 -msse2 -msse3
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++11
LDFLAGS= -lm

DOMP_LIB = lib/domplib.a

all: arraySum testDataTransfer logisticRegression

DOMP_LIB:
	(cd lib; make)

arraySum: DOMP_LIB tests/arraySum.cpp
	$(CC) $(CFLAGS) $(OMP) -o build/arraySum tests/arraySum.cpp $(DOMP_LIB) $(LDFLAGS)

testDataTransfer: DOMP_LIB tests/testDataTransfer.cpp
	$(CC) $(CFLAGS) $(OMP) -o build/testDataTransfer tests/testDataTransfer.cpp $(DOMP_LIB) $(LDFLAGS)

logisticRegression: DOMP_LIB test/logisticRegression.cpp
    $(CC) $(CFLAGS) $(OMP) -o build/logisticRegression tests/logisticRegression.cpp $(DOMP_LIB) $(LDFLAGS)

clean:
	rm -rf build/*.o
	(cd lib; make clean)