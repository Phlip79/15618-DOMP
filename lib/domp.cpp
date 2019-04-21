//
// Created by Apoorv Gupta on 4/20/19.
//

#include <mpi.h>
#include <stdlib.h>
#include "domp.h"

//void debug_printf(char )

namespace domp {
DOMP::DOMP(int *argc, char ***argv) {
  MPI_Init(argc, argv);
  MPI_Comm_size(MPI_COMM_WORLD, &clusterSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

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

void DOMP::FirstShared(void *ptr, int offset, int size) {
}

}

//bool DOMP_init(domp_t *dompObject, int *argc, char ***argv) {
//  if (dompObject == NULL) {
//
//  }
//  initList(&dompObject->var_list);
//
//  MPI_Init(argc, argv);
//  MPI_Comm_size(MPI_COMM_WORLD, &dompObject->clusterSize);
//  MPI_Comm_rank(MPI_COMM_WORLD, &dompObject->rank);
//  if (dompObject->rank == 0) {
//    return true;
//  } else {
//    return false;
//  }
//}
//
//void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size) {
//  if (dompObject == NULL) {
//
//  }
//  int perNode = totalSize / dompObject->clusterSize;
//  int startOffset = perNode * dompObject->rank;
//  int extraWork = totalSize % dompObject->clusterSize;
//
//  // Assign extra work to first few nodes
//  if (dompObject->rank < extraWork) {
//    startOffset += dompObject->rank;
//    perNode += 1;
//  } else {
//    // Adjust the offset
//    startOffset += extraWork;
//  }
//
//  *offset = startOffset;
//  *size = perNode;
//}
//
//void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size) {
//  if (dompObject == NULL || location == NULL) {
//
//  }
//
//  // Search for the mapping
//  ListNode *varNode = getDataNode(&dompObject->var_list, location, &cmp);
//  if (varNode == NULL) {
//    // Not found
//    varNode = malloc(sizeof(ListNode));
//    if (varNode != NULL) {
//      domp_var_t *var = malloc(sizeof(domp_var_t));
//      if (var != NULL) {
//        varNode->data = var;
//        initList(&var->intervals);
//        ListNode *intervalNode = malloc(sizeof(ListNode));
//        if (intervalNode != NULL) {
//          domp_interval_t *interval = malloc(sizeof(domp_interval_t));
//          if (interval != NULL) {
//            intervalNode->data = interval;
//            interval->nodeId = dompObject->rank;
//            interval->start = offset;
//            interval->size = size;
//            varNode->data = var;
//            // Insert nodes into linkedlist
//            insertNodeFront(&var->intervals, intervalNode);
//            insertNodeFront(&dompObject->var_list, varNode);
//          }
//        }
//      }
//    }
//  } else {
//    // Update the mapping
//
//  }
//}
//
//static int cmp(void *a, void *b) {
//  return ((void *) a == ((domp_var_t *) b)->ptr) ? 1 : 0;
//}
//}
