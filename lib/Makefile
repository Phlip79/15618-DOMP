MPI =-DDEBUG_DOMP
DEBUG=0
CFLAGS=-c -g -O3 -Wall -DDEBUG=$(DEBUG) -DPROFILING=$(PROFILING)

SRCS = domp.cpp DataManager.cpp CommandManager.cpp util/SplitList.cpp util/CycleTimer.cpp
HFILES = domp.h DataManager.h CommandManager.h util/SplitList.h util/DoublyLinkedList.h util/CycleTimer.h

OBJS := ${SRCS:.cpp=.o}

all: domplib.a

%.o : %.cpp
	$(MPICC) $(CFLAGS) $(MPI) $< -o $@

domplib.a: $(OBJS) #let's link library files into a static library
	@echo "PROFILING is $DPROFILING"
	ar rcu domplib.a $(OBJS)
	ranlib domplib.a

clean:
	rm -f *.o util/*.o *.a