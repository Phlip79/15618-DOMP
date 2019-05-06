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

if [[ "$#" -ne 4 ]]; then
    echo "Illegal number of parameters. Arguments are <host start> <host end> <processor count on each host> <OUTPUT
    CSV FILE>"
    exit 1
fi

#First prepare the hostfile
tmpDir="tmp"
mkdir -p ${tmpDir}
hostFile="$tmpDir/HOSTFILE"
rangeStart=$1
rangeEnd=$2
blacklist=(41 42 43)
procCountEachMachine=$3
hostCount=0
cat /dev/null>${hostFile}
for (( pCount=1; pCount<=procCountEachMachine; pCount++ )); do
    for (( host=$rangeStart; host<=$rangeEnd; host++ )); do
        blacklisted=0
        for blacklistHost in ${blacklist[@]}; do
            if [[ "$blacklistHost" == "$host" ]] ; then
                blacklisted=1
                break
            fi
        done
        if [[ ${blacklisted} == 0 ]] ; then
            echo "ghc$host.ghc.andrew.cmu.edu"
            echo "ghc$host.ghc.andrew.cmu.edu">>${hostFile}
            ((hostCount++))
        fi
    done
done
#inputs=('kmeans01.dat' 'kmeans02.dat' 'kmeans03.dat' 'kmeans04.dat')
#clusterSize=(2 4 6 8 16 32 64 128)
#processorCount=(128 64 32 16 8 4 2 1)
inputs=('kmeans04.dat')
clusterSize=(16)
processorCount=(128 64 32 16 8 4 2 1)
dataDirectory='data/kmeans'
executablePath='build/seq_kmeans'
outputfile=$4
columns="inPutFile,ClusterSize,NP,Master Lib Time,Master CompTime,Master Total Time,Slave Lib Time,Slave CompTime,
Slave Total Time,Cluster Lib Time,Cluster CompTime,Cluster Total Time"
echo ${columns} > ${outputfile}

echo "--------------------------------------------------------------------------------"
uptime
echo "--------------------------------------------------------------------------------"

for input in ${inputs[@]}; do
    echo "Processing file $input"
    for np in ${processorCount[@]}; do
        echo "Processing np $np"
        hFile=${tmpDir}/"host_${np}"
        if [[ ${np} < ${hostCount} ]]; then
            cat ${hostFile} | head -n ${np} > ${hFile}
        else
            cat ${hostFile} > ${hFile}
        fi
        for cluster in ${clusterSize[@]}; do
            echo "Processing cluster $cluster"
            command="mpiexec -np $np -hostfile $hFile $executablePath -o -n $cluster -i $dataDirectory/${input} |grep 'DOMP'"
            echo "Command is $command"
            output=$($command)
            # Only for master
            masterLibTime=$(echo "${output}" | grep 'DOMP Master Library time' | awk '{print $6}')
            masterTotalTime=$(echo "${output}" | grep 'DOMP Master Total time' | awk '{print $6}')
            masterComputationTime=$(echo "scale=2; ${masterTotalTime} - ${masterLibTime}" | bc)

            # Only Slave average
            slaveLibTime=$(echo "${output}" | grep 'DOMP Slave Library time' | awk '{print $6}')
            slaveTotalTime=$(echo "${output}" | grep 'DOMP Slave Total time' | awk '{print $6}')
            slaveComputationTime=$(echo "scale=2; ${slaveComputationTime} - ${slaveLibTime}" | bc)

            # Whole cluster average
            clusterLibTime=$(echo "${output}" | grep 'DOMP Cluster Library time' | awk '{print $6}')
            clusterTotalTime=$(echo "${output}" | grep 'DOMP Cluster Total time' | awk '{print $6}')
            clusterComputationTime=$(echo "scale=2; ${clusterTotalTime} - ${clusterLibTime}" | bc)

            testString=${input}','${cluster}','${np}','${masterLibTime}','${masterTotalTime}','${masterTotalTime}','${slaveLibTime}','${slaveTotalTime}','${slaveComputationTime}','${clusterLibTime}','${clusterTotalTime}','${clusterComputationTime}
            echo ${testString}
            echo ${testString} >> ${outputfile}
        done
        #rm -f ${hFile}
    done
done
#rm -f ${hostFile}