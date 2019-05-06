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

if [[ "$#" -ne 1 ]]; then
    echo "Illegal number of parameters. Arguments are <OUTPUT CSV FILE>"
    exit 1
fi

#inputs=('kmeans01.dat' 'kmeans02.dat' 'kmeans03.dat' 'kmeans04.dat')
#clusterSize=(2 4 6 8 16 32 64 128)
#processorCount=(128 64 32 16 8 4 2 1)
inputs=('kmeans04.dat')
clusterSize=(16)
processorCount=(128 64 32 16 8 4 2 1)
dataDirectory='data/kmeans'
executablePath='build/seq_kmeans'
outputfile=$1
columns="inPutFile,ClusterSize,NP,LibTime,CompTime,TotalTime"
echo ${columns} > ${outputfile}

echo "--------------------------------------------------------------------------------"
uptime
echo "--------------------------------------------------------------------------------"

for input in ${inputs[@]}; do
    echo "Processing file $input"
    for np in ${processorCount[@]}; do
        echo "Processing np $np"
        for cluster in ${clusterSize[@]}; do
            echo "Processing cluster $cluster"
            command="mpiexec -np $np $executablePath -o -n $cluster -i $dataDirectory/${input} |grep 'DOMP'"
            echo "Command is $command"
            output=$($command)
            libTime=$(echo "${output}" | grep 'DOMP Library time' | awk '{print $5}')
            totalTime=$(echo "${output}" | grep 'DOMP Total time' | awk '{print $5}')
            computationTime=$(echo "scale=2; ${totalTime} - ${libTime}" | bc)
            testString=${input}','${cluster}','${np}','${libTime}','${computationTime}','${totalTime}
            echo ${testString}
            echo ${testString} >> ${outputfile}
        done
    done
done