//
// Created by Apoorv Gupta on 4/20/19.
//
#include <iostream>

#include "../lib/domp.h"
#include <omp.h>

using namespace domp;
using namespace std;

void compute(int total_size) {
  int ITERATIONS = 1;

  std::cout<<DOMP_NODE_ID<<" starting function"<<std::endl;
  int *arr = new int[total_size];
  int i, offset, size;

  DOMP_REGISTER(arr, MPI_INT, total_size);
  DOMP_PARALLELIZE(total_size, &offset, &size);

  int nextOffset = ((offset + size) >= total_size)?0:offset+size;

  DOMP_EXCLUSIVE(arr, offset, size);
  DOMP_SYNC;

  // Initialize your own array
  for (i = offset; i < (offset + size); i++)
    arr[i] = i;


  for (int it = 0; it < ITERATIONS; it++) {
    int sum = 0;
    DOMP_EXCLUSIVE(arr, offset, size);
    // Fetch the data from neighbour
    DOMP_SHARED(arr, nextOffset, size);
    DOMP_SYNC;
    std::cout << DOMP_NODE_ID << " starting computation" << std::endl;
#pragma omp parallel
    {
#pragma omp for reduction(+ : sum)
      for (i = offset; i < (offset + size); i++) {
        // Array sum
        // Add neighbours value to own value
        int nei = (i + size)%total_size;
        arr[i] += arr[nei];
        sum += arr[i];
      }
    }
    std::cout << DOMP_NODE_ID << " Calling reduce now" << std::endl;
    sum = DOMP_REDUCE(sum, MPI_INT, MPI_SUM);
    if (DOMP_IS_MASTER) {
      std::cout <<"Sum for iterations "<<it<<" is "<<sum<<std::endl;
    }
  }

  for(i = 0; i < total_size; i++) {
    printf("Node:%d Index[%d] value[%d] Address[%p]\n", DOMP_NODE_ID, i, arr[i], &arr[i]);
  }

}

int main(int argc, char **argv) {
  DOMP_INIT(&argc, &argv);
  int totalSize = 4;
  if (totalSize % DOMP_CLUSTER_SIZE != 0) {
    std::cout<<"Please run this program with 1000 as modulo of clusterSize"<<std::endl;
  } else {
    compute(totalSize);
  }
  DOMP_FINALIZE();
  return 0;
}

