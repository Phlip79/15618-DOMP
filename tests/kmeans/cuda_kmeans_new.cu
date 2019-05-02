/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         cuda_kmeans.cu  (CUDA version)                            */
/*   Description:  Implementation of simple k-means clustering algorithm     */
/*                 This program takes an array of N data objects, each with  */
/*                 M coordinates and performs a k-means clustering given a   */
/*                 user-provided value of the number of clusters (K). The    */
/*                 clustering results are saved in 2 arrays:                 */
/*                 1. a returned array of size [K][N] indicating the center  */
/*                    coordinates of K clusters                              */
/*                 2. membership[N] stores the cluster center ids, each      */
/*                    corresponding to the cluster a data object is assigned */
/*                                                                           */
/*   Author:  Wei-keng Liao                                                  */
/*            ECE Department, Northwestern University                        */
/*            email: wkliao@ece.northwestern.edu                             */
/*   Copyright, 2005, Wei-keng Liao                                          */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Copyright (c) 2005 Wei-keng Liao
// Copyright (c) 2011 Serban Giuroiu
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "kmeans.h"
//#define PROFILING 0
#define MAX_THREADS (1024)

static inline int nextPowerOfTwo(int n) {
  n--;

  n = n >>  1 | n;
  n = n >>  2 | n;
  n = n >>  4 | n;
  n = n >>  8 | n;
  n = n >> 16 | n;
//  n = n >> 32 | n;    //  For 64-bit ints

  return ++n;
}

/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__host__ __device__ inline static
float euclid_dist_2(int    numCoords,
                    int    numObjs,
                    int    numClusters,
                    float *objects,     // [numCoords][numObjs]
                    float *clusters,    // [numCoords][numClusters]
                    int    objectId,
                    int    clusterId)
{
  int i;
  float ans=0.0;

  for (i = 0; i < numCoords; i++) {
    ans += (objects[numObjs * i + objectId] - clusters[numClusters * i + clusterId]) *
        (objects[numObjs * i + objectId] - clusters[numClusters * i + clusterId]);
  }

  return(ans);
}

/*----< find_nearest_cluster() >---------------------------------------------*/
__global__ static
void find_nearest_cluster(int numCoords,
                          int numObjs,
                          int numClusters,
                          float *objects,           //  [numCoords][numObjs]
                          float *deviceClusters,    //  [numCoords][numClusters]
                          int *membership,          //  [numObjs]
                          int *intermediates)
{
  extern __shared__ char sharedMemory[];

  //  The type chosen for membershipChanged must be large enough to support
  //  reductions! There are blockDim.x elements, one for each thread in the
  //  block.
  unsigned char *membershipChanged = (unsigned char *)sharedMemory;
  float *clusters = (float *)(sharedMemory + blockDim.x);

  membershipChanged[threadIdx.x] = 0;

  //  BEWARE: We can overrun our shared memory here if there are too many
  //  clusters or too many coordinates!
  for (int i = threadIdx.x; i < numClusters; i += blockDim.x) {
    for (int j = 0; j < numCoords; j++) {
      clusters[numClusters * j + i] = deviceClusters[numClusters * j + i];
    }
  }
  __syncthreads();

  int objectId = blockDim.x * blockIdx.x + threadIdx.x;

  if (objectId < numObjs) {
    int index, i;
    float dist, min_dist;

    /* find the cluster id that has min distance to object */
    index = 0;
    min_dist = euclid_dist_2(numCoords, numObjs, numClusters,
                             objects, clusters, objectId, 0);

    for (i = 1; i < numClusters; i++) {
      dist = euclid_dist_2(numCoords, numObjs, numClusters,
                           objects, clusters, objectId, i);
      /* no need square root */
      if (dist < min_dist) { /* find the min and its array index */
        min_dist = dist;
        index = i;
      }
    }

    if (membership[objectId] != index) {
      membershipChanged[threadIdx.x] = 1;
    }

    /* assign the membership to object objectId */
    membership[objectId] = index;

    __syncthreads();    //  For membershipChanged[]

    //  blockDim.x *must* be a power of two!
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
      if (threadIdx.x < s) {
        membershipChanged[threadIdx.x] +=
            membershipChanged[threadIdx.x + s];
      }
      __syncthreads();
    }
  }
  if (threadIdx.x == 0) {
    intermediates[blockIdx.x] = membershipChanged[0];
  }
}

__global__ static
void compute_delta(int *deviceIntermediates,
                   int *deviceStore,
                   int numIntermediates,    //  The actual number of intermediates
                   int numIntermediates2)   //  The next power of two
{
  //  The number of elements in this array should be equal to
  //  numIntermediates2, the number of threads launched. It *must* be a power
  //  of two!
  extern __shared__ unsigned int intermediates[];

  int location = blockIdx.x * blockDim.x + threadIdx.x;

  //  Copy global intermediate values into shared memory.
  intermediates[threadIdx.x] = (location < numIntermediates) ? deviceIntermediates[location] : 0;
  __syncthreads();

  //  numIntermediates2 *must* be a power of two!
  for (unsigned int s = numIntermediates2 / 2; s > 0; s >>= 1) {
    if (threadIdx.x < s) {
      intermediates[threadIdx.x] += intermediates[threadIdx.x + s];
    }
    __syncthreads();
  }

  if (threadIdx.x == 0) {
    deviceStore[blockIdx.x] = intermediates[0];
  }
}


/**
 * This is a CUDA kernel which creates clusters and clusterArray using the
 * membership of every object. To reduce the contetion among threads from
 * various thread-blocks each thread block computes these datastructures in
 * their local shared memory and then finally copies them back to global
 * memory. Since, each thread-block creates a datastrutucre for its own
 * thread-block, they need to be reduced later by another kernel.
 * @param objects The pointer to device memory address holding the all
 * object's coordinate values.
 * @param deviceMembership The pointer to memory address holding the
 * membership information for all objects.
 * @param deviceComputeClusters The global memory where cluster array per
 * thread-block is stored.
 * @param deviceComputeClustersSize The global memory where clusterSize array
 * per thread-block is stored.
 * @param numObjects The actual number of objects
 * @param numClusters The number of clusters
 * @param numCoords The number of coordinates
 * @return
 */
__global__ static
void compute_membership(float *objects, // [numCoords][numObjs]
                    int *deviceMembership, // [numObjs]
                   float *deviceComputeClusters, //block_size][num_coords][num_clusters]
                   int *deviceComputeClustersSize, //[block_size][num_clusters]
                   int numObjects,
                   int numClusters,
                   int numCoords) {
  extern __shared__ int shared_memory[];
  int *clusterSize = shared_memory;
  float *clusters = (float*)(shared_memory + numClusters); // size of
                                            // clusterSize array is numClusters
  int threadCount = blockDim.x;
  int objectIndex = blockIdx.x * blockDim.x + threadIdx.x;
  int clusterIndex = blockIdx.x;

  // Let's not load the 0 values from global memory. It's slow. Initialize
  // the shared memory itself.

  int start = threadIdx.x;
  int maxPoints = numCoords * numClusters;

  // Initialize cluster size collectively
  if (threadIdx.x < numClusters) {
    clusterSize[threadIdx.x] = 0;
  }


  // numCoords * numClusters could be larger than threadCount
  while (start < maxPoints) {
    clusters[start] = 0.0;
    start += threadCount;
  }

  // Wait for load to complete
  __syncthreads();

  int newCluster = deviceMembership[objectIndex];
  if (objectIndex < numObjects) {
    // Reduce contention by having threads update different counters
    if (threadIdx.x < 512) {
      atomicAdd(&clusterSize[newCluster], 1);
      if (threadIdx.x > 256) {
        for (int i = 0; i < numCoords; i++) {
          atomicAdd(&clusters[i * numClusters + newCluster],
                    objects[i * numObjects + objectIndex]);
        }
      } else {
        for (int i = numCoords - 1; i >= 0; i--) {
          atomicAdd(&clusters[i * numClusters + newCluster],
                    objects[i * numObjects + objectIndex]);
        }
      }
    } else {
      if (threadIdx.x > 768) {
        for (int i = 0; i < numCoords; i++) {
          atomicAdd(&clusters[i * numClusters + newCluster],
                    objects[i * numObjects + objectIndex]);
        }
      } else {
        for (int i = numCoords - 1; i >= 0; i--) {
          atomicAdd(&clusters[i * numClusters + newCluster],
                    objects[i * numObjects + objectIndex]);
        }
      }
      atomicAdd(&clusterSize[newCluster], 1);
    }
  }

  // Wait for processing to complete
  __syncthreads();
  // Now store the data collectively to global memory at the block's clusterIndex
  if (threadIdx.x < numClusters) {
      int start = (clusterIndex * numClusters);
      deviceComputeClustersSize[start + threadIdx.x] = clusterSize[threadIdx.x];
  }

  int startIndex = clusterIndex * numClusters * numCoords;
  start = threadIdx.x;
  while (start < maxPoints) {
    deviceComputeClusters[startIndex + start] = clusters[start];
    start += threadCount;
  }
}

/**
 * This function retrieves the index given the clusterBlock, cooridnatorIndex
 * and clusterIndex.
 * @param numClusters The total number of clusters.
 * @param numCoord The total number of coordinates.
 * @param clusterBlock It's the index of thread-block from previous kernel.
 * @param coordIndex The coorindator Index
 * @param clusterIndex The clusterIndex
 * @return Returns the index in deviceComputeClusters array based on block
 * index, coordinator index and clusterindex.
 */
__device__ static inline int getIndex(int numClusters,
                                      int numCoord,
                                      int clusterBlock,
                                      int coordIndex,
                                      int clusterIndex) {
  int index = (clusterBlock * numClusters * numCoord) + (coordIndex * numClusters) + clusterIndex;
  return index;
}

/**
 * This function loads the coordinate value stored by previous kernel from
 * global memory based on the arguments provided. Each thread is trying to
 * reduce a different value hence this function will load value from
 * different index for every thread.
 * @param deviceComputeClusters The global memory where cluster array per
 * thread-block is stored.
 * @param numClusters The total number of clusters.
 * @param numCoord The total number of coordinates.
 * @param maxBlocks The maximum number of thread-block from previous kernel
 * we are trying to reduce.
 * @param clusterBlock It's the index of thread-block from previous kernel.
 * @param coordIndex The coorindator Index
 * @param clusterIndex The clusterIndex
 * @return It returns the coordinate value stored at the index.
 */
__device__ static inline float getIndexValue(float *deviceComputeClusters, //[prev_block_size][num_coords][num_clusters]
                                       int numClusters,
                                       int numCoord,
                                       int maxBlocks,
                                       int clusterBlock,
                                       int coordIndex,
                                       int clusterIndex) {
  if (clusterBlock < maxBlocks && clusterIndex < numClusters && coordIndex < numCoord) {
    int index = getIndex(numClusters, numCoord, clusterBlock, coordIndex, clusterIndex);
    return deviceComputeClusters[index];
  }
  return 0.0;
}

/**
 * This CUDA kernel reduces the cluster and clusterSize Array created by
 * compute-membership kernel at thread-block level. It finally copies the
 * reduced value back to global memory, which will later reduced by host
 * function. This kernel has grid dimension as (x, num_coords, num_clusters).
 * Each coordinator index and clusterIndex can be computed independently in a
 * different thread block thus providing a higher level of parallelism. Each
 * thread is responsible for a thread-block of previous kernel.
 * @param deviceComputeClusters The global memory where cluster array per
 * thread-block is stored.
 * @param deviceComputeClustersSize The global memory where clusterSize array
 * per thread-block is stored.
 * @param deviceClustersReduction The global memory, where the final
 * reduced result for clusters array will be stored.
 * @param deviceClustersSizeReduction The global memory, where the final
 * reduced result for clustersSize array will be stored.
 * @param numClusters The total number of clusters.
 * @param numCoord The total number of coordinates.
 * @param maxBlocks The maximum number of thread-block from previous kernel
 * we are trying to reduce.
 * @param numIntermediates2 The number of threads. It should always be a
 * power of 2 for binary reduction.
 * @return
 */
__global__ static
void reduce_clusters(float *deviceComputeClusters, //[prev_block_size][num_coords][num_clusters]
                    int * deviceComputeClustersSize, // [prev_block_size][num_clusters]
                   float *deviceClustersReduction, // [current_block_size][num_coords][num_clusters]
                   int *deviceClustersSizeReduction, // // [current_block_size][num_clusters]
                   int numClusters,
                   int numCoord,
                   int maxBlocks,
                   int numIntermediates2) {
  extern __shared__ float shared_memoryReduction[];
  float *intermediates = shared_memoryReduction;
  int *intermediateSize = (int*)(shared_memoryReduction + blockDim.x);

  int clusterBlock = blockIdx.x * blockDim.x + threadIdx.x;
  int coordIndex = blockIdx.y;
  int clusterIndex = blockIdx.z;
  //  Copy global intermediate values into shared memory.
  intermediates[threadIdx.x] = getIndexValue(deviceComputeClusters, numClusters, numCoord, maxBlocks, clusterBlock, coordIndex, clusterIndex);

  // If coodIndex is 0, also reduce the clusterSize array
  if (coordIndex == 0) {
    if (clusterBlock < maxBlocks)
    {
      intermediateSize[threadIdx.x] = deviceComputeClustersSize[clusterBlock * numClusters + clusterIndex];
    } else {
      intermediateSize[threadIdx.x] = 0;
    }
  }

  __syncthreads();
  //  numIntermediates2 *must* be a power of two!
  for (unsigned int s = numIntermediates2 / 2; s > 0; s >>= 1) {
    if (threadIdx.x < s) {
      intermediates[threadIdx.x] += intermediates[threadIdx.x + s];
      // Also reduce clusterSize
      if (coordIndex == 0) {
        intermediateSize[threadIdx.x] += intermediateSize[threadIdx.x + s];
      }
    }
    __syncthreads();
  }

  // One of the threads copies the reduced value to global memory
  if (threadIdx.x == 0) {
    deviceClustersReduction[blockIdx.x * numClusters * numCoord + coordIndex * numClusters + clusterIndex] = intermediates[0];
    // Also copy reduced size if reduced by this thread to global memory
    if (coordIndex == 0) {
      deviceClustersSizeReduction[blockIdx.x * numClusters + clusterIndex] = intermediateSize[0];
    }
  }
}

/*----< cuda_kmeans() >-------------------------------------------------------*/
//
//  ----------------------------------------
//  DATA LAYOUT
//
//  objects         [numObjs][numCoords]
//  clusters        [numClusters][numCoords]
//  dimObjects      [numCoords][numObjs]
//  dimClusters     [numCoords][numClusters]
//  newClusters     [numCoords][numClusters]
//  deviceObjects   [numCoords][numObjs]
//  deviceClusters  [numCoords][numClusters]
//  ----------------------------------------
//
/* return an array of cluster centers of size [numClusters][numCoords]       */
float** cuda_kmeans(float **objects,      /* in: [numObjs][numCoords] */
                    int     numCoords,    /* no. features */
                    int     numObjs,      /* no. objects */
                    int     numClusters,  /* no. clusters */
                    float   threshold,    /* % objects change membership */
                    int    *membership,   /* out: [numObjs] */
                    int    *loop_iterations)
{
  int      i, j, loop=0;
  int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
  float    delta;          /* % of objects change their clusters */
  float  **dimObjects;
  float  **clusters;       /* out: [numClusters][numCoords] */
  float  **dimClusters;
  float  **newClusters;    /* [numCoords][numClusters] */

  float *deviceObjects;
  float *deviceClusters;
  int *deviceMembership;
  int *deviceIntermediates;

  float *deviceComputeClusters;
  int *deviceComputeClustersSize;

  int *deviceDeltaReduction;
  int *hostDeltaReduction;
  int *deviceClusterSizeReduction;
  int *hostClusterSizeReduction;
  float *deviceClustersReduction;
  float *hostClustersReduction;

  //  Copy objects given in [numObjs][numCoords] layout to new
  //  [numCoords][numObjs] layout
  malloc2D(dimObjects, numCoords, numObjs, float);
  for (i = 0; i < numCoords; i++) {
    for (j = 0; j < numObjs; j++) {
      dimObjects[i][j] = objects[j][i];
    }
  }

  /* pick first numClusters elements of objects[] as initial cluster centers*/
  malloc2D(dimClusters, numCoords, numClusters, float);
  for (i = 0; i < numCoords; i++) {
    for (j = 0; j < numClusters; j++) {
      dimClusters[i][j] = dimObjects[i][j];
    }
  }

  /* initialize membership[] */
  for (i=0; i<numObjs; i++) membership[i] = -1;

  /* need to initialize newClusterSize and newClusters[0] to all 0 */
  newClusterSize = (int*) calloc(numClusters, sizeof(int));
  assert(newClusterSize != NULL);

  malloc2D(newClusters, numCoords, numClusters, float);
  memset(newClusters[0], 0, numCoords * numClusters * sizeof(float));

  //  To support reduction, numThreadsPerClusterBlock *must* be a power of
  //  two, and it *must* be no larger than the number of bits that will
  //  fit into an unsigned char, the type used to keep track of membership
  //  changes in the kernel.
  const unsigned int numThreadsPerClusterBlock = 128;
  const unsigned int numClusterBlocks =
      (numObjs + numThreadsPerClusterBlock - 1) / numThreadsPerClusterBlock;
  const unsigned int clusterBlockSharedDataSize =
      numThreadsPerClusterBlock * sizeof(unsigned char) +
          numClusters * numCoords * sizeof(float);

  // Below datastructures are initialized for Apoorv's implementation

  /* Below variables are used for delta reduction kernel*/

  /* It should be clusterBlocks divided by number of threads*/
  int deltaReductionBlocks = (numClusterBlocks + MAX_THREADS - 1) / MAX_THREADS;

  /* Total number of threads should be power of 2 for binary reduction */
  int deltaReductionThreads = min(MAX_THREADS, numClusterBlocks);
  int deltaReductionPow2 = nextPowerOfTwo(deltaReductionThreads);

  /* Each thread needs a shared memory to load the delta value */
  const unsigned int deltaReductionSharedDataSize = MAX_THREADS * sizeof(unsigned int);

  /* Allocate memory so that each thread-block could store their final values
   * after reduction to a different location. The host code will finally
   * aggregate them*/
  hostDeltaReduction = (int *)malloc(sizeof(int) * deltaReductionBlocks);
  assert(hostDeltaReduction != NULL);
  checkCuda(cudaMalloc(&deviceDeltaReduction, deltaReductionBlocks*sizeof(int)));

  /* Below variables are used for computing clusters using membership kernel*/
  const unsigned int computeClusterObjectBlocks = (numObjs + MAX_THREADS - 1) / MAX_THREADS;
  /* Each thread block requires to store clusters and clusterSize in their
   * shared memory */
  const unsigned int computeClusterSharedDataSize = sizeof(int) * numClusters + sizeof(float) * numClusters * numCoords;

  /*Allocate enough memory for all thread-blocks */
  checkCuda(cudaMalloc(&deviceComputeClusters, computeClusterObjectBlocks*numClusters*numCoords*sizeof(float)));
  checkCuda(cudaMalloc(&deviceComputeClustersSize, computeClusterObjectBlocks*numClusters*sizeof(int)));

  /* Below variables are used to reduce the clusters and clusterSize from
   * previous kernel */

  /* We have total computeClusterObjectBlocks different datastructures. Let
   * one thread handle each of them */
  int clusterReductionBlocks = (computeClusterObjectBlocks + MAX_THREADS - 1) / MAX_THREADS;
  /* For binary reduction, it is necessary to have power of two as number of
   * threads */
  int clusterReductionThreads = min(MAX_THREADS, computeClusterObjectBlocks);
  int clusterReductionPow2 = nextPowerOfTwo(clusterReductionThreads);

  /* Each thread-block is reducing the size and coordinate value at the same
   * time. Hence we need shared memory for both of them */
  const unsigned int clusterReductionSharedDataSize = sizeof(float) *
      MAX_THREADS + sizeof(int) * MAX_THREADS;

  /* Every coordinate and cluster is independent of each other and can be
   * executed in parallel. We will use X axis as block index, Y axis as
   * coordinator index and z-axis as clusterIndex */
  dim3 blockDim(clusterReductionBlocks, numCoords, numClusters);

  /*Allocate enough host and device memory so that this kernel can store the
   * reduced data in global memory. This data is copied to host where it
   * calculate the final values of clusters and clusterSize */
  hostClusterSizeReduction = (int*)malloc(sizeof(int) *
      clusterReductionBlocks * numClusters);
  assert(hostClusterSizeReduction != NULL);
  hostClustersReduction = (float*)malloc(sizeof(float) *
      clusterReductionBlocks * numClusters * numCoords);
  assert(hostClustersReduction != NULL);
  checkCuda(cudaMalloc(&deviceClusterSizeReduction, clusterReductionBlocks
      * numClusters * sizeof(int)));
  checkCuda(cudaMalloc(&deviceClustersReduction, clusterReductionBlocks
      * numCoords * numClusters * sizeof(float)));

  /**Optimization related datastructures ready here*/

  checkCuda(cudaMalloc(&deviceObjects, numObjs*numCoords*sizeof(float)));
  checkCuda(cudaMalloc(&deviceClusters, numClusters*numCoords*sizeof(float)));
  checkCuda(cudaMalloc(&deviceMembership, numObjs*sizeof(int)));
  checkCuda(cudaMalloc(&deviceIntermediates, deltaReductionThreads*sizeof(unsigned int)));

  /* Copy initial objects and membership to device memory */
  checkCuda(cudaMemcpy(deviceObjects, dimObjects[0],
                       numObjs*numCoords*sizeof(float), cudaMemcpyHostToDevice));
  checkCuda(cudaMemcpy(deviceMembership, membership,
                       numObjs*sizeof(int), cudaMemcpyHostToDevice));

/* The code inside #ifdef PROFILING will be compiled out when PROFILING is
 * not defined. If PROFILING is required please pass a flag as -dPROFILING as
 * compile time parameters. This PROFILING code is really helpful to profile
 * various sections of the implementation */
#ifdef PROFILING
  double findTime = 0;
  double computeTime = 0;
  double newClusterTime = 0;
  double newReductionTime = 0;
  double totalTime = wtime();
#endif
  do {
    /* First copy the clusters back to device memory */
    checkCuda(cudaMemcpy(deviceClusters, dimClusters[0],
                         numClusters*numCoords*sizeof(float), cudaMemcpyHostToDevice));
#ifdef PROFILING
    double beforeFind = wtime();
#endif
    /* Calculate new membership for every object using eucledian method */
    find_nearest_cluster
        <<< numClusterBlocks, numThreadsPerClusterBlock, clusterBlockSharedDataSize >>>
        (numCoords, numObjs, numClusters,
            deviceObjects, deviceClusters, deviceMembership, deviceIntermediates);

//    cudaDeviceSynchronize(); checkLastCudaError();
#ifdef PROFILING
    double afterFind = wtime();
#endif
    /* This kernel reduces the delta calculated by every thread block in
     * previous kernel */
    compute_delta <<< deltaReductionBlocks, deltaReductionPow2, deltaReductionSharedDataSize >>>
            (deviceIntermediates, deviceDeltaReduction, numClusterBlocks, deltaReductionPow2);
//    cudaDeviceSynchronize(); checkLastCudaError();
    // Now aggregate the delta after copying from device
    checkCuda(cudaMemcpy(hostDeltaReduction, deviceDeltaReduction,
                         sizeof(int) * deltaReductionBlocks, cudaMemcpyDeviceToHost));
    int d = 0;
    for (int i = 0; i < deltaReductionBlocks; i++) {
      d += hostDeltaReduction[i];
    }
#ifdef PROFILING
    double afterCompute = wtime();
#endif
    /* This kernel uses the membership array to calculate new cluster and
     * clusterSize array per thread block. This needs to be reduced later */
    compute_membership <<< computeClusterObjectBlocks, MAX_THREADS, computeClusterSharedDataSize>>>
            (   deviceObjects,
                deviceMembership,
                deviceComputeClusters,
                deviceComputeClustersSize,
                numObjs,    //  The actual number of objects
                numClusters,  //  The number of clusters
                numCoords);
//    cudaDeviceSynchronize(); checkLastCudaError();
#ifdef PROFILING
    double kTime = wtime();
#endif
    /* Reduce the clusters and clustersSize array created per thread-block in
     * previous kernel function */
    reduce_clusters <<< blockDim, clusterReductionPow2,
        clusterReductionSharedDataSize >>> (deviceComputeClusters,
            deviceComputeClustersSize, deviceClustersReduction,
            deviceClusterSizeReduction, numClusters,
            numCoords, computeClusterObjectBlocks, clusterReductionPow2);

    // Now aggregate the clusters and clusterSize after copying from device
    checkCuda(cudaMemcpy(hostClusterSizeReduction, deviceClusterSizeReduction,
                         sizeof(int) * clusterReductionBlocks * numClusters, cudaMemcpyDeviceToHost));
    checkCuda(cudaMemcpy(hostClustersReduction, deviceClustersReduction,
                         sizeof(float) * clusterReductionBlocks * numCoords * numClusters, cudaMemcpyDeviceToHost));
    // Agrregate the clustersSize
    for (int i = 0; i < numClusters; i++) {
      newClusterSize[i] = 0;
      for (int j = 0; j < clusterReductionBlocks; j++) {
        newClusterSize[i] += hostClusterSizeReduction[j * numClusters + i];
      }
    }

    // Aggregate the clusters. This is really quick code. PROFILING clearly
    // indicates that this was never a bottleneck in our implementation
    for (int i = 0; i < numClusters; i++) {
      for (int j = 0; j < numCoords; j++) {
        dimClusters[j][i] = 0.0;
        for (int k = 0; k < clusterReductionBlocks; k++) {
          dimClusters[j][i] += hostClustersReduction[k * numClusters * numCoords + j * numClusters + i];
        }
        // Average based on it's size
        if (newClusterSize[i] != 0) {
          dimClusters[j][i] /= newClusterSize[i];
        } else {
          dimClusters[j][i] = 0.0;
        }
      }
    }
#ifdef PROFILING
    double afterReduction = wtime();
    findTime += (afterFind - beforeFind);
    computeTime += (afterCompute - afterFind);
    newClusterTime += (kTime - afterCompute);
    newReductionTime += (afterReduction - afterCompute);
#endif
    delta = d/(float)numObjs;
  } while (delta > threshold && loop++ < 500);

#ifdef PROFILING
  totalTime = wtime() - totalTime;
  printf("Total Time: %10.4f\n", totalTime);
  printf("Total Find Time: %10.4f\n", findTime);
  printf("Total Compute Time: %10.4f\n", computeTime);
  printf("Total New cluster Time: %10.4f\n", newClusterTime);
  printf("Total New Reduction Time: %10.4f\n", newReductionTime);
#endif
  *loop_iterations = loop + 1;

  checkCuda(cudaMemcpy(membership, deviceMembership,
                         numObjs*sizeof(int), cudaMemcpyDeviceToHost));

  /* allocate a 2D space for returning variable clusters[] (coordinates
     of cluster centers) */
  malloc2D(clusters, numClusters, numCoords, float);
  for (i = 0; i < numClusters; i++) {
    for (j = 0; j < numCoords; j++) {
      clusters[i][j] = dimClusters[j][i];
    }
  }

  checkCuda(cudaFree(deviceObjects));
  checkCuda(cudaFree(deviceClusters));
  checkCuda(cudaFree(deviceMembership));
  checkCuda(cudaFree(deviceIntermediates));
  checkCuda(cudaFree(deviceComputeClusters));
  checkCuda(cudaFree(deviceComputeClustersSize));
  checkCuda(cudaFree(deviceClusterSizeReduction));
  checkCuda(cudaFree(deviceClustersReduction));
  checkCuda(cudaFree(deviceDeltaReduction));

  free(dimObjects[0]);
  free(dimObjects);
  free(dimClusters[0]);
  free(dimClusters);
  free(newClusters[0]);
  free(newClusters);
  free(newClusterSize);
  free(hostClusterSizeReduction);
  free(hostClustersReduction);
  free(hostDeltaReduction);

  return clusters;
}