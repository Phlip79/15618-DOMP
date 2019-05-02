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

input='kmeans04.dat'
#input='texture17695.bin'

mkdir -p profiles

make seq omp

echo "--------------------------------------------------------------------------------"
uptime
echo "--------------------------------------------------------------------------------"

# TODO: Add quotes around ${input} so that spaces in the filename don't break things

# for k in 4 8 16 32 64 128; do
#     seqTime=$(./seq_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
#     gprof ./seq_main > profiles/seq-profile-$k.txt
#     mv ../../data/${input}.cluster_centres ../../data/${input}-$k.cluster_centres
#     mv ../../data/${input}.membership ../../data/${input}-$k.membership

#     ompTime=$(./omp_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
#     gprof ./omp_main > profiles/omp-profile-$k.txt
#     diff -q ../../data/${input}-$k.cluster_centres ../../data/${input}.cluster_centres
#     diff -q ../../data/${input}-$k.membership ../../data/${input}.membership

#     speedup=$(echo "scale=1; ${seqTime} / ${ompTime}" | bc)
#     echo "k = $(printf "%3d" $k)  seqTime = ${seqTime}s  ompTime = ${ompTime}s speedup = ${speedup}x"
# done
# 
# 
# 
# 

#Test first
input='kmeans01.dat'
echo "****************************************"
echo "Verifying the binaries"
echo "****************************************"
seqTime=$(./seq_main -o -n 4 -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
ompTime=$(./omp_main -o -n 4 -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
speedup=$(echo "scale=1; ${seqTime} / ${ompTime}" | bc)
echo "input=$input, k = $(printf "%3d" $k)  seqTime = ${seqTime}s  ompTime = ${ompTime}s speedup = ${speedup}x"
ompTime=$(./omp_main_basic -o -n 4 -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
speedup=$(echo "scale=1; ${seqTime} / ${ompTime}" | bc)
echo "input=$input, k = $(printf "%3d" $k)  seqTime = ${seqTime}s  ompTime = ${ompTime}s speedup = ${speedup}x"


echo "****************************************"
echo "Now running your implementation first"
echo "****************************************"
ompTotalTime=0
seqTotalTime=0
for input in  'kmeans01.dat' 'kmeans02.dat' 'kmeans03.dat' 'kmeans04.dat'; do
    for k in 2 4 8 16 32 64 128; do
        seqTime=$(./seq_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
        gprof ./seq_main > profiles/seq-profile-$k.txt
        mv ../../data/${input}.cluster_centres ../../data/${input}-$k.cluster_centres
        mv ../../data/${input}.membership ../../data/${input}-$k.membership

        ompTime=$(./omp_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
        # set +e
        # gprof ./omp_main > profiles/omp-profile-$k.txt
        # diff ../../data/${input}-$k.cluster_centres ../../data/${input}.cluster_centres
        # # diff ../../data/${input}-$k.membership ../../data/${input}.membership
        # set -e
        speedup=$(echo "scale=1; ${seqTime} / ${ompTime}" | bc)
        ompTotalTime=$(echo "scale=1; ${ompTime} + ${ompTotalTime}" | bc)
        seqTotalTime=$(echo "scale=1; ${seqTime} + ${seqTotalTime}" | bc)
        echo "input=$input, k = $(printf "%3d" $k)  seqTime = ${seqTime}s  ompTime = ${ompTime}s speedup = ${speedup}x"
    done
done
echo "Sequential total time is $seqTotalTime"
echo "OMP total time is $ompTotalTime"
finalSpeedup=$(echo "scale=1; ${seqTotalTime} / ${ompTotalTime}" | bc)
echo "Our final speedup is ${finalSpeedup}x"
# echo "****************************************"
# echo "Now running default implementation"
# echo "****************************************"
# # Calculate the basic version now
# basicSpeedup=0
# for input in  'kmeans01.dat' 'kmeans02.dat' 'kmeans03.dat' 'kmeans04.dat'; do
#     for k in 2 4 8 16 32 64 128; do
#         seqTime=$(./seq_main -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
#         gprof ./seq_main > profiles/seq-profile-$k.txt
#         mv ../../data/${input}.cluster_centres ../../data/${input}-$k.cluster_centres
#         mv ../../data/${input}.membership ../../data/${input}-$k.membership

#         ompTime=$(./omp_main_basic -o -n $k -i ../../data/${input} | grep 'Computation' | awk '{print $4}')
#         # gprof ./omp_main > profiles/omp-profile-$k.txt
#         # diff -q ../../data/${input}-$k.cluster_centres ../../data/${input}.cluster_centres
#         # diff -q ../../data/${input}-$k.membership ../../data/${input}.membership

#         speedup=$(echo "scale=1; ${seqTime} / ${ompTime}" | bc)
#         basicSpeedup=$(echo "scale=1; ${speedup} + ${basicSpeedup}" | bc)
#         echo "input=$input, k = $(printf "%3d" $k)  seqTime = ${seqTime}s  ompTime = ${ompTime}s speedup = ${speedup}x"
#     done
# done    
# echo "Their total speedup is $basicSpeedup"

# finalSpeedup=$(echo "scale=1; ${ourSpeedup} / ${basicSpeedup}" | bc)
# echo "Our final speedup is $finalSpeedup"