/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         kmeans_clustering.c  (OpenMP version)                     */
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "nmmintrin.h" // for SSE4.2

#include <omp.h>
#include "kmeans.h"
#define LOOP_UNROLL_FACTOR (3)
int UNROLL_MASK = ((-1 >> LOOP_UNROLL_FACTOR) << LOOP_UNROLL_FACTOR);


/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__inline static
float euclid_dist_2(int    numdims,  /* no. dimensions */
                    float *coord1,   /* [numdims] */
                    float *coord2)   /* [numdims] */
{
    int i = 0;
    float ans = 0.0;
    float tmp;
    __m128 vsum = _mm_set1_ps(0.0f);
    int tNumDims = i & UNROLL_MASK;

    /* OPTIMIZATION:
     * LOOP UNROLLING by 2
     * Use SIMD for calculations
    */
    for (i = 0; i < tNumDims; i += 8) {
        __m128 first = _mm_load_ps(&coord1[i]);
        __m128 second = _mm_load_ps(&coord2[i]);
        __m128 diff = _mm_sub_ps(first, second);
        diff = _mm_mul_ps(diff, diff);
        vsum = _mm_add_ps(vsum, diff);

        first = _mm_load_ps(&coord1[i + 4]);
        second = _mm_load_ps(&coord2[i + 4]);
        diff = _mm_sub_ps(first, second);
        vsum = _mm_add_ps(vsum, diff);

    }
    // We need to reduce the vsum and store it back to sum
    vsum = _mm_hadd_ps(vsum, vsum);
    vsum = _mm_hadd_ps(vsum, vsum);
    _mm_store_ss(&ans, vsum);

    // Scaler processing for remaining elements
    for(i= tNumDims;i < numdims; i++) {
        tmp = coord1[i]-coord2[i];
        ans += (tmp * tmp);
    }
    return(ans);
}

/*----< find_nearest_cluster() >---------------------------------------------*/
__inline static
int find_nearest_cluster(int     numClusters, /* no. clusters */
                         int     numCoords,   /* no. coordinates */
                         float  *object,      /* [numCoords] */
                         float **clusters)    /* [numClusters][numCoords] */
{
    int   index, i;
    float dist1, dist2, dist3, dist4, dist5, dist6, dist7, dist8;
    float min_dist;

    /* find the cluster id that has min distance to object */
    index    = -1;
    min_dist = INT_MAX;

    int tNumClusters = numClusters & UNROLL_MASK;

    /**
     * OPTIMIZATION: Loop unrolling by 8
     */
    for (i=0; i<tNumClusters; i+=8) {
        dist1 = euclid_dist_2(numCoords, object, clusters[i]);
        dist2 = euclid_dist_2(numCoords, object, clusters[i + 1]);
        dist3 = euclid_dist_2(numCoords, object, clusters[i + 2]);
        dist4 = euclid_dist_2(numCoords, object, clusters[i + 3]);
        dist5 = euclid_dist_2(numCoords, object, clusters[i + 4]);
        dist6 = euclid_dist_2(numCoords, object, clusters[i + 5]);
        dist7 = euclid_dist_2(numCoords, object, clusters[i + 6]);
        dist8 = euclid_dist_2(numCoords, object, clusters[i + 7]);
        /* no need square root */
        if (dist1 < min_dist) { /* find the min and its array index */
            min_dist = dist1;
            index    = i;
        }
        if (dist2 < min_dist) { /* find the min and its array index */
            min_dist = dist2;
            index    = i + 1;
        }
        if (dist3 < min_dist) { /* find the min and its array index */
            min_dist = dist3;
            index    = i + 2;
        }
        if (dist4 < min_dist) { /* find the min and its array index */
            min_dist = dist4;
            index    = i + 3;
        }
        if (dist5 < min_dist) { /* find the min and its array index */
            min_dist = dist5;
            index    = i + 4;
        }
        if (dist6 < min_dist) { /* find the min and its array index */
            min_dist = dist6;
            index    = i + 5;
        }
        if (dist7 < min_dist) { /* find the min and its array index */
            min_dist = dist7;
            index    = i + 6;
        }
        if (dist8 < min_dist) { /* find the min and its array index */
            min_dist = dist8;
            index    = i + 7;
        }
    }


    // Process the remaining elements
    for (i = tNumClusters; i<numClusters; i++) {
        dist1 = euclid_dist_2(numCoords, object, clusters[i]);
        /* no need square root */
        if (dist1 < min_dist) { /* find the min and its array index */
            min_dist = dist1;
            index    = i;
        }
    }
    return(index);
}


/*----< kmeans_clustering() >------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
float** omp_kmeans(int     is_perform_atomic, /* in: */
                   float **objects,           /* in: [numObjs][numCoords] */
                   int     numCoords,         /* no. coordinates */
                   int     numObjs,           /* no. objects */
                   int     numClusters,       /* no. clusters */
                   float   threshold,         /* % objects change membership */
                   int    *membership)        /* out: [numObjs] */
{

    int      i, j, k, index, loop=0;
    int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float    delta;          /* % of objects change their clusters */
    float  **clusters;       /* out: [numClusters][numCoords] */
    float  **newClusters;    /* [numClusters][numCoords] */
    double   timing;

    int      nthreads;             /* no. threads */
    int    **local_newClusterSize; /* [nthreads][numClusters] */
    float ***local_newClusters;    /* [nthreads][numClusters][numCoords] */
    float ***local_prevClusters;    /* [nthreads][numClusters][numCoords] */

    float **newObjects;     /* [numObjs][numCoords] */

    float iNumObj = 1.0/numObjs;

    nthreads = omp_get_max_threads();

    /* allocate a 2D space for returning variable clusters[] (coordinates
       of cluster centers) */
    clusters    = (float**) malloc(numClusters *             sizeof(float*));
    assert(clusters != NULL);
    clusters[0] = (float*)  malloc(numClusters * numCoords * sizeof(float));
    assert(clusters[0] != NULL);
    for (i=1; i<numClusters; i++)
        clusters[i] = clusters[i-1] + numCoords;

    /* pick first numClusters elements of objects[] as initial cluster centers*/
    for (i=0; i<numClusters; i++)
        for (j=0; j<numCoords; j++)
            clusters[i][j] = objects[i][j];

    /* initialize membership[] */
    for (i=0; i<numObjs; i++) membership[i] = -1;

    /* need to initialize newClusterSize and newClusters[0] to all 0 */
    newClusterSize = (int*) calloc(numClusters, sizeof(int));
    assert(newClusterSize != NULL);

    newClusters    = (float**) malloc(numClusters *            sizeof(float*));
    assert(newClusters != NULL);
    newClusters[0] = (float*)  calloc(numClusters * numCoords, sizeof(float));
    assert(newClusters[0] != NULL);
    for (i=1; i<numClusters; i++)
        newClusters[i] = newClusters[i-1] + numCoords;  

    if (!is_perform_atomic) {
        /* each thread calculates new centers using a private space,
           then thread 0 does an array reduction on them. This approach
           should be faster */
        local_newClusterSize    = (int**) malloc(nthreads * sizeof(int*));
        assert(local_newClusterSize != NULL);
        local_newClusterSize[0] = (int*)  calloc(nthreads*numClusters,
                                                 sizeof(int));
        assert(local_newClusterSize[0] != NULL);
        for (i=1; i<nthreads; i++)
            local_newClusterSize[i] = local_newClusterSize[i-1]+numClusters;

        /* local_newClusters is a 3D array */
        local_newClusters    =(float***)malloc(nthreads * sizeof(float**));
        assert(local_newClusters != NULL);
        local_newClusters[0] =(float**) malloc(nthreads * numClusters *
                                               sizeof(float*));
        assert(local_newClusters[0] != NULL);
        for (i=1; i<nthreads; i++)
            local_newClusters[i] = local_newClusters[i-1] + numClusters;
        for (i=0; i<nthreads; i++) {
            for (j=0; j<numClusters; j++) {
                local_newClusters[i][j] = (float*)calloc(numCoords,
                                                         sizeof(float));
                assert(local_newClusters[i][j] != NULL);
            }
        }

        /* local_prevClusters is a 3D array */
        local_prevClusters    =(float***)malloc(nthreads * sizeof(float**));
        assert(local_prevClusters != NULL);
        local_prevClusters[0] =(float**) malloc(nthreads * numClusters *
                                               sizeof(float*));
        assert(local_prevClusters[0] != NULL);
        for (i=1; i<nthreads; i++)
            local_prevClusters[i] = local_prevClusters[i-1] + numClusters;
        for (i=0; i<nthreads; i++) {
            for (j=0; j<numClusters; j++) {
                local_prevClusters[i][j] = (float*)malloc(numCoords *
                                                         sizeof(float));
                assert(local_prevClusters[i][j] != NULL);
                for(k=0; k<numCoords;k++) {
                    local_prevClusters[i][j][k] = clusters[j][k];
                }
            }
        }

        /* Get the memory aligned coordinates for SIMD */
        newObjects = (float**) malloc(numObjs * sizeof(float*));
        assert(newObjects != NULL);
        size_t coordSize = numCoords * sizeof(float);
        // If numCoordinates * sizeof(float) isnot aligned, add padding
        int rem = coordSize % 16;
        if (rem != 0) {
            coordSize = coordSize + (16 - rem);
        }
        newObjects[0] = (float*) aligned_alloc(16, numObjs * coordSize);
        assert(newObjects[0]);
        memcpy(&newObjects[0], &objects[0], sizeof(float) * numCoords);
        for(i = 1; i < numObjs; i++) {
            newObjects[i] = newObjects[i-1] + coordSize;
            memcpy(&newObjects[i], &objects[i], sizeof(float) * numCoords);
        }
    }

    
    
    int tNumObjs = (numObjs/4) * 4;

    if (_debug) timing = omp_get_wtime();
    do {
        delta = 0.0;

        if (is_perform_atomic) {
            #pragma omp parallel for\
                    private(i,j,index) \
                    firstprivate(numObjs,numClusters,numCoords) \
                    shared(objects,clusters,membership,newClusters,newClusterSize) \
                    schedule(static) \
                    reduction(+:delta)
            for (i=0; i<numObjs; i++) {
                /* find the array index of nestest cluster center */
                index = find_nearest_cluster(numClusters, numCoords, objects[i],
                                             clusters);

                /* if membership changes, increase delta by 1 */
                if (membership[i] != index) delta += 1.0;

                /* assign the membership to object i */
                membership[i] = index;

                /* update new cluster centers : sum of objects located within */
                #pragma omp atomic
                newClusterSize[index]++;
                for (j=0; j<numCoords; j++)
                    #pragma omp atomic
                    newClusters[index][j] += objects[i][j];
            }
            /* average the sum and replace old cluster centers with newClusters */
            for (i=0; i<numClusters; i++) {
                for (j=0; j<numCoords; j++) {
                    if (newClusterSize[i] > 1) {
                        clusters[i][j] = newClusters[i][j] / newClusterSize[i];
                    }
                    newClusters[i][j] = 0.0;   /* set back to 0 */
                }
                newClusterSize[i] = 0;   /* set back to 0 */
            }
        }
        else {
            #pragma omp parallel \
                    shared(newObjects, membership,local_newClusters,local_newClusterSize,local_prevClusters)
            {
                /**
                 * OPTIMIZATION: Loop unrolling by 4
                 */
                int tid = omp_get_thread_num();
                #pragma omp for nowait\
                            private(i,j) \
                            firstprivate(numObjs,tNumObjs,numClusters,numCoords) \
                            schedule(guided, 8) \
                            reduction(+:delta)
                for (i=0; i < tNumObjs; i+=4) {
                    /* find the array index of nestest cluster center */
                    int index1 = find_nearest_cluster(numClusters, numCoords,
                                                 newObjects[i], local_prevClusters[tid]);
                    int index2 = find_nearest_cluster(numClusters, numCoords,
                                                 newObjects[i + 1], local_prevClusters[tid]);
                    int index3 = find_nearest_cluster(numClusters, numCoords,
                                                 newObjects[i + 2], local_prevClusters[tid]);
                    int index4 = find_nearest_cluster(numClusters, numCoords,
                                                 newObjects[i + 3], local_prevClusters[tid]);

                    /* if membership changes, increase delta by 1 */
                    if (membership[i] != index1) delta += 1.0;
                    if (membership[i + 1] != index2) delta += 1.0;
                    if (membership[i + 2] != index3) delta += 1.0;
                    if (membership[i + 3] != index4) delta += 1.0;

                    /* assign the membership to object i */
                    membership[i] = index1;
                    membership[i + 1] = index2;
                    membership[i + 2] = index3;
                    membership[i + 3] = index4;

                    /* update new cluster centers : sum of all objects located
                       within (average will be performed later) */
                    local_newClusterSize[tid][index1]++;
                    local_newClusterSize[tid][index2]++;
                    local_newClusterSize[tid][index3]++;
                    local_newClusterSize[tid][index4]++;

                    for (j=0; j<numCoords; j++) {
                        local_newClusters[tid][index1][j] += newObjects[i][j];
                        local_newClusters[tid][index2][j] += newObjects[i + 1][j];
                        local_newClusters[tid][index3][j] += newObjects[i + 2][j];
                        local_newClusters[tid][index4][j] += newObjects[i + 3][j];
                    }
                }

                // Process the remaining elements from loop unrolling
                #pragma omp for \
                            private(i,j) \
                            firstprivate(numObjs,tNumObjs,numClusters,numCoords) \
                            schedule(static) \
                            reduction(+:delta)
                for ( i = tNumObjs;i<numObjs; i++) {
                    /* find the array index of nestest cluster center */
                    index = find_nearest_cluster(numClusters, numCoords,
                                                 newObjects[i], local_prevClusters[tid]);

                    /* if membership changes, increase delta by 1 */
                    if (membership[i] != index) delta += 1.0;

                    /* assign the membership to object i */
                    membership[i] = index;

                    /* update new cluster centers : sum of all objects located
                       within (average will be performed later) */
                    local_newClusterSize[tid][index]++;
                    for (j=0; j<numCoords; j++)
                        local_newClusters[tid][index][j] += newObjects[i][j];
                }

            }
            /* end of #pragma omp parallel */
            
  
            /* let the main thread perform the array reduction */
            for (i=0; i<numClusters; i++) {
                for (j=0; j<nthreads; j++) {
                    newClusterSize[i] += local_newClusterSize[j][i];
                    local_newClusterSize[j][i] = 0.0;
                    for (k=0; k<numCoords; k++) {
                        newClusters[i][k] += local_newClusters[j][i][k];
                        local_newClusters[j][i][k] = 0.0;
                    }
                }
            }

            /* average the sum and replace old cluster centers with newClusters */
            for (i=0; i<numClusters; i++) {
                for (j=0; j<numCoords; j++) {
                    if (newClusterSize[i] > 1) {
                        clusters[i][j] = newClusters[i][j] / newClusterSize[i];
                        for(k=0; k<nthreads; k++) {
                            local_prevClusters[k][i][j] = clusters[i][j]; /* set back to clusters */
                        }
                    }
                    newClusters[i][j] = 0.0;   /* set back to 0 */
                }
                newClusterSize[i] = 0;   /* set back to 0 */
            }
        }
            
        /**
         * OPTIMIZATION: Let's not use divide operation, instead use *
         */
        delta *= iNumObj;
    } while (delta > threshold && loop++ < 500);

    if (_debug) {
        timing = omp_get_wtime() - timing;
        printf("nloops = %2d (T = %7.4f)",loop,timing);
    }

    // Free all memory
    if (!is_perform_atomic) {
        free(local_newClusterSize[0]);
        free(local_newClusterSize);

        for (i=0; i<nthreads; i++)
            for (j=0; j<numClusters; j++)
                free(local_newClusters[i][j]);
        free(local_newClusters[0]);
        free(local_newClusters);
        free(local_prevClusters[0]);
        free(local_prevClusters);
    }
    free(newClusters[0]);
    free(newClusters);
    free(newClusterSize);

    return clusters;
}

