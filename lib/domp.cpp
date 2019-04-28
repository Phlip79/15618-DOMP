//
// Created by Apoorv Gupta on 4/20/19.
//

#include <mpi.h>
#include <stdlib.h>
#include "domp.h"
#include "MPIServer.h"

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
    mpiServer = new MPIMasterServer(dompObject, clusterSize, rank);
  } else {
    mpiServer = new MPIServer(dompObject, clusterSize, rank);
  }
}

DOMP::~DOMP() {
  log("Node %d destructor called", rank);
  delete(mpiServer);

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
}

void DOMP::FirstShared(std::string varName, int offset, int size) {

}

void DOMP::Register(std::string varName, void *varValue, MPI_Datatype type) {

}

void DOMP::Shared(std::string varName, int offset, int size) {

}

void DOMP::Exclusive(std::string varName, int offset, int size) {

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
  MPI_Barrier(MPI_COMM_WORLD);
  log("Node %d returned sync",rank);

}

bool DOMP::IsMaster() {
  if (rank == 0) return true;
  else return false;
}

int DOMP::GetRank() {
  return rank;
}

std::pair<void *, int> DOMP::mapDataRequest(std::string varName, int start, int size) {
  // TODO bookkeeping
  int a;
  return std::make_pair((void *) &a, 0);
}

}
