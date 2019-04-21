//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_DOMP_H
#define DOMP_DOMP_H

#include <stdbool.h>
#include "util/linkedlist.h"

typedef struct domp_interval {
  int start;
  int location;
  int nodeId;
} domp_interval_t;

typedef struct domp_var {
  void *ptr;
  List intervals;
} domp_var_t;

typedef struct domp_struct{
  int rank;
  int clusterSize;
  List var_list;
} domp_t;



void domp_error() {

}

enum DOMP_TYPE{DOMP_INT, DOMP_FLOAT};

bool DOMP_init(domp_t *dompObject, int *argc, char ***argv);

void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size);

void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);

void DOMP_shared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);


void DOMP_reduce(domp_t *dompObject, int tag, int *sum, int value);

#endif //DOMP_DOMP_H
