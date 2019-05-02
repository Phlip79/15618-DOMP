/*

    Copyright (C) 2011  Abhinav Jauhri (abhinav.jauhri@gmail.com), Carnegie Mellon University - Silicon Valley 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <omp.h>
#include <memory.h>
#include <stdlib.h>
#include "matrix_mul.h"
#include <stdio.h>
#include "nmmintrin.h" // for SSE4.2


#define MIN_DIM 40 // Minimum matrix dimension to trigger OMP version.

// the MASK variable is used to make the matrix dimension, in multiples
// of four
int MASK = ((-1 >> 2) << 2);

namespace omp
{
    /*
     * Description: 
     * this function is used by the OMP matrix multiplication
     * logic to get the transpose of the second matrix we are
     * using during multiplication. The purpose of this to
     * reduce the number of cache misses when doing the multiplication                      
     *  
     * @param[in]: original matrix
     * @param[in]: transposed matrix
     * @param[in]: matrix dimensions
     *
     * @param[out]: the transposed matrix is stored in trans_matrix variable
     */ 
    void matrix_transpose(float *ori_matrix, float *trans_matrix, unsigned int n) {

        // Iterate over the number of rows in the matrix
        for (int i = 0; i < n; i++) {
            // Iterate over the number of cols in the matrix
            for (int j = 0; j < n; j++) {
                // Performing the transpose logic
                trans_matrix[j*n + i] = ori_matrix[i*n + j];
            }
        }
    }



    /*
     * Description:
     * this function performs the core logic for OMP matrix multiplication.
     * It takes in two matrices sq_matrix_1 and sq_matrix_2 as the TWO inputs
     * and the output of multiplication is store in sq_matrix_result.
     * The logic followed here is that, if the sq_dimension is lesser than the                              
     * MIN_DIM then the normal matrix mulitplication logic with no OMP is
     * followed (to avoid any overhead caused due to OMP). If the sq_dimension
     * is greater than MIN_DIM, matrix transpose of the second matrix occurs
     * then the OMP logic to creating max_thread and the SIMD logic of 
     * loading/multiplying multiple elements of each row 
     * is performed and the result is stored in the appropriate row. 
     *
     * @param[in]: first matrix
     * @param[in]: second matrix
     * @param[in]: resulting matrix after multiplication
     * @param[in]: matrix dimensions
     *
     * @param[out]: result of matrix multiplication stored in sq_matrix_result
     *
     */ 
    void matrix_multiplication(float *sq_matrix_1, float *sq_matrix_2, float *sq_matrix_result, unsigned int sq_dimension ){

        // If the matrix dimension is less than minimum dimension, avoid the
        // overhead of OMP logic involving threads. Use the sequential logic
        if (sq_dimension <= MIN_DIM) {
            for(int i = 0; i < sq_dimension; i++) {
                int row = (i * sq_dimension);
                for(int j = 0 ; j < sq_dimension; j++) {
                    float sum = 0.0f;
                    for(int k = 0; k < sq_dimension; k++)
                        sum += sq_matrix_1[row + k] * sq_matrix_2[k*sq_dimension + j];
                    sq_matrix_result[row + j] = sum;
                }
            } 
            return;
        }

        // This is the main OMP logic that is executed when the matrix dimensions is 
        // greater than the minimum dimensions.
        
        // Variable to store the matrix transpose of second variable
        float *sq_trans_matrix;
        sq_trans_matrix = (float*) malloc (sizeof(float) * sq_dimension * sq_dimension);

        // Calling the matrix transpose logic
        matrix_transpose (sq_matrix_2, sq_trans_matrix, sq_dimension);
        
        #pragma omp parallel
        {
            #pragma omp for
            for (int i = 0; i < sq_dimension; i++) {
                int row = i * sq_dimension;
                for (int j = 0; j < sq_dimension; j++) {
                    int col = j * sq_dimension;
                    float temp = 0.0;
                    // This is the SIMD logic to store 4 float values
                    // at the same time in a vector
                    int last = (sq_dimension & MASK);
                    __m128 vsum = _mm_set1_ps(0.0f);
                    for (int k = 0; k < last; k+=4) {
                        // Read the 4 values from first matrix    
                        __m128 first = _mm_loadu_ps(&sq_matrix_1[row+k]);

                        // Read the 4 values from second matrix
                        __m128 second = _mm_loadu_ps(&sq_trans_matrix[col+k]);

                        // Perform multiplication between the two vectors
                        __m128 prod = _mm_mul_ps(first, second);
                        vsum = _mm_add_ps(vsum, prod);  
                    }

                    // Store the vector sum result into a scalar variable
                    vsum = _mm_hadd_ps(vsum, vsum);
                    vsum = _mm_hadd_ps(vsum, vsum);
                    _mm_store_ss(&temp, vsum);
         
                    // This loop is used to handle the modulo 4, offsets 
                    for (int k = last; k < sq_dimension; k++) { 
                        temp += sq_matrix_1[row + k] * sq_trans_matrix[col + k];
                    }

                    // Store the final result of each iteration in the result matrix    
                    sq_matrix_result[row + j] = temp;
                }
            }
        }
   }
} //namespace omp
