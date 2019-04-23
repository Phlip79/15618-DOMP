//
// Created by Apoorv Gupta on 4/20/19.
//
#include <iostream>

#include "../lib/domp.h"
#include <mpi.h>
#include <omp.h>

using namespace domp;
using namespace std;

int compute(int total_size) {
  int *arr = new int[total_size];
  int i, offset, size;

  DOMP_REGISTER(arr, DOMP_INT);
  DOMP_PARALLELIZE(total_size, &offset, &size);

  DOMP_EXCLUSIVE(arr, offset, size);
  DOMP_SYNC;
  int sum = 0;

  #pragma omp parallel
  {

    if (DOMP_IS_MASTER) {
      // Basic initialization of array
      #pragma omp for
        for (i = offset; i < (offset + size); i++)
          arr[i] = i;
    }
    #pragma omp for reduction(+ : sum)
      // Array sum
      for(i=offset; i < (offset + size); i++) {
        sum += arr[i];
      }
    }
    DOMP_REDUCE(sum, DOMP_INT, DOMP_ADD);

  return sum;
}

int main(int argc, char **argv) {
  DOMP_INIT(&argc, &argv);
  int sum = compute(200);

  std::cout<<"Hello the total sum is "<<sum<<std::endl;

  DOMP_FINALIZE()
  return 0;
}

