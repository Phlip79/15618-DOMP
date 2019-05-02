PROJ = matrix_mul
CC = g++

CFLAGS = -Wno-sign-compare -c -g -Wall -I/opt/local/include -I$(HOME)/cppunit/include -O2
LDFLAGS = -L/opt/local/lib -L$(HOME)/cppunit/lib -O2
LIBS = -lcppunit -ldl
OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OMPFLAGS = -fopenmp
SIMDFLAGS = -msse2 -msse3 -msse4.2 -O2 -lm

all: $(PROJ)

$(PROJ): $(OBJS)
	$(CC) $(OMPFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) $(SIMDFLAGS)

%.o : %.cpp
	$(CC) $(OMPFLAGS) $(CFLAGS) $< -o $@ $(SIMDFLAGS)

%.o : %.cpp %.h
	$(CC) $(OMPFLAGS) $(CFLAGS) $< -o $@ $(SIMDFLAGS)

clean:
	rm -f $(PROJ) $(OBJS) *.xml
