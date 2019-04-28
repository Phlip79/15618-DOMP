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

void gen_random(char *s, const int len) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

DOMP::DOMP(int *argc, char ***argv) {
  int provided;
  MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
  clusterSize = 1;
  rank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &clusterSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Generate a random clusterName and broadcast it to all nodes
  if (rank == 0) {
    gen_random(clusterName, DOMP_MAX_CLUSTER_NAME - 1);
  }
  MPI_Bcast(clusterName, DOMP_MAX_CLUSTER_NAME, MPI_BYTE, 0, MPI_COMM_WORLD);

  log("My rank=%d, size=%d, ClusterName=%s, provided support=%d\n", rank, clusterSize, clusterName, provided);

  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    mpiServer = new MPIMasterServer(dompObject, clusterName, clusterSize, rank);
  } else {
    mpiServer = new MPIServer(dompObject, clusterName, clusterSize, rank);
  }
  mpiServer->startServer();
}

DOMP::~DOMP() {
  log("Node %d destructor called", rank);
  mpiServer->stopServer();
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
