#!/bin/bash
# vim:set ts=8 sw=4 sts=4 et:

# Copyright (c) 2011 Serban Giuroiu
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# ------------------------------------------------------------------------------

set -e

inputs=('kmeans01.dat' 'kmeans02.dat' 'kmeans03.dat' 'kmeans04.dat')
#inputs=('kmeans04.dat')
#inputs=('kmeans04.dat')

mkdir -p profiles

make cuda

echo "--------------------------------------------------------------------------------"
uptime
echo "--------------------------------------------------------------------------------"

# Add quotes around ${input} so that spaces in the filename don't break things
cudaTotalTime=0
cudaNewTotalTime=0

for input in ${inputs[@]}; do
    cudaFileTime=0
    cudaNewFileTime=0

    for k in 2 4 8 16 32 64 128; do
    #for k in 64; do
        cudaTime=$(./cuda_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
        #gprof ./seq_main > profiles/seq-profile-$input-$k.txt
        mv ../../data/${input}.cluster_centres ../../data/${input}-$k.cluster_centres
        mv ../../data/${input}.membership ../../data/${input}-$k.membership
    
        cudaNewTime=$(./cuda_new_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
        set +e
        #diff ../../data/${input}-$k.cluster_centres ../../data/${input}.cluster_centres
        #diff ../../data/${input}-$k.membership ../../data/${input}.membership
        set -e
        # report speedup for individual test case
        speedup=$(echo "scale=2; ${cudaTime} / ${cudaNewTime}" | bc)
        echo "k = $(printf "%3d" $k)  cudaTime = ${cudaTime}s  cudaNewTime = ${cudaNewTime}s  speedup = ${speedup}x"

        # aggregate file times
        cudaFileTime=$(echo "${cudaFileTime} + ${cudaTime}" | bc)
        cudaNewFileTime=$(echo "${cudaNewFileTime} + ${cudaNewTime}" | bc)
    done

    # report total file speedup
    fileSpeedup=$(echo "scale=2; ${cudaFileTime} / ${cudaNewFileTime}" | bc)
    echo "input = ${input}  cudaTime = ${cudaFileTime}s cudaNewTime = ${cudaNewFileTime}s speedup = ${fileSpeedup}x"

    # aggregate total times
    cudaTotalTime=$(echo "${cudaTotalTime} + ${cudaFileTime}" | bc)
    cudaNewTotalTime=$(echo "${cudaNewTotalTime} + ${cudaNewFileTime}" | bc)
    echo "--------------------------------------------------------------------------------"
done

# report overall speedup
overallSpeedup=$(echo "scale=1; ${cudaTotalTime} / ${cudaNewTotalTime}" | bc)
echo "overall speedup = ${overallSpeedup}"
