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

  extern DOMP *dompObject;

  #define DOMP_INIT(argc, argv) { \
    dompObject = new DOMP(argc, argv); \
  }
  #define DOMP_REGISTER(var, type) { \
    dompObject->Register(#var, var, type); \
  }
  #define DOMP_PARALLELIZE(var, offset, size) { \
    dompObject->Parallelize(var, offset, size); \
  }
  #define DOMP_SHARED(var, offset, size) { \
    dompObject->Shared(#var, offset, size); \
  }
  #define DOMP_SYNC { dompObject->Synchronize(); }

  #define DOMP_REDUCE(var, type, op) { \
    dompObject->Reduce(#var, (void*)&(var), type, op); \
  }
  #define DOMP_FINALIZE() { \
    delete(dompObject); \
    dompObject = NULL; \
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

#endif //DOMP_DOMP_H
