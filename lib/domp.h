//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_DOMP_H
#define DOMP_DOMP_H

#include <iostream>
#include <set>
#include <list>
#include <map>

#include <stdbool.h>
#include "util/DoublyLinkedList.h"

namespace domp {
  #define DOMP_MAX_VAR_NAME (50)
  #define DOMP_MAX_CLUSTER_NAME (10)
  #define DOMP_MAX_CLIENT_NAME (DOMP_MAX_CLUSTER_NAME + 10)

  enum DOMP_TYPE {DOMP_INT, DOMP_FLOAT};
  enum DOMP_REDUCE_OP {DOMP_ADD, DOMP_SUBTRACT};

  class DOMP;
  class Variable;
  class Fragment;
  class SplitList;

  template <typename T>
  class DoublyLinkedList;
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

  #define DOMP_EXCLUSIVE(var, offset, size) { \
    dompObject->Exclusive(#var, offset, size); \
  }

  #define DOMP_SYNC { dompObject->Synchronize(); }

  #define DOMP_REDUCE(var, type, op) { \
    dompObject->Reduce(#var, (void*)&(var), type, op); \
  }
  #define DOMP_FINALIZE() { \
    delete(dompObject); \
    dompObject = NULL; \
  }

  #define DOMP_IS_MASTER (dompObject->IsMaster())
}

class domp::DOMP{
  int rank;
  int clusterSize;
  char clusterName[DOMP_MAX_CLUSTER_NAME];
  std::map<std::string, Variable> varList;
 public:
  DOMP(int * argc, char ***argv);
  ~DOMP();
  void Register(std::string varName, void* varValue, DOMP_TYPE type);
  void Parallelize(int totalSize, int *offset, int *size);
  void FirstShared(std::string varName, int offset, int size);
  void Shared(std::string varName, int offset, int size);
  void Exclusive(std::string varName, int offset, int size);
  int Reduce(std::string varName, void *address, DOMP_TYPE type, DOMP_REDUCE_OP op);
  void Synchronize();
  bool IsMaster();

  // These functions are used by MPIServer
  std::pair<void *, int> mapDataRequest(std::string varName, int start, int size);


};

class domp::Fragment {
  int start;
  int size;
  int end;
  std::set <int> nodes;
  friend SplitList;
  friend DoublyLinkedList<Fragment>;
  Fragment *next;
  Fragment *prev;
 public:
  Fragment(int start, int size, int nodeId) {
    this->start = start;
    this->size = size;
    this->end = start + size - 1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(nodeId);
  }

  Fragment(Fragment *from) {
    this->start = from->start;
    this->size = from->size;
    this->end = start + size -  1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(from->nodes.begin(), from->nodes.end());
  }

  void addNode(int nodeId) {
    this->nodes.insert(nodeId);
  }

  void update(int start, int end) {
    this->start = start;
    this->end = end;
    this->size = end - start + 1;
  }
};

class domp::Variable {
  void *ptr;
  DoublyLinkedList<Fragment> fragments;
 public:
  Variable(void * ptr) {
    this->ptr = ptr;
  }
};

#endif //DOMP_DOMP_H
