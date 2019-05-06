//
// Created by Apoorv Gupta on 4/20/19.
//

#include <mpi.h>
#include <stdlib.h>
#include "domp.h"
#include "DataManager.h"
#include "util/CycleTimer.h"

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

  dataBuffer = NULL;

  if(void *buffer = std::realloc(dataBuffer, DOMP_BUFFER_INIT_SIZE)) {
    dataBuffer = buffer;
    log("Buffer returned by realloc is %p", buffer);
    currentBufferSize = DOMP_BUFFER_INIT_SIZE;
  } else {
    currentBufferSize =  0;
    throw std::bad_alloc();
  }

  log("My rank=%d, size=%d, provided support=%d\n", rank, clusterSize, provided);

  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    dataManager = new MasterDataManager(this, clusterSize, rank);
  } else {
    dataManager = new DataManager(this, clusterSize, rank);
  }
}

DOMP::~DOMP() {
  log("Node %d destructor called", rank);
  delete(dataManager);

  free(dataBuffer);
  currentBufferSize = 0;
  // Free the memory for variables
  for (std::map<std::string,Variable*>::iterator it=varList.begin(); it!=varList.end(); ++it)
    delete(it->second);
  PrintProfilingData();
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

void DOMP::Register(std::string varName, void *varValue, MPI_Datatype type, int size) {
  if (varList.count(varName) != 0) {
    delete(varList[varName]);
  }
  varList[varName] =  new Variable((char*)varValue, type, size);
  log("Node %d registered Var[%s] Address[%p]", rank, varName.c_str(), varValue);
  if (IsMaster()) {
    dataManager->registerVariable(varName, varList[varName]);
  }
}

void DOMP::FirstShared(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_SHARED_FIRST);
}

void DOMP::Shared(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_EXCLUSIVE_FETCH);
}

void DOMP::Exclusive(std::string varName, int offset, int size) {
  dataManager->requestData(varName, offset, size, MPI_EXCLUSIVE_FIRST);
}

void DOMP::Reduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op) {
  ArrayReduce(varName, address, type, op, 0, 1, REDUCE_ON_MASTER);
}


void DOMP::ArrayReduce(std::string varName, void *address, MPI_Datatype type, MPI_Op op, int offset, int size,
                      DOMP_REDUCE_TYPE reduceType) {
  log("Node %d::Called ArrayReduce with address %p", rank, address);
#if PROFILING
  double start = currentSeconds();
#endif
  int varSize = getSizeBytes(type);
  int totalSize =  varSize * size;
  if (totalSize > currentBufferSize) {
    log("Node %d::Called realloc with pointer %p and required size:%d", rank, dataBuffer, totalSize);
    if(void *buffer = std::realloc(dataBuffer, totalSize)) {
      log("Node %d::After realloc with pointer %p", rank, buffer);
      dataBuffer = buffer;
      currentBufferSize = totalSize;
    } else {
      log("Node %d::realloc failed for size %d", rank, totalSize);
      throw std::bad_alloc();
    }
  }
  void *dataPtr = (char*)address + (offset * varSize);
  log("Node %d calling ArrayReduce on %s and address %p, pointer %p, size=%d",rank, varName.c_str(), dataPtr,
      dataBuffer, size);
  if (reduceType == REDUCE_ON_MASTER) {
    MPI_Reduce(dataPtr, dataBuffer, size, type, op, 0, MPI_COMM_WORLD);
  } else {
    MPI_Allreduce(dataPtr, dataBuffer, size, type, op, MPI_COMM_WORLD);
  }
  memcpy(dataPtr, dataBuffer, totalSize);
#if PROFILING
  profiler.reduceTime += currentSeconds() - start;
#endif
  log("Node %d returned ArrayReduce on %s",rank, varName.c_str());
}

void DOMP::Synchronize() {
  log("Node %d calling sync",rank);
#if PROFILING
  double start = currentSeconds();
#endif
  dataManager->triggerMap();
#if PROFILING
  profiler.syncTime += currentSeconds() - start;
#endif
  log("Node %d returned sync",rank);
}

bool DOMP::IsMaster() {
  if (rank == 0) return true;
  else return false;
}

int DOMP::GetRank() {
  return rank;
}

int DOMP::GetClusterSize() {
  return clusterSize;
}

std::pair<char*, int> DOMP::mapDataRequest(char* varName, int start, int size) {
  std::string key(varName);
  if (varList.count(key) == 0) {
    log("Node %d:: Variable %s not found", rank, varName);
    MPI_Abort(MPI_COMM_WORLD, DOMP_VAR_NOT_FOUND_ON_NODE);
  }

  Variable *var = varList[key];
  int varSize = sizeof(int);
  auto type = var->getType();
  varSize = getSizeBytes(type);

  int offset = start * varSize;
  char *address = var->getPtr() + offset;
  return std::make_pair(address, varSize * size);
}
int DOMP::getSizeBytes(const MPI_Datatype &type) const {
  int varSize;
  if (type == MPI_BYTE) varSize = 1;
  else if (type == MPI_FLOAT)  varSize = sizeof(float);
  else if (type == MPI_DOUBLE)  varSize = sizeof(double);
  else if (type == MPI_INT)  varSize = sizeof(int);
  else varSize = sizeof(char);
  return varSize;
}

void DOMP::PrintProfilingData() {
#if PROFILING
  double currentTime = currentSeconds();
  double libTime = this->profiler.reduceTime + this->profiler.syncTime;
  double totalTime = currentTime - this->profiler.programStart;
  if(rank == 0) {
    double currentTime = currentSeconds();
    printf("DOMP Master Sync time = %10.4f sec\n", this->profiler.syncTime);
    printf("DOMP Master Reduce time = %10.4f sec\n", this->profiler.reduceTime);
    printf("DOMP Master Library time = %10.4f sec\n", libTime);
    double totalTime = currentTime - this->profiler.programStart;
    printf("DOMP Master Total time = %10.4f sec\n", totalTime);
  }
  double clusterLibTime, slaveLibTime;
  double clusterTotalTime, slaveTotalTime;
  MPI_Reduce(&libTime, &clusterLibTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&totalTime, &clusterTotalTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    printf("DOMP Cluster Library time = %10.4f sec\n", clusterLibTime/clusterSize);
    printf("DOMP Cluster Total time = %10.4f sec\n", clusterTotalTime/clusterSize);
    if (clusterSize > 1) {
      slaveLibTime = (clusterLibTime - libTime)/(clusterSize - 1);
      slaveTotalTime = (clusterTotalTime - totalTime)/(clusterSize - 1);
    } else {
      slaveLibTime = 0;
      slaveTotalTime = 0;
    }
    printf("DOMP Slave Library time = %10.4f sec\n", slaveLibTime);
    printf("DOMP Slave Total time = %10.4f sec\n", slaveTotalTime);
  }
#endif
}

void DOMP::InitProfiler() {
  this->profiler.programStart = currentSeconds();
}

}
