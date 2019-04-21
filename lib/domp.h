//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_DOMP_H
#define DOMP_DOMP_H


typedef struct domp_struct{
  int nodeId;
  int cluserSize;

} domp_t;


enum DOMP_TYPE{DOMP_INT, DOMP_FLOAT};

void DOMP_init(domp_t *dompObject, char **argc, int argv);

void DOMP_parallelize(domp_t *dompObject, int totalSize, int *offset, int *size);

void DOMP_firstShared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);

void DOMP_shared(domp_t *dompObject, enum DOMP_TYPE type, void *location, int offset, int size);


void DOMP_reduce(domp_t *dompObject, int tag, int *sum, int value);

#endif //DOMP_DOMP_H
