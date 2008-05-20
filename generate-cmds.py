#!/usr/bin/env python
import math
import os

FAST_INFO_DICT = {
    'net': 'openib',
    'class': 'fast',
    'tasks_per_node': 2,
    'node': 1,
    'n': 2,
    'in_num': 0,
    'in_file': 'in.txt',
}

FILENAME_TEMPLATE = "parallel_job_in%(in_num)i_%(net)s_%(n)02i.cmd"

def get_class_for_n(n):
    if n <= 8:
        return 'standard'
    if n <= 16:
        return 'short'
    if n <= 24:
        return 'long'

def main():
    run_all_script = [
        '#!/bin/bash',
        ''
    ]
    
    for in_num in (1, 2, 3):
        in_file = 'in-t1-n70-k30-0%d.txt' % in_num
        for net in ("openib", "tcp"):
            for (node, tasks_per_node) in ((1, 1), (1, 2), (1, 4), (2, 4), (4, 4), (5, 4), (6, 4)):
                n = node * tasks_per_node
                info_dict = {
                    'net': net,
                    'class': get_class_for_n(n),
                    'node': node,
                    'tasks_per_node': tasks_per_node,
                    'n': n,
                    'in_num': in_num,
                    'in_file': in_file,
                }
            
                filename = FILENAME_TEMPLATE % info_dict
            
                file = open(filename, "w")
                file.write(CONTENT_TEMPLATE % info_dict)
                file.close()
            
                run_all_script.append('llsubmit %s' % filename);
    
    file = open('parallel_job_fast.cmd', 'w')
    file.write(CONTENT_TEMPLATE % FAST_INFO_DICT)
    file.close()
    
    run_all_script.append('')
    file = open('run_all', 'w')
    file.write('\n'.join(run_all_script))
    file.close()

CONTENT_TEMPLATE = '''#!/bin/bash
#  ===========================================================================
# |                                                                           |
# |             COMMAND FILE FOR SUBMITTING TWS LoadLeveler JOBS              |
# |                                                                           |
# |                                                                           |
# | LoadLeveler keyword statements begin with # @.                            |
# | There can be any number of blanks between the # and the @.                |
# |                                                                           |
# | Comments begin with #.                                                    |
# | Any line whose first non-blank character is a pound sign (#)              |
# | and is not a LoadLeveler keyword statement is regarded as a comment.      |
#  ===========================================================================

#  ---------------------------------------------------------------------------
# | Type of queue: long, short, fast, ...                                     |
# |                                                                           |
# | Specifies the name of a job class defined locally in your cluster.        |
# | You can use the llclass command to find out information on job classes.   |
#  ---------------------------------------------------------------------------
# @ class = %(class)s


#  ---------------------------------------------------------------------------
# | Type of task: parallel or serial.                                         |
# |                                                                           |
# | Specifies the type of job step to process.                                |
#  ---------------------------------------------------------------------------
# @ job_type = parallel


#  ---------------------------------------------------------------------------
# | Specifies whether this job step shares nodes with other job steps.        |
# |                                                                           |
# | where:                                                                    |
# |       shared     - Specifies that nodes can be shared with other tasks of |
# |                    other job steps.                                       |
# |       not_shared - Specifies that nodes are not shared.                   |
# |                    No other job steps are scheduled on this node.         |
#  ---------------------------------------------------------------------------
# @ node_usage = shared


#  ---------------------------------------------------------------------------
# | Name of output files.                                                     |
# |                                                                           |
# | job_name:                                                                 |
# |           Specifies the name of the job. This keyword must be specified   |
# |           in the first job step. If it is specified in other job steps in |
# |           the job command file, it is ignored. The job_name only appears  |
# |           in the long reports of the llq, llstatus, and llsummary         |
# |           commands, and in mail related to the job. You can name the job  |
# |           using any combination of letters, numbers, or both.             |
# |                                                                           |
# | error:                                                                    |
# |         Specifies the name of the file to use as standard error (stderr)  |
# |         when your job step runs.                                          |
# |                                                                           |
# | output:                                                                   |
# |         Specifies the name of the file to use as standard output (stdout) |
# |         when your job step runs.                                          |
#  ---------------------------------------------------------------------------
# @ job_name = job_i%(in_num)d_%(net)s_%(n)02i_%(class)s
# @ error    = $(job_name).$(Host).$(Cluster).$(Process).err
# @ output   = $(job_name).$(Host).$(Cluster).$(Process).out


#  ---------------------------------------------------------------------------
# | Specifies the number of nodes requested by a job step.                    |
# |                                                                           |
# | Range: 1 - 8                                                              |
# |                                                                           |
# | !!! If you set bad value then your job in the queue never run !           |
# | It will waitting to get all nodes where do not exists !                   |
#  ---------------------------------------------------------------------------
# @ node = %(node)i


#  ---------------------------------------------------------------------------
# | Specifies the number of tasks of a parallel job you want to run per node. |
# | Use this keyword together with the node keyword.                          |
# |                                                                           |
# | Range: 1 - 4                                                              |
# |                                                                           |
# | !!! If you set bad value then your job in the queue never run !           |
# | It will waitting to get all nodes where do not exists !                   |
#  ---------------------------------------------------------------------------
# @ tasks_per_node = %(tasks_per_node)i


#  ---------------------------------------------------------------------------
# | Identifies the name of the program to run, which can be a shell script or |
# | a binary. For parallel jobs, executable must be the parallel job launcher |
# | (POE or mpirun), or the name of a program that invokes the parallel job   |
# | launcher.                                                                 |
# |                                                                           |
# | !!! Do not change !!!                                                     |
# |                                                                           |
#  ---------------------------------------------------------------------------
# @ executable = /home/openmpi-1.2.5/bin/mpirun


#  ---------------------------------------------------------------------------
# | Specifies the list of arguments to pass to your program when your job     |
# | runs.                                                                     |
# |                                                                           |
# | values:                                                                   |
# | OPEN_MPI_ARGS YOUR_MPI_PROGRAMS arg1 ...                                  |
# |                                                                           |
# | !!! mandatory parameters:                                                 |
# | !!!!  OPEN_MPI_ARGS - !!! Do not change: !!!                              |
# |                                                                           |
# |                     --mca btl %(net)s,self                                 |
# |                     --mca mpi_paffinity_alone 1                           |
# |                                                                           |
# |       YOUR_MPI_PROGRAMS --- your (MPI) program                            |
# |                                                                           |
# |      arg1 ... --- optional parameters of your (MPI) program.              |
# |                                                                           |
# |                                                                           |
# | !!! Do not change arguments sequence !!!                                  |
#  ---------------------------------------------------------------------------
# @ arguments = --mca mpi_paffinity_alone 1 --mca btl %(net)s,self main %(in_file)s


#  ---------------------------------------------------------------------------
# | Places one copy of the job step in the queue. This statement is required. |
# | The queue statement essentially marks the end of the job step.            |
# | Note that you can specify statements between queue statements.            |
# |                                                                           |
# | !!! Do not remove !!!                                                     |
# |                                                                           |
#  ---------------------------------------------------------------------------
# @ queue




#  ---------------------------------------------------------------------------
# | Enable processor (and potentially memory) affinity                        |
# |                                                                           |
# | !!! Do not change !!! only if you know what do you do                     |
#  ---------------------------------------------------------------------------
# --mca mpi_paffinity_alone 1

#  ---------------------------------------------------------------------------
# | Set network communication openMPI between nodes:                          |
# | Ethernet (tcp) or InfiniBand (openib)                                     |
# | !!! Do not change !!! only if you know what do you do                     |
#  ---------------------------------------------------------------------------
# --mca btl tcp,self
# --mca btl openib,self

'''

if __name__ == '__main__':
    main()