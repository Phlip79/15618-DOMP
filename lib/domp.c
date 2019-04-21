//
// Created by Apoorv Gupta on 4/20/19.
//


#include "domp.h"
#include <mpi.h>

//void debug_printf(char )

bool DOMP_init(domp_t *dompObject, int *argc, char ***argv) {
  if (dompObject == NULL) {

  }
  initList(&dompObject->var_list);

  MPI_Init(argc, argv);
  MPI_Comm_size(MPI_COMM_WORLD, &dompObject->clusterSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &dompObject->rank);
  if (dompObject->rank == 0) {
    return  true;
  } else {
    return  false;
  }
}



void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size) {
  if (dompObject == NULL) {

  }
  int perNode = totalSize / dompObject->clusterSize;
  int startOffset = perNode * dompObject->rank;
  int extraWork = totalSize % dompObject->clusterSize;

  // Assign extra work to first few nodes
  if (dompObject->rank < extraWork) {
    startOffset += dompObject->rank;
    perNode += 1;
  } else {
    // Adjust the offset
    startOffset += extraWork;
  }

  *offset = startOffset;
  *size = perNode;
}


void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size) {

}
