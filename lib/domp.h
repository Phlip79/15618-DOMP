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

  enum DOMP_TYPE {DOMP_INT, DOMP_FLOAT};
  enum DOMP_REDUCE_OP {DOMP_ADD, DOMP_SUBTRACT};


  class DOMP;
  class Variable;
  class Interval;

DOMP *domp;

#define DOMP_INIT(argc, argv) { \
  domp = new DOMP(argc, argv); \
}
#define DOMP_REGISTER(var, type) ( \
  domp->Register(#var, var, type); \
)
#define DOMP_PARALLELIZE(var, offset, size) { \
  domp->Parallelize(var, offset, size); \
}
#define DOMP_SHARED(var, offset, size) { \
  domp->Shared(var, offset, size); \
}
#define DOMP_SYNC { \
  domp->Synchronize(); \
};
#define DOMP_REDUCE(var, type, op) { \
  domp->Reduce(#var, (void*)&var, type, op); \
}
#define DOMP_FINALIZE() { \
  delete(domp);\
}
}


class domp::DOMP{
  int rank;
  int clusterSize;
  std::map<std::string, Variable> varList;
 public:
  DOMP(int * argc, char ***argv);
  ~DOMP();
  void Register(std::string varName, void* varValue, DOMP_TYPE type);
  void Parallelize(int totalSize, int *offset, int *size);
  void FirstShared(std::string varName, int offset, int size);
  void Shared(std::string varName, int offset, int size);
  int Reduce(std::string varName, void *address, DOMP_TYPE type, DOMP_REDUCE_OP op);
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

//typedef struct domp_interval {
//  int start;
//  int size;
//  int nodeId;
//} domp_interval_t;
//
//typedef struct domp_var {
//  void *ptr;
//  List intervals;
//} domp_var_t;
//
//typedef struct domp_struct{
//  int rank;
//  int clusterSize;
//  List var_list;
//} domp_t;



void domp_error() {

}

//bool DOMP_init(domp_t *dompObject, int *argc, char ***argv);
//
//void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size);
//
//void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);
//
//void DOMP_shared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);
//
//
//void DOMP_reduce(domp_t *dompObject, int tag, int *sum, int value);

#endif //DOMP_DOMP_H
