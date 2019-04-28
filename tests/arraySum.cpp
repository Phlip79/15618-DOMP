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
  std::cout<<DOMP_NODE_ID<<" starting function"<<std::endl;
  int *arr = new int[total_size];
  int i, offset, size;

  DOMP_REGISTER(arr, MPI_INT, total_size);
  DOMP_PARALLELIZE(total_size, &offset, &size);

  DOMP_EXCLUSIVE(arr, offset, size);
  DOMP_SYNC;
  int sum = 0;

  std::cout<<DOMP_NODE_ID<<" starting computation"<<std::endl;
  #pragma omp parallel
  {
    // Basic initialization of array
    #pragma omp for
    for (i = offset; i < (offset + size); i++)
        arr[i] = i;
    #pragma omp for reduction(+ : sum)
      // Array sum
      for(i=offset; i < (offset + size); i++) {
        sum += arr[i];
      }
   }
  std::cout<<DOMP_NODE_ID<<" Calling reduce now"<<std::endl;
  sum = DOMP_REDUCE(sum, MPI_INT, MPI_SUM);
  return sum;
}

int main(int argc, char **argv) {
  DOMP_INIT(&argc, &argv);
  int sum = compute(200);

  if(DOMP_IS_MASTER) {
    std::cout << "Hello the total sum is " << sum <<std::endl;
  } else {
    std::cout<<DOMP_NODE_ID<<" returned"<<std::endl;
  }
  DOMP_FINALIZE();
  return 0;
}

