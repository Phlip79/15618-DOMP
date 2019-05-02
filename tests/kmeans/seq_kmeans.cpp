/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         seq_kmeans.c  (sequential version)                        */
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
#include "mpi.h"
#include "../../lib/domp.h"

using namespace domp;

/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__inline static
float euclid_dist_2(int    numdims,  /* no. dimensions */
                    float *coord1,   /* [numdims] */
                    float *coord2)   /* [numdims] */
{
    int i;
    float ans=0.0;

    for (i=0; i<numdims; i++)
        ans += (coord1[i]-coord2[i]) * (coord1[i]-coord2[i]);

    return(ans);
}

/*----< find_nearest_cluster() >---------------------------------------------*/
__inline static
int find_nearest_cluster(int     numClusters, /* no. clusters */
                         int     numCoords,   /* no. coordinates */
                         float  *object,      /* [numCoords] */
                         float *clusters)    /* [numClusters * numCoords] */
{
    int   index, i;
    float dist, min_dist;

    /* find the cluster id that has min distance to object */
    index    = 0;
    min_dist = euclid_dist_2(numCoords, object, &clusters[0]);

    for (i=1; i<numClusters; i++) {
        dist = euclid_dist_2(numCoords, object, &clusters[i * numCoords]);
        /* no need square root */
        if (dist < min_dist) { /* find the min and its array index */
            min_dist = dist;
            index    = i;
        }
    }
    return(index);
}

/*----< seq_kmeans() >-------------------------------------------------------*/
/* return an array of cluster centers of size [numClusters * numCoords]       */
float* seq_kmeans(float *objects,      /* in: [numObjs * numCoords] */
                   int     numCoords,    /* no. features */
                   int     numObjs,      /* no. objects */
                   int     numClusters,  /* no. clusters */
                   float   threshold,    /* % objects change membership */
                   int    *membership,   /* out: [numObjs] */
                   int    *loop_iterations) {
    int i, j, index, loop = 0;
    int *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float delta;          /* % of objects change their clusters */
    float *clusters;       /* out: [numClusters * numCoords] */
    float *newClusters;    /* [numClusters * numCoords] */

    /* allocate a 2D space for returning variable clusters[] (coordinates
       of cluster centers) */
    clusters = (float *) malloc(numClusters * numCoords * sizeof(float));
    assert(clusters != NULL);

    DOMP_SHARED(objects, 0, numClusters * numCoords);
    DOMP_SYNC;

    /* pick first numClusters elements of objects[] as initial cluster centers*/
    for (i = 0; i < numClusters; i++)
        for (j = 0; j < numCoords; j++)
            clusters[i * numCoords + j] = objects[i * numCoords + j];

    /* need to initialize newClusterSize and newClusters[0] to all 0 */
    newClusterSize = (int*) calloc(numClusters, sizeof(int));
    assert(newClusterSize != NULL);

    newClusters = (float*)  calloc(numClusters * numCoords, sizeof(float));
    assert(newClusters != NULL);

    int offset;
    int size;

    DOMP_PARALLELIZE(numObjs, &offset, &size);
    DOMP_SHARED(objects, offset * numCoords, size * numCoords);
    DOMP_SYNC;

    /* initialize membership[] */
    for (i=offset; i<size; i++) membership[i] = -1;
    do {
        delta = 0.0;

        for (i=offset; i<offset + size; i++) {
            /* find the array index of nestest cluster center */
            index = find_nearest_cluster(numClusters, numCoords, &objects[i * numCoords], clusters);

            /* if membership changes, increase delta by 1 */
            if (membership[i] != index) delta += 1.0;

            /* assign the membership to object i */
            membership[i] = index;

            /* update new cluster centers : sum of objects located within */
            newClusterSize[index]++;
            for (j=0; j<numCoords; j++)
                newClusters[index * numCoords + j] += objects[i * numCoords+ j];
        }

        DOMP_ARRAY_REDUCE_ALL(&delta, MPI_FLOAT, MPI_SUM, 0, 1);
        DOMP_ARRAY_REDUCE_ALL(newClusters, MPI_FLOAT, MPI_SUM, 0, numClusters * numCoords);
        DOMP_ARRAY_REDUCE_ALL(newClusterSize, MPI_INT, MPI_SUM, 0, numClusters);

        /* average the sum and replace old cluster centers with newClusters */
        for (i=0; i<numClusters; i++) {
            for (j = 0; j < numCoords; j++) {
                if (newClusterSize[i] > 0)
                    clusters[i * numCoords + j] = newClusters[i * numCoords + j] / newClusterSize[i];
                newClusters[i * numCoords + j] = 0.0;   /* set back to 0 */
            }
            newClusterSize[i] = 0;   /* set back to 0 */
        }
        if (DOMP_IS_MASTER) {
            std::cout<<"Iteration "<<loop<<", delta is "<<delta<<std::endl;
        }
        delta /= numObjs;
    } while (delta > threshold && loop++ < 500);

    *loop_iterations = loop + 1;

    free(newClusters);
    free(newClusterSize);

    return clusters;
}

