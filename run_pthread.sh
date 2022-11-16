#!/bin/bash
#SBATCH --job-name=your_job_name # Job name
#SBATCH --nodes=1                    # Run all processes on a single node	
#SBATCH --ntasks=20                   # number of processes = 20
#SBATCH --cpus-per-task=1            # Number of CPU cores allocated to each process
#SBATCH --partition=Project            # Partition name: Project or Debug (Debug is default)

# ./pthread $X_RESN $Y_RESN $max_iteration $n_thd
# ./pthreadg $X_RESN $Y_RESN $max_iteration $n_thd

./pthread 10 100 4
./pthread 10 100 8
./pthread 10 100 12
./pthread 10 100 16
./pthread 10 100 20
./pthread 10 100 24
./pthread 10 100 28
./pthread 10 100 32
./pthread 10 100 36

#./pthread 100 100 4
#./pthread 100 100 8
#./pthread 100 100 12
#./pthread 100 100 16
#./pthread 100 100 20
#./pthread 100 100 24
#./pthread 100 100 28
#./pthread 100 100 32
#./pthread 100 100 36

#./pthread 1000 100 4
#./pthread 1000 100 8
#./pthread 1000 100 12
#./pthread 1000 100 16
#./pthread 1000 100 20
#./pthread 1000 100 24
#./pthread 1000 100 28
#./pthread 1000 100 32
#./pthread 1000 100 36