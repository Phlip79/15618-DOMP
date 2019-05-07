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

if [[ "$#" -ne 2 ]]; then
    echo "Illegal number of parameters. Arguments are <host start> <host end> <processor count on each host> <OUTPUT
    CSV FILE>"
    exit 1
fi
suffix=$1
filename="$suffix*.sh.o*"
outputfile=$2

columns="clusterSize,Master Lib Time,Master CompTime,Master Total Time,Slave Lib Time,Slave CompTime,
Slave Total Time,Cluster Lib Time,Cluster CompTime,Cluster Total Time"
echo ${columns} > ${outputfile}

for file in ${filename};do
    echo $file
    #whatever you need with "$file"
    output=$(cat $file| grep "DOMP")
    # Only for master
    masterLibTime=$(echo "${output}" | grep 'DOMP Master Library time' | awk '{print $6}')
    masterTotalTime=$(echo "${output}" | grep 'DOMP Master Total time' | awk '{print $6}')
    masterComputationTime=$(echo "scale=3; ${masterTotalTime} - ${masterLibTime}" | bc)

    # Only Slave average
    slaveLibTime=$(echo "${output}" | grep 'DOMP Slave Library time' | awk '{print $6}')
    slaveTotalTime=$(echo "${output}" | grep 'DOMP Slave Total time' | awk '{print $6}')
    slaveComputationTime=$(echo "scale=3; ${slaveTotalTime} - ${slaveLibTime}" | bc)

    # Whole cluster average
    clusterLibTime=$(echo "${output}" | grep 'DOMP Cluster Library time' | awk '{print $6}')
    clusterTotalTime=$(echo "${output}" | grep 'DOMP Cluster Total time' | awk '{print $6}')
    clusterComputationTime=$(echo "scale=3; ${clusterTotalTime} - ${clusterLibTime}" | bc)

    clusterSize=$(echo "${output}" | grep 'DOMP Cluster Size' | awk '{print $5}')

    testString=${clusterSize}','${masterLibTime}','${masterComputationTime}','${masterTotalTime}','${slaveLibTime}',
    '${slaveComputationTime}','${slaveTotalTime}','${clusterLibTime}','${clusterComputationTime}','${clusterTotalTime}
    echo ${testString}
    echo ${testString} >> ${outputfile}
done