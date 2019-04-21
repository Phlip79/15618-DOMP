//
// Created by Apoorv Gupta on 4/20/19.
//


#include "domp.h"
#include <mpi.h>

//void debug_printf(char )

bool DOMP_init(domp_t *dompObject, int *argc, char ***argv) {
  if (dompObject == NULL) {

  }
  MPI_Init(argc, argv);
  MPI_Comm_size(MPI_COMM_WORLD, &dompObject->clusterSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &dompObject->rank);
  if (dompObject->rank == 0) {
    return  true;
  } else {
    return  false;
  }
}





