MPI =-DDEBUG_DOMP
MPICC = mpic++

DEBUG=1
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG) -std=c++11

SRCS = domp.cpp DataManager.cpp util/SplitList.cpp
HFILES = domp.h DataManager.h util/SplitList.h util/DoublyLinkedList.h

OBJS := ${SRCS:.cpp=.o}

all: domplib.a

%.o : %.cpp
	$(MPICC) $(CFLAGS) $(MPI) $< -o $@

domplib.a: $(OBJS) #let's link library files into a static library
	ar rcu domplib.a $(OBJS)
	ranlib domplib.a

clean:
	rm -f *.o util/*.o *.a