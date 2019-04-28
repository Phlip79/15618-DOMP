//
// Created by Apoorv Gupta on 4/20/19.
//

#include <mpi.h>
#include <stdlib.h>
#include "domp.h"
#include "DataManager.h"

//void debug_printf(char )

namespace domp {

void log(const char *fmt, ...) {
#if DEBUG
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  bool got_newline = fmt[strlen(fmt) - 1] == '\n';
  if (!got_newline)
    fprintf(stderr, "\n");
#endif
}

DOMP::DOMP(int *argc, char ***argv) {
  int provided;
  MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
  clusterSize = 1;
  rank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &clusterSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  log("My rank=%d, size=%d, provided support=%d\n", rank, clusterSize, provided);

  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    dataManager = new MasterDataManager(dompObject, clusterSize, rank);
  } else {
    dataManager = new DataManager(dompObject, clusterSize, rank);
  }
}

DOMP::~DOMP() {
  log("Node %d destructor called", rank);
  delete(dataManager);

  // Free the memory for variables
  for (std::map<std::string,Variable*>::iterator it=varList.begin(); it!=varList.end(); ++it)
    delete(it->second);

  MPI_Finalize();
}

DOMP *dompObject;

void DOMP::Parallelize(int totalSize, int *offset, int *size) {
  int perNode = totalSize / clusterSize;
  int startOffset = perNode * rank;
  int extraWork = totalSize % clusterSize;

  // Assign extra work to first few nodes
  if (rank < extraWork) {
    startOffset += rank;
    perNode += 1;
  } else {
    // Adjust the offset
    startOffset += extraWork;
  }
  *offset = startOffset;
  *size = perNode;

  log("Node %d::Parallelize returned with Offset[%d], Size[%d], TotalSize[%d]", rank, *offset, *size, totalSize);

}

void DOMP::FirstShared(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_SHARED_FIRST);
}

void DOMP::Register(std::string varName, void *varValue, MPI_Datatype type, int size) {
  if (varList.count(varName) != 0) {
    delete(varList[varName]);
  }
  varList[varName] =  new Variable((char*)varValue, type, size);
  if (IsMaster()) {
    dataManager->registerVariable(varName, varList[varName]);
  }
}

void DOMP::Shared(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_EXCLUSIVE_FETCH);
}

void DOMP::Exclusive(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_EXCLUSIVE_FIRST);
}

int DOMP::Reduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op) {
  int val;
  log("Node %d calling reduce on %s",rank, varName.c_str());
  MPI_Reduce(address, &val, 1, type, op, 0, MPI_COMM_WORLD);
  log("Node %d returned reduce on %s",rank, varName.c_str());
  return val;
}

void DOMP::Synchronize() {
  log("Node %d calling sync",rank);
  dataManager->triggerMap();
  log("Node %d returned sync",rank);
}

bool DOMP::IsMaster() {
  if (rank == 0) return true;
  else return false;
}

int DOMP::GetRank() {
  return rank;
}

std::pair<char*, int> DOMP::mapDataRequest(char* varName, int start, int size) {
  // TODO bookkeeping
  if (varList.count(varName) == 0) {
    log("Node %s:: Variable %s not found", rank, varName);
    MPI_Abort(MPI_COMM_WORLD, DOMP_VAR_NOT_FOUND_ON_NODE);
  }

  Variable *var = varList[std::string(varName)];
  int varSize = sizeof(int);
  auto type = var->getType();
  if (type == MPI_BYTE)  varSize = 1;
  else if (type == MPI_FLOAT)  varSize = sizeof(float);
  else if (type == MPI_DOUBLE)  varSize = sizeof(double);
  else varSize = sizeof(char);

  int offset = start * varSize;
  char *address = var->getPtr();
  return std::make_pair(address, offset);
}

}
