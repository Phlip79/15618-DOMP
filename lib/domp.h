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
#include "util/CycleTimer.h"

namespace domp {
  #define DOMP_MAX_VAR_NAME (50)
  #define DOMP_MAX_CLUSTER_NAME (10)
  #define DOMP_BUFFER_INIT_SIZE (256)
  #define DOMP_MAX_CLIENT_NAME (DOMP_MAX_CLUSTER_NAME + 10)

  enum DOMP_TYPE {DOMP_INT, DOMP_FLOAT};
  enum DOMP_REDUCE_OP {DOMP_ADD, DOMP_SUBTRACT};
  enum DOMP_REDUCE_TYPE {REDUCE_ON_MASTER, REDUCE_ALL};

  enum DOMP_ERROR_MSG {
    DOMP_VAR_NOT_FOUND_ON_NODE,
    DOMP_VAR_NOT_FOUND_ON_MASTER
  };

  class DOMP;
  class DataManager;
  class Variable;
  class Profiler;

void log(const char *fmt, ...);
  extern DOMP *dompObject;

  #define DOMP_INIT(argc, argv) { \
    dompObject = new DOMP(argc, argv); \
  }

  #define DOMP_NODE_ID (dompObject->GetRank())
  #define DOMP_CLUSTER_SIZE (dompObject->GetClusterSize())

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

  #define DOMP_ARRAY_REDUCE(var, type, op, offset, size) { \
      dompObject->ArrayReduce(#var, var, type, op, offset, size, REDUCE_ON_MASTER); \
  }

  #define DOMP_ARRAY_REDUCE_ALL(var, type, op, offset, size) { \
        dompObject->ArrayReduce(#var, var, type, op, offset, size, REDUCE_ALL); \
  }

  #define DOMP_FINALIZE() { \
    delete(dompObject); \
    dompObject = NULL; \
  }

  #define DOMP_IS_MASTER (dompObject->IsMaster())
}

class domp::Profiler {
 public:
  double syncTime;
  double reduceTime;
  double programStart;
  Profiler() {
    syncTime = 0;
    reduceTime = 0;
    programStart = currentSeconds();
  }
};

class domp::DOMP{
  int rank;
  int clusterSize;
  std::map<std::string, Variable*> varList;
  DataManager *dataManager;
  void *dataBuffer;
  int currentBufferSize;
#if PROFILING
  Profiler profiler;
#endif
  int getSizeBytes(const MPI_Datatype &type) const;
 public:
  DOMP(int * argc, char ***argv);
  ~DOMP();
  void Register(std::string varName, void* varValue, MPI_Datatype type, int size);
  void Parallelize(int totalSize, int *offset, int *size);
  void FirstShared(std::string varName, int offset, int size);
  void Shared(std::string varName, int offset, int size);
  void Exclusive(std::string varName, int offset, int size);
  void Reduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op);
  void Synchronize();
  bool IsMaster();
  int GetRank();
  int GetClusterSize();

  // These functions are used by DataManager
  std::pair<char *, int> mapDataRequest(char* varName, int start, int size);
  // For reduction
  void ArrayReduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op, int offset, int size,
    DOMP_REDUCE_TYPE reduceType);

  void PrintProfilingData();

};

class domp::Variable {
  char *ptr;
  MPI_Datatype type;
  int size;
 public:
  Variable(char * ptr, MPI_Datatype type, int size) {
    this->ptr = ptr;
    this->type = type;
    this->size = size;
  }
  char *getPtr() const {
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
