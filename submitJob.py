#!/usr/bin/python

import sys
import subprocess
import getopt
import random

# Program to format a benchmark run and submit it to the latedays job queue

def usage(name):
    print "Usage: %s -h -J -s NAME -a ARGS -r ROOT -d DIGITS"
    print "  -h         Print this message"
    print "  -J         Don't submit job (just generate command file)"
    print "  -s NAME    Specify command name"
    print "  -a ARGS    Arguments for benchmark.py (can be quoted string)"
    print "  -r ROOT    Specify root name of benchmark output file"
    print "  -d DIGITS  Specify number of randomly generated digits in command and benchmark output file names"
    sys.exit(0)

uniqueId = ""

def generateId(digits):
    id = ""
    for i in range(digits):
        c = chr(random.randint(ord('0'), ord('9')))
        id += c
    return id

def generateFileName(root, extension):
    if uniqueId == "":
        return root + "." + extension
    else:
        return root + "-" + uniqueId  + "." + extension

# Create shell script to submit to qsub
# Results stored in file 'OUTROOT-XXXX.out' with specified number of digits
def generateScript(scriptName = "latedays.sh", argString = "", nodes=2, proc=4):
    try:
        scriptFile = open(scriptName, 'w')
    except Exception as e:
        print "Couldn't open file '%s' (%s)" % (scriptName, str(e))
        return False

    scriptFile.write("#!/bin/bash\n")
    scriptFile.write("# This script lets you submit jobs for execution on the latedays cluster\n")
    scriptFile.write("# You should submit it using qsub:\n")
    scriptFile.write("#   'qsub latedays.sh'\n")
    scriptFile.write("# Upon completion, the output generated on stdout will show up in the\n")
    scriptFile.write("# file latedays.sh.oNNNNN where NNNNN is the job number.  The output\n")
    scriptFile.write("# generated on stderr will show up in the file latedays.sh.eNNNNN.\n")
    scriptFile.write("\n")
    scriptFile.write("# Go to the directory from which you submitted your job\n")
    scriptFile.write("cd $PBS_O_WORKDIR\n")
    scriptFile.write("\n")
    # scriptFile.write("# Limit execution time to 60 minutes\n")
    # scriptFile.write("#PBS -lwalltime=0:60:00\n")
    # scriptFile.write("# Allocate all available CPUs on a single node\n")
    # scriptFile.write("#PBS -l nodes=%d:ppn=%d\n"%(nodes, proc))
    # scriptFile.write("\n")
    scriptFile.write('''
#
#  make a list of allocated nodes(cores)
#  Note that if multiple jobs run in same directory, use different names
#     for example, add on jobid nmber, e.g. nodes.$PBS_JOBID.
cat $PBS_NODEFILE > nodes.$PBS_JOBID

NODE_LIST=`cat $PBS_NODEFILE `
#
# Just for kicks, see which nodes we got.
echo "Nodelist::$NODE_LIST"
echo "NodeCount::$PBS_NP"
    ''');
    scriptFile.write("# Configure to place threads on successive processors\n")
    scriptFile.write("OMP_PLACES=cores\n")
    scriptFile.write("OMP_PROC_BIND=close\n")
    scriptFile.write("\n")
    scriptFile.write("/opt/opt-openmpi/1.8.5rc1/bin/mpirun -np $PBS_NP -machinefile nodes.$PBS_JOBID %s\n" % argString)
    scriptFile.close()
    return True

def command_line(cmd):
    cmdline = " ".join(cmd)
    print "Executing:" + cmdline
    try:
        process = subprocess.Popen(cmd)
    except Exception as e:
        print "Couldn't execute '%s' (%s)" % (cmdline, str(e))
        return
    process.wait()
    if process.returncode != 0:
        print "Error.  Executing '%s' gave return code %d" % (cmdline, process.returncode)

def submit(scriptName, argString=""):
    cmd = ["qsub", argString, scriptName]
    command_line(cmd)


def run(name, args):
    global uniqueId
    submitJob = True
    scriptRoot = "latedays"
    scriptExtension = "sh"
    argString = "echo 'No job submitted'"
    digits = 4
    nodes = 1
    proc = 24
    optlist, args = getopt.getopt(args, "hJs:a:n:p:")
    for (opt, val) in optlist:
        if opt == '-h':
            usage(name)
        elif opt == '-J':
            submitJob = False
        elif opt == '-s':
            scriptRoot = val
        elif opt == '-a':
            argString =  val
        elif opt == '-n':
            nodes = int(val)
        elif opt == '-p':
            proc = int(val)
    uniqueId = generateId(digits)
    scriptName = generateFileName(scriptRoot, scriptExtension)
    if generateScript(scriptName, argString, nodes, proc):
        print "Generated script %s" % scriptName
        if submitJob:
            submit(scriptName, "-l walltime=0:01:00,nodes=%d:ppn=%d"%(nodes, proc))
            #submit(scriptName, "-l walltime=0:01:00")

if __name__ == "__main__":
    run(sys.argv[0], sys.argv[1:])
