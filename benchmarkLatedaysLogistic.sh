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
    echo "Illegal number of parameters. Arguments are <suffix>"
    exit 1
fi

inputs=('kmeans04.dat')
clusterSize=(16)
processorCount=(16 8 4 2 1)
suffix=$1

echo "--------------------------------------------------------------------------------"
uptime
echo "--------------------------------------------------------------------------------"

# process for multiple nodes
for (( pCount=14; pCount>=1; pCount-- )); do
    echo "Processing np $pCount"
    command='./submitJob.py -a "build/logisticRegression -i data/logistic/input1M.txt -l data/logistic/data1M.txt" -s'"${suffix}_${pCount}_24 -n $pCount -p 24"
    echo "Command is $command"
    eval ${command}
done
# Process for single node also
for np in ${processorCount[@]}; do
    echo "Processing np $np"
    command="./submitJob.py -a 'build/logisticRegression -i data/logistic/input1M.txt -l data/logistic/data1M.txt' -s ${suffix}_1_${np} -n 1 -p $np"
    echo "Command is $command"
    eval ${command}
done
#rm -f ${hostFile}