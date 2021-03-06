DEBUG=0
PROFILING=1
MPICC=mpic++
OMP=-fopenmp -msse4.2 -msse2 -msse3
CFLAGS=-g -O3 -Wall -DDEBUG=$(DEBUG) -DPROFILING=$(PROFILING)
LDFLAGS= -lm

all: arraySum testDataTransfer kmeans logisticRegression logisticRegressionSeq

export MPICC
export PROFILING
export ROOT_DIR=${PWD}

DOMP_LIB = ${ROOT_DIR}/lib/domplib.a
OUTPUT_DIR = ${ROOT_DIR}/build
export DOMP_LIB
export OUTPUT_DIR

DOMP_LIB:
	$(MAKE) -C lib

arraySum: DOMP_LIB tests/arraySum.cpp
	$(MPICC) $(CFLAGS) $(OMP) -o build/arraySum tests/arraySum.cpp $(DOMP_LIB) $(LDFLAGS)

testDataTransfer: DOMP_LIB tests/testDataTransfer.cpp
	$(MPICC) $(CFLAGS) $(OMP) -o build/testDataTransfer tests/testDataTransfer.cpp $(DOMP_LIB) $(LDFLAGS)

logisticRegression: DOMP_LIB tests/logistic_regression/logisticRegression.cpp
	$(MPICC) $(CFLAGS) $(OMP) -o build/logisticRegression tests/logistic_regression/logisticRegression.cpp tests/logistic_regression/wtime.cpp $(DOMP_LIB) $(LDFLAGS)

logisticRegressionSeq: DOMP_LIB tests/logistic_regression/logisticRegressionSeq.cpp
	$(MPICC) $(CFLAGS) $(OMP) -o build/logisticRegressionSeq tests/logistic_regression/logisticRegressionSeq.cpp tests/logistic_regression/wtime.cpp $(DOMP_LIB) $(LDFLAGS)

kmeans:
	$(MAKE) -C tests/kmeans

clean:
	rm -rf build/*.o
	$(MAKE) -C tests/kmeans clean
	$(MAKE) -C lib clean