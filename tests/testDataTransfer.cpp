//
// Created by Apoorv Gupta on 4/20/19.
//
#include <iostream>

#include "../lib/domp.h"
#include <omp.h>

using namespace domp;
using namespace std;

void compute(int total_size, int iterations) {
  int *arr = new int[total_size];
  int i, offset, size;

  DOMP_REGISTER(arr, MPI_INT, total_size);
  DOMP_PARALLELIZE(total_size, &offset, &size);

  // Initialize your own array
  for (i = offset; i < (offset + size); i++)
    arr[i] = i;

  DOMP_EXCLUSIVE(arr, offset, size);
  DOMP_SYNC;

  int nextOffset = ((offset + size) >= total_size)?0:offset+size;
  for(int it = 0; it < iterations; it++) {
    int sum = 0;
    DOMP_EXCLUSIVE(arr, offset, size);
    // Fetch the data from neighbour
    DOMP_SHARED(arr, nextOffset, size);
    DOMP_SYNC;
    #pragma omp parallel
    {
      #pragma omp for reduction(+ : sum)
      for (i = offset; i < (offset + size); i++) {
        // Add neighbours value to own value
        int nei = (i + size) % total_size;
        arr[i] += arr[nei];
        sum += arr[i];
      }
    }
    DOMP_REDUCE(sum, MPI_INT, MPI_SUM);
    if (DOMP_IS_MASTER) {
      std::cout << "Iteration:"<<it<<", Sum is " << sum << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  DOMP_INIT(&argc, &argv);
  int totalSize = 8648640;
  if (totalSize % DOMP_CLUSTER_SIZE != 0) {
    std::cout<<"Please run this program with 1000 as modulo of clusterSize"<<std::endl;
  } else {
    compute(totalSize, 25);
  }
  DOMP_FINALIZE();
  return 0;
}

