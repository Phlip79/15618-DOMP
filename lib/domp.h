//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_DOMP_H
#define DOMP_DOMP_H

#include <iostream>
#include <set>
#include <list>
#include <map>

#include <mpi.h>
#include <stdbool.h>

namespace domp {
  #define DOMP_MAX_VAR_NAME (50)
  #define DOMP_MAX_CLUSTER_NAME (10)
  #define DOMP_MAX_CLIENT_NAME (DOMP_MAX_CLUSTER_NAME + 10)

  enum DOMP_TYPE {DOMP_INT, DOMP_FLOAT};
  enum DOMP_REDUCE_OP {DOMP_ADD, DOMP_SUBTRACT};

  class DOMP;
  class DataManager;
  class Variable;

void log(const char *fmt, ...);
  extern DOMP *dompObject;

  #define DOMP_INIT(argc, argv) { \
    dompObject = new DOMP(argc, argv); \
  }

  #define DOMP_NODE_ID (dompObject->GetRank())

  #define DOMP_REGISTER(var, type, size) { \
    dompObject->Register(#var, var, type, size); \
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

  #define DOMP_REDUCE(var, type, op) (dompObject->Reduce(#var, (void*)&(var), type, op))

  #define DOMP_FINALIZE() { \
    delete(dompObject); \
    dompObject = NULL; \
  }

  #define DOMP_IS_MASTER (dompObject->IsMaster())
}

class domp::DOMP{
  int rank;
  int clusterSize;
  std::map<std::string, Variable*> varList;
  DataManager *dataManager;
 public:
  DOMP(int * argc, char ***argv);
  ~DOMP();
  void Register(std::string varName, void* varValue, MPI_Datatype type, int size);
  void Parallelize(int totalSize, int *offset, int *size);
  void FirstShared(std::string varName, int offset, int size);
  void Shared(std::string varName, int offset, int size);
  void Exclusive(std::string varName, int offset, int size);
  int Reduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op);
  void Synchronize();
  bool IsMaster();
  int GetRank();

  // These functions are used by DataManager
  std::pair<void *, int> mapDataRequest(std::string varName, int start, int size);


};

class domp::Variable {
  void *ptr;
  MPI_Datatype type;
  int size;
 public:
  Variable(void * ptr, MPI_Datatype type, int size) {
    this->ptr = ptr;
    this->type = type;
    this->size = size;
  }
  void *getPtr() const {
    return ptr;
  }
  const MPI_Datatype &getType() const {
    return type;
  }
  int getSize() const {
    return size;
  }
};


#endif //DOMP_DOMP_H
