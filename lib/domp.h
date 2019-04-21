//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_DOMP_H
#define DOMP_DOMP_H

#include <iostream>
#include <list>
#include <map>

#include <stdbool.h>
#include "util/linkedlist.h"


namespace domp {
  class DOMP;
  class Variable;
  class Interval;

#define DOMP_INIT(argc, argv) ()
#define DOMP_REGISTER(var, type) ()
#define DOMP_PARALLELIZE(var, offset, size) { \
  *offset = 0;\
  *size = var; \
}
#define DOMP_SHARED(var, offset, size) ()
#define DOMP_SYNC ()
#define DOMP_REDUCE(var, op) ()
}


class domp::DOMP{
  int rank;
  int clusterSize;
  std::map<std::string, Variable> varList;
 public:
  DOMP(int * argc, char ***argv);
  void Parallelize(int totalSize, int *offset, int *size);
  void FirstShared(void *ptr, int offset, int size);
  void Shared(void *ptr, int offset, int size);
  int Reduce();
  void Synchronize();
};

class domp::Interval {
  int start;
  int size;
  std::list <int> nodes;
  Interval(int start, int size) {
    this->start = start;
    this->size = size;
  }
  void addNode(int nodeId) {
    this->nodes.push_back(nodeId);
  }
};

class domp::Variable {

};

typedef struct domp_interval {
  int start;
  int size;
  int nodeId;
} domp_interval_t;

typedef struct domp_var {
  void *ptr;
  List intervals;
} domp_var_t;

typedef struct domp_struct{
  int rank;
  int clusterSize;
  List var_list;
} domp_t;



void domp_error() {

}

enum DOMP_TYPE{DOMP_INT, DOMP_FLOAT};

bool DOMP_init(domp_t *dompObject, int *argc, char ***argv);

void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size);

void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);

void DOMP_shared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);


void DOMP_reduce(domp_t *dompObject, int tag, int *sum, int value);

#endif //DOMP_DOMP_H
